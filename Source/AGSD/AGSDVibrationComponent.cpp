// Fill out your copyright notice in the Description page of Project Settings.

#include "AGSDVibrationComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UAGSDVibrationComponent::UAGSDVibrationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAGSDVibrationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bRandomizePhaseOnStart)
	{
		PhaseOffsetSeconds = FMath::FRandRange(0.0, 100.0);
	}

	ResolveDefaultTargetsIfNeeded();

	if (bAutoPlayOnBeginPlay)
	{
		ActivateVibration(false);
	}
}

void UAGSDVibrationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetTargetsToInitialTransform();
	Super::EndPlay(EndPlayReason);
}

void UAGSDVibrationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateRampState(DeltaTime);
	BroadcastIntensityIfChanged();

	if (CurrentState == EAGSDVibrationState::Stopped)
	{
		return;
	}

	AccumulatedTime += static_cast<double>(DeltaTime);

	if (!IsWithinActiveDistance())
	{
		if (!bSuspendedByDistance)
		{
			bSuspendedByDistance = true;
			ResetTargetsToInitialTransform();
		}
		return;
	}
	bSuspendedByDistance = false;

	const float IntensityAlpha = GetCurrentIntensityAlpha();
	FVector LocationOffset = FVector::ZeroVector;
	FRotator RotationOffset = FRotator::ZeroRotator;
	EvaluateVibrationOffset(AccumulatedTime + PhaseOffsetSeconds, IntensityAlpha, LocationOffset, RotationOffset);

	ApplyOffsetsToTargets(LocationOffset, RotationOffset);
}

void UAGSDVibrationComponent::ActivateVibration(bool bInstant)
{
	ResolveDefaultTargetsIfNeeded();

	const bool bWasCompletelyStopped = (CurrentState == EAGSDVibrationState::Stopped);

	if (bInstant || Settings.RampUpDuration <= KINDA_SMALL_NUMBER)
	{
		const bool bWasAlreadyFull = (CurrentState == EAGSDVibrationState::Running && LinearRampProgress >= 1.0f);
		LinearRampProgress = 1.0f;
		CurrentState = EAGSDVibrationState::Running;
		bSuspendedByDistance = false;
		SetComponentTickEnabled(true);

		if (bWasCompletelyStopped)
		{
			OnVibrationStarted.Broadcast();
		}
		BroadcastIntensityIfChanged();
		if (!bWasAlreadyFull)
		{
			OnVibrationFullSpeed.Broadcast();
		}
		return;
	}

	if (CurrentState == EAGSDVibrationState::Running || CurrentState == EAGSDVibrationState::RampingUp)
	{
		return;
	}

	CurrentState = EAGSDVibrationState::RampingUp;
	bSuspendedByDistance = false;
	SetComponentTickEnabled(true);

	if (bWasCompletelyStopped)
	{
		OnVibrationStarted.Broadcast();
	}
	BroadcastIntensityIfChanged();
}

void UAGSDVibrationComponent::DeactivateVibration(bool bInstant)
{
	if (CurrentState == EAGSDVibrationState::Stopped)
	{
		return;
	}

	if (bInstant || Settings.RampDownDuration <= KINDA_SMALL_NUMBER)
	{
		EnterStoppedState();
		return;
	}

	CurrentState = EAGSDVibrationState::RampingDown;
	SetComponentTickEnabled(true);
}

void UAGSDVibrationComponent::ToggleVibration(bool bInstant)
{
	if (IsVibrating())
	{
		DeactivateVibration(bInstant);
	}
	else
	{
		ActivateVibration(bInstant);
	}
}

void UAGSDVibrationComponent::SetTargetComponent(USceneComponent* NewTarget)
{
	ClearTargetComponents();
	if (NewTarget)
	{
		RegisterTargetInternal(NewTarget);
	}
}

void UAGSDVibrationComponent::AddTargetComponent(USceneComponent* AdditionalTarget)
{
	if (AdditionalTarget)
	{
		RegisterTargetInternal(AdditionalTarget);
	}
}

void UAGSDVibrationComponent::ClearTargetComponents()
{
	ResetTargetsToInitialTransform();
	TargetEntries.Empty();
}

void UAGSDVibrationComponent::ResetTargetsToInitialTransform()
{
	for (const FAGSDVibrationTargetEntry& Entry : TargetEntries)
	{
		USceneComponent* Target = Entry.TargetComponent.Get();
		if (IsValid(Target) && Entry.bHasSavedInitialTransform)
		{
			Target->SetRelativeLocationAndRotation(Entry.InitialRelativeLocation, Entry.InitialRelativeRotation);
		}
	}
}

float UAGSDVibrationComponent::GetCurrentIntensityAlpha() const
{
	const float ClampedProgress = FMath::Clamp(LinearRampProgress, 0.0f, 1.0f);
	const float Exponent = FMath::Max(0.1f, Settings.RampCurveExponent);
	return FMath::Pow(ClampedProgress, Exponent);
}

void UAGSDVibrationComponent::ResolveDefaultTargetsIfNeeded()
{
	const bool bHasAnyValidTarget = TargetEntries.ContainsByPredicate([](const FAGSDVibrationTargetEntry& Entry)
	{
		return Entry.TargetComponent.IsValid();
	});
	if (bHasAnyValidTarget)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	// 1. 사용자가 에디터에 TargetComponentNames를 직접 입력한 경우 우선 매칭
	if (TargetComponentNames.Num() > 0)
	{
		TArray<USceneComponent*> AllSceneComps;
		Owner->GetComponents<USceneComponent>(AllSceneComps);

		for (USceneComponent* SceneComp : AllSceneComps)
		{
			if (IsValid(SceneComp) && TargetComponentNames.Contains(SceneComp->GetFName()))
			{
				if (SceneComp == Owner->GetRootComponent())
				{
					UE_LOG(LogTemp, Warning, TEXT("[%s] AGSDVibrationComponent: TargetComponent '%s' is the RootComponent. Consider vibrating a child mesh instead."),
						*Owner->GetName(), *SceneComp->GetName());
				}
				RegisterTargetInternal(SceneComp);
			}
		}
		return;
	}

	// 2. 이름이 비어있으면 루트(Root)가 아닌 첫 번째 스태틱 메시 컴포넌트 자동 탐색
	USceneComponent* RootComp = Owner->GetRootComponent();
	TArray<UStaticMeshComponent*> StaticMeshes;
	Owner->GetComponents<UStaticMeshComponent>(StaticMeshes);

	for (UStaticMeshComponent* StaticMeshComp : StaticMeshes)
	{
		if (IsValid(StaticMeshComp) && StaticMeshComp != RootComp)
		{
			RegisterTargetInternal(StaticMeshComp);
			return;
		}
	}

	// 3. 스태틱 메시가 없으면 루트가 아닌 일반 메시 컴포넌트(스켈레탈 등) 탐색
	TArray<UMeshComponent*> AnyMeshes;
	Owner->GetComponents<UMeshComponent>(AnyMeshes);

	for (UMeshComponent* MeshComp : AnyMeshes)
	{
		if (IsValid(MeshComp) && MeshComp != RootComp)
		{
			RegisterTargetInternal(MeshComp);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] AGSDVibrationComponent: No non-root mesh component found automatically. Assign TargetComponentNames or call SetTargetComponent."),
		*Owner->GetName());
}

bool UAGSDVibrationComponent::RegisterTargetInternal(USceneComponent* Target)
{
	if (!IsValid(Target))
	{
		return false;
	}

	const bool bAlreadyExists = TargetEntries.ContainsByPredicate([Target](const FAGSDVibrationTargetEntry& Entry)
	{
		return Entry.TargetComponent.Get() == Target;
	});
	if (bAlreadyExists)
	{
		return false;
	}

	FAGSDVibrationTargetEntry NewEntry;
	NewEntry.TargetComponent = Target;
	NewEntry.InitialRelativeLocation = Target->GetRelativeLocation();
	NewEntry.InitialRelativeRotation = Target->GetRelativeRotation();
	NewEntry.bHasSavedInitialTransform = true;

	TargetEntries.Add(NewEntry);
	return true;
}

void UAGSDVibrationComponent::UpdateRampState(float DeltaTime)
{
	if (CurrentState == EAGSDVibrationState::RampingUp)
	{
		if (Settings.RampUpDuration <= KINDA_SMALL_NUMBER)
		{
			LinearRampProgress = 1.0f;
			CurrentState = EAGSDVibrationState::Running;
			OnVibrationFullSpeed.Broadcast();
			return;
		}

		LinearRampProgress = FMath::Min(1.0f, LinearRampProgress + (DeltaTime / Settings.RampUpDuration));
		if (LinearRampProgress >= 1.0f)
		{
			CurrentState = EAGSDVibrationState::Running;
			OnVibrationFullSpeed.Broadcast();
		}
		return;
	}

	if (CurrentState == EAGSDVibrationState::RampingDown)
	{
		if (Settings.RampDownDuration <= KINDA_SMALL_NUMBER)
		{
			EnterStoppedState();
			return;
		}

		LinearRampProgress = FMath::Max(0.0f, LinearRampProgress - (DeltaTime / Settings.RampDownDuration));
		if (LinearRampProgress <= 0.0f)
		{
			EnterStoppedState();
		}
	}
}

void UAGSDVibrationComponent::BroadcastIntensityIfChanged()
{
	const float CurrentAlpha = GetCurrentIntensityAlpha();
	if (!FMath::IsNearlyEqual(CurrentAlpha, LastBroadcastIntensityAlpha, KINDA_SMALL_NUMBER))
	{
		LastBroadcastIntensityAlpha = CurrentAlpha;
		OnVibrationIntensityChanged.Broadcast(CurrentAlpha);
	}
}

void UAGSDVibrationComponent::EvaluateVibrationOffset(double TimeSeconds, float IntensityAlpha, FVector& OutLocationOffset, FRotator& OutRotationOffset) const
{
	if (IntensityAlpha <= KINDA_SMALL_NUMBER)
	{
		OutLocationOffset = FVector::ZeroVector;
		OutRotationOffset = FRotator::ZeroRotator;
		return;
	}

	float EffectiveIntensity = IntensityAlpha;

	// 엔진 맥동(Amplitude Modulation): 기계가 힘차게 덜컹거리며 숨 쉬는 듯한 강약 리듬 추가
	if (Settings.bEnableEnginePulse && Settings.PulseFrequencyHz > 0.0f && Settings.PulseDepth > 0.0f)
	{
		const double TwoPi = static_cast<double>(PI) * 2.0;
		const double PulsePhase = FMath::Fmod(TimeSeconds * static_cast<double>(Settings.PulseFrequencyHz) * TwoPi, TwoPi);
		const float PulseWave = 0.5f * (1.0f + static_cast<float>(FMath::Sin(PulsePhase)));
		const float PulseModulation = 1.0f - (FMath::Clamp(Settings.PulseDepth, 0.0f, 0.8f) * PulseWave);
		EffectiveIntensity *= PulseModulation;
	}

	// 각 축이 독립적으로 움직이도록 서로 다른 위상 및 노이즈 시드 부여
	const float SigLocX = ComputeAxisSignal(TimeSeconds, 0.00f, 11.3f);
	const float SigLocY = ComputeAxisSignal(TimeSeconds, 2.09f, 37.7f);
	const float SigLocZ = ComputeAxisSignal(TimeSeconds, 4.18f, 73.1f);

	const float SigPitch = ComputeAxisSignal(TimeSeconds, 1.05f, 109.5f);
	const float SigYaw   = ComputeAxisSignal(TimeSeconds, 3.14f, 149.9f);
	const float SigRoll  = ComputeAxisSignal(TimeSeconds, 5.23f, 193.3f);

	OutLocationOffset = FVector(
		Settings.PositionAmplitude.X * SigLocX * EffectiveIntensity,
		Settings.PositionAmplitude.Y * SigLocY * EffectiveIntensity,
		Settings.PositionAmplitude.Z * SigLocZ * EffectiveIntensity
	);

	OutRotationOffset = FRotator(
		Settings.RotationAmplitude.Pitch * SigPitch * EffectiveIntensity,
		Settings.RotationAmplitude.Yaw * SigYaw * EffectiveIntensity,
		Settings.RotationAmplitude.Roll * SigRoll * EffectiveIntensity
	);
}

float UAGSDVibrationComponent::ComputeAxisSignal(double TimeSeconds, float AxisPhaseOffset, float NoiseSeedOffset) const
{
	const double TwoPi = static_cast<double>(PI) * 2.0;
	const double BaseFreqRad = static_cast<double>(Settings.FrequencyHz) * TwoPi;

	const double PrimaryPhase = FMath::Fmod(TimeSeconds * BaseFreqRad + static_cast<double>(AxisPhaseOffset), TwoPi);
	const double SecondaryPhase = FMath::Fmod(
		TimeSeconds * BaseFreqRad * static_cast<double>(Settings.SecondaryFrequencyRatio) + static_cast<double>(AxisPhaseOffset * 1.37f),
		TwoPi);

	const float PrimaryWave = static_cast<float>(FMath::Sin(PrimaryPhase));
	const float SecondaryWave = static_cast<float>(FMath::Sin(SecondaryPhase));
	const float HarmonicSignal = (0.65f * PrimaryWave) + (0.35f * SecondaryWave);

	const float Blend = FMath::Clamp(Settings.NoiseBlend, 0.0f, 1.0f);
	if (Blend <= KINDA_SMALL_NUMBER)
	{
		return HarmonicSignal;
	}

	const float NoiseInput = static_cast<float>(FMath::Fmod(
		TimeSeconds * static_cast<double>(Settings.FrequencyHz) * 0.65 + static_cast<double>(NoiseSeedOffset),
		65536.0));
	const float NoiseSignal = FMath::Clamp(FMath::PerlinNoise1D(NoiseInput), -1.0f, 1.0f);

	return FMath::Lerp(HarmonicSignal, NoiseSignal, Blend);
}

void UAGSDVibrationComponent::ApplyOffsetsToTargets(const FVector& LocationOffset, const FRotator& RotationOffset)
{
	const FQuat OffsetQuat = RotationOffset.Quaternion();

	for (const FAGSDVibrationTargetEntry& Entry : TargetEntries)
	{
		USceneComponent* Target = Entry.TargetComponent.Get();
		if (!IsValid(Target) || !Entry.bHasSavedInitialTransform)
		{
			continue;
		}

		const FVector NewRelativeLocation = Entry.InitialRelativeLocation + LocationOffset;
		const FQuat NewRelativeQuat = Entry.InitialRelativeRotation.Quaternion() * OffsetQuat;
		Target->SetRelativeLocationAndRotation(NewRelativeLocation, NewRelativeQuat);
	}
}

bool UAGSDVibrationComponent::IsWithinActiveDistance() const
{
	if (MaxActiveDistance <= 0.0f)
	{
		return true;
	}

	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (!IsValid(Owner) || !IsValid(World))
	{
		return true;
	}

	FVector ReferenceLocation = FVector::ZeroVector;
	bool bHasReference = false;

	if (const APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(World, 0))
	{
		ReferenceLocation = CamMgr->GetCameraLocation();
		bHasReference = true;
	}
	else if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		ReferenceLocation = PlayerPawn->GetActorLocation();
		bHasReference = true;
	}

	if (!bHasReference)
	{
		return true;
	}

	const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), ReferenceLocation);
	return DistSq <= FMath::Square(MaxActiveDistance);
}

void UAGSDVibrationComponent::EnterStoppedState()
{
	LinearRampProgress = 0.0f;
	CurrentState = EAGSDVibrationState::Stopped;
	bSuspendedByDistance = false;

	ResetTargetsToInitialTransform();
	SetComponentTickEnabled(false);

	BroadcastIntensityIfChanged();
	OnVibrationStopped.Broadcast();
}
