// Fill out your copyright notice in the Description page of Project Settings.

#include "AGSDTrainSplineLibrary.h"

#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

bool UAGSDTrainSplineLibrary::UpdateLoopingTrainWithCarriages(
	AActor* LeaderTrain,
	const TArray<AActor*>& Carriages,
	USplineComponent* SplineComp,
	double DeltaSeconds,
	bool bShouldMove,
	const FAGSDLoopingTrainSettings& Settings,
	double& CurrentSpeed,
	double& CurrentDistance,
	float& OutNormalizedSpeed)
{
	OutNormalizedSpeed = 0.0f;

	if (!IsValid(LeaderTrain) || !IsValid(SplineComp))
	{
		return false;
	}

	if (Settings.bEnsureClosedLoop && !SplineComp->IsClosedLoop())
	{
		SplineComp->SetClosedLoop(true, true);
	}

	const double SplineLength = static_cast<double>(SplineComp->GetSplineLength());
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	// CurrentDistance가 음수(예: 기본값 -1.0)로 전달되면 첫 호출 시 현재 배치 위치로 자동 초기화
	if (CurrentDistance < 0.0)
	{
		CurrentDistance = FindClosestDistanceOnSpline(LeaderTrain, SplineComp);
	}

	double EffectiveDeltaSeconds = DeltaSeconds;
	if (EffectiveDeltaSeconds <= 0.0)
	{
		if (const UWorld* World = LeaderTrain->GetWorld())
		{
			EffectiveDeltaSeconds = static_cast<double>(World->GetDeltaSeconds());
		}
	}

	// 1. 기관차 속도 가감속 및 스플라인 순환 거리 갱신
	AdvanceLoopingTrainKinematics(
		EffectiveDeltaSeconds,
		bShouldMove,
		Settings,
		SplineLength,
		CurrentSpeed,
		CurrentDistance
	);

	OutNormalizedSpeed = (Settings.MaxSpeed > KINDA_SMALL_NUMBER)
		? static_cast<float>(FMath::Clamp(CurrentSpeed / Settings.MaxSpeed, 0.0, 1.0))
		: 0.0f;

	// 2. 선두 기관차(LeaderTrain) 위치 및 회전 적용
	FVector LeaderLoc = FVector::ZeroVector;
	FRotator LeaderRot = FRotator::ZeroRotator;
	ComputeSplineActorTransform(SplineComp, CurrentDistance, Settings, LeaderLoc, LeaderRot);
	ApplyTrainTransformSafely(LeaderTrain, LeaderLoc, LeaderRot, Settings.bDisablePawnMovement);

	// 3. 연결된 화물칸(Carriages)들을 고정 간격 뒤에 정확히 배치
	const double SpacingSign = Settings.bReverseDirection ? 1.0 : -1.0;
	for (int32 Index = 0; Index < Carriages.Num(); ++Index)
	{
		AActor* Carriage = Carriages[Index];
		if (!IsValid(Carriage))
		{
			continue;
		}

		const double Offset = static_cast<double>(Index + 1) * Settings.CarriageSpacing * SpacingSign;
		const double CarriageDist = WrapSplineDistance(CurrentDistance + Offset, SplineLength);

		FVector CarriageLoc = FVector::ZeroVector;
		FRotator CarriageRot = FRotator::ZeroRotator;
		ComputeSplineActorTransform(SplineComp, CarriageDist, Settings, CarriageLoc, CarriageRot);
		ApplyTrainTransformSafely(Carriage, CarriageLoc, CarriageRot, Settings.bDisablePawnMovement);
	}

	return true;
}

double UAGSDTrainSplineLibrary::FindClosestDistanceOnSpline(const AActor* TargetActor, const USplineComponent* SplineComp)
{
	if (!IsValid(TargetActor) || !IsValid(SplineComp))
	{
		return 0.0;
	}

	const double SplineLength = static_cast<double>(SplineComp->GetSplineLength());
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return 0.0;
	}

	const float InputKey = SplineComp->FindInputKeyClosestToWorldLocation(TargetActor->GetActorLocation());
	const double Distance = static_cast<double>(SplineComp->GetDistanceAlongSplineAtSplineInputKey(InputKey));
	return WrapSplineDistance(Distance, SplineLength);
}

void UAGSDTrainSplineLibrary::AdvanceLoopingTrainKinematics(
	double DeltaSeconds,
	bool bShouldMove,
	const FAGSDLoopingTrainSettings& Settings,
	double SplineLength,
	double& InOutCurrentSpeed,
	double& InOutCurrentDistance)
{
	const double ClampedMaxSpeed = FMath::Max(0.0, Settings.MaxSpeed);

	if (bShouldMove)
	{
		if (Settings.Acceleration > 0.0)
		{
			InOutCurrentSpeed = FMath::Min(ClampedMaxSpeed, InOutCurrentSpeed + (Settings.Acceleration * DeltaSeconds));
		}
		else
		{
			InOutCurrentSpeed = ClampedMaxSpeed;
		}
	}
	else
	{
		if (Settings.Deceleration > 0.0)
		{
			InOutCurrentSpeed = FMath::Max(0.0, InOutCurrentSpeed - (Settings.Deceleration * DeltaSeconds));
		}
		else
		{
			InOutCurrentSpeed = 0.0;
		}
	}

	const double DirectionSign = Settings.bReverseDirection ? -1.0 : 1.0;
	const double DeltaDistance = InOutCurrentSpeed * DeltaSeconds * DirectionSign;
	InOutCurrentDistance = WrapSplineDistance(InOutCurrentDistance + DeltaDistance, SplineLength);
}

void UAGSDTrainSplineLibrary::ComputeSplineActorTransform(
	const USplineComponent* SplineComp,
	double Distance,
	const FAGSDLoopingTrainSettings& Settings,
	FVector& OutWorldLocation,
	FRotator& OutWorldRotation)
{
	const FVector SplineLocation = SplineComp->GetLocationAtDistanceAlongSpline(static_cast<float>(Distance), ESplineCoordinateSpace::World);
	FRotator BaseRot = SplineComp->GetRotationAtDistanceAlongSpline(static_cast<float>(Distance), ESplineCoordinateSpace::World);

	if (Settings.bFlattenPitchAndRoll)
	{
		BaseRot.Pitch = 0.0f;
		BaseRot.Roll = 0.0f;
	}

	if (Settings.bReverseDirection)
	{
		BaseRot.Yaw = FRotator::NormalizeAxis(BaseRot.Yaw + 180.0f);
		BaseRot.Pitch = -BaseRot.Pitch;
		BaseRot.Roll = -BaseRot.Roll;
	}

	const FQuat BaseQuat = BaseRot.Quaternion();
	const FQuat OffsetQuat = Settings.RotationOffset.Quaternion();

	OutWorldRotation = (BaseQuat * OffsetQuat).Rotator();
	OutWorldLocation = SplineLocation + (BaseQuat.GetUpVector() * Settings.HeightOffset);
}

void UAGSDTrainSplineLibrary::ApplyTrainTransformSafely(
	AActor* TargetActor,
	const FVector& WorldLocation,
	const FRotator& WorldRotation,
	bool bDisablePawnMovement)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	if (bDisablePawnMovement)
	{
		if (UPawnMovementComponent* MoveComp = TargetActor->FindComponentByClass<UPawnMovementComponent>())
		{
			if (MoveComp->IsActive())
			{
				MoveComp->StopMovementImmediately();
				MoveComp->Deactivate();
			}
			if (MoveComp->IsComponentTickEnabled())
			{
				MoveComp->SetComponentTickEnabled(false);
			}
		}
	}

	// ETeleportType::None을 사용해 위에 올라탄 캐릭터의 Moving Base가 깨지지 않도록 유지
	TargetActor->SetActorLocationAndRotation(WorldLocation, WorldRotation, false, nullptr, ETeleportType::None);
}

bool UAGSDTrainSplineLibrary::UpdateLoopingSplineFollow(
	AActor* TargetActor,
	USplineComponent* SplineComp,
	double DeltaSeconds,
	double& OutCurrentDistance,
	FRotator& OutTargetRotation,
	double LookAheadDistance,
	float RotationInterpSpeed,
	float RailCenterCorrectionSpeed,
	bool bFollowSplinePitch,
	bool bPreserveActorZ,
	bool bReverseDirection,
	bool bEnsureClosedLoop)
{
	OutCurrentDistance = 0.0;
	OutTargetRotation = FRotator::ZeroRotator;

	if (!IsValid(TargetActor) || !IsValid(SplineComp))
	{
		return false;
	}

	if (bEnsureClosedLoop && !SplineComp->IsClosedLoop())
	{
		SplineComp->SetClosedLoop(true, true);
	}

	const double SplineLength = static_cast<double>(SplineComp->GetSplineLength());
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	double EffectiveDeltaSeconds = DeltaSeconds;
	if (EffectiveDeltaSeconds <= 0.0)
	{
		if (const UWorld* World = TargetActor->GetWorld())
		{
			EffectiveDeltaSeconds = static_cast<double>(World->GetDeltaSeconds());
		}
	}

	const FVector ActorLocation = TargetActor->GetActorLocation();
	FVector RailCenterLocation = ActorLocation;

	EvaluateLoopingSplineTarget(
		SplineComp,
		ActorLocation,
		LookAheadDistance,
		bFollowSplinePitch,
		bPreserveActorZ,
		bReverseDirection,
		OutCurrentDistance,
		RailCenterLocation,
		OutTargetRotation
	);

	const FRotator CurrentRotation = TargetActor->GetActorRotation();
	const FRotator DesiredRotation = (RotationInterpSpeed > 0.0f && EffectiveDeltaSeconds > 0.0)
		? FMath::RInterpTo(CurrentRotation, OutTargetRotation, static_cast<float>(EffectiveDeltaSeconds), RotationInterpSpeed)
		: OutTargetRotation;

	TargetActor->SetActorRotation(DesiredRotation, ETeleportType::None);

	if (RailCenterCorrectionSpeed > 0.0f && EffectiveDeltaSeconds > 0.0)
	{
		const FVector CorrectedLocation = FMath::VInterpTo(
			ActorLocation,
			RailCenterLocation,
			static_cast<float>(EffectiveDeltaSeconds),
			RailCenterCorrectionSpeed
		);

		if (!CorrectedLocation.Equals(ActorLocation, KINDA_SMALL_NUMBER))
		{
			TargetActor->SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::None);
		}
	}

	return true;
}

double UAGSDTrainSplineLibrary::WrapSplineDistance(double Distance, double SplineLength)
{
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return 0.0;
	}

	double Wrapped = FMath::Fmod(Distance, SplineLength);
	if (Wrapped < 0.0)
	{
		Wrapped += SplineLength;
	}
	return Wrapped;
}

bool UAGSDTrainSplineLibrary::GetTrainCarriageFollowTransform(
	const AActor* LeaderActor,
	const USplineComponent* SplineComp,
	double FollowOffsetDistance,
	bool bReverseDirection,
	FVector& OutTargetLocation,
	FRotator& OutTargetRotation,
	double& OutTargetDistance)
{
	OutTargetLocation = FVector::ZeroVector;
	OutTargetRotation = FRotator::ZeroRotator;
	OutTargetDistance = 0.0;

	if (!IsValid(LeaderActor) || !IsValid(SplineComp))
	{
		return false;
	}

	const double SplineLength = static_cast<double>(SplineComp->GetSplineLength());
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float LeaderKey = SplineComp->FindInputKeyClosestToWorldLocation(LeaderActor->GetActorLocation());
	const double LeaderDistance = static_cast<double>(SplineComp->GetDistanceAlongSplineAtSplineInputKey(LeaderKey));
	const double SignedOffset = bReverseDirection ? FollowOffsetDistance : -FollowOffsetDistance;

	OutTargetDistance = WrapSplineDistance(LeaderDistance + SignedOffset, SplineLength);
	OutTargetLocation = SplineComp->GetLocationAtDistanceAlongSpline(static_cast<float>(OutTargetDistance), ESplineCoordinateSpace::World);
	OutTargetRotation = SplineComp->GetRotationAtDistanceAlongSpline(static_cast<float>(OutTargetDistance), ESplineCoordinateSpace::World);

	if (bReverseDirection)
	{
		OutTargetRotation.Yaw = FRotator::NormalizeAxis(OutTargetRotation.Yaw + 180.0f);
		OutTargetRotation.Pitch = -OutTargetRotation.Pitch;
		OutTargetRotation.Roll = -OutTargetRotation.Roll;
	}

	return true;
}

void UAGSDTrainSplineLibrary::EvaluateLoopingSplineTarget(
	const USplineComponent* SplineComp,
	const FVector& ActorLocation,
	double LookAheadDistance,
	bool bFollowSplinePitch,
	bool bPreserveActorZ,
	bool bReverseDirection,
	double& OutCurrentDistance,
	FVector& OutRailCenterLocation,
	FRotator& OutTargetRotation)
{
	const double SplineLength = static_cast<double>(SplineComp->GetSplineLength());
	const float CurrentKey = SplineComp->FindInputKeyClosestToWorldLocation(ActorLocation);
	OutCurrentDistance = WrapSplineDistance(
		static_cast<double>(SplineComp->GetDistanceAlongSplineAtSplineInputKey(CurrentKey)),
		SplineLength
	);

	const FVector CurrentClosestSplinePoint = SplineComp->GetLocationAtDistanceAlongSpline(
		static_cast<float>(OutCurrentDistance),
		ESplineCoordinateSpace::World
	);

	OutRailCenterLocation = CurrentClosestSplinePoint;
	if (bPreserveActorZ)
	{
		OutRailCenterLocation.Z = ActorLocation.Z;
	}

	const double ClampedLookAhead = FMath::Max(1.0, LookAheadDistance);
	const double SignedLookAhead = bReverseDirection ? -ClampedLookAhead : ClampedLookAhead;
	const double LookAheadDist = WrapSplineDistance(OutCurrentDistance + SignedLookAhead, SplineLength);

	const FVector LookAheadSplinePoint = SplineComp->GetLocationAtDistanceAlongSpline(
		static_cast<float>(LookAheadDist),
		ESplineCoordinateSpace::World
	);

	FVector AdjustedLookAtTarget = LookAheadSplinePoint;
	if (bPreserveActorZ)
	{
		AdjustedLookAtTarget.Z = ActorLocation.Z + (LookAheadSplinePoint.Z - CurrentClosestSplinePoint.Z);
	}

	OutTargetRotation = UKismetMathLibrary::FindLookAtRotation(ActorLocation, AdjustedLookAtTarget);
	if (!bFollowSplinePitch)
	{
		OutTargetRotation.Pitch = 0.0f;
		OutTargetRotation.Roll = 0.0f;
	}
}
