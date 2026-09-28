#include "Component/PetGuideComponent.h"
#include "BaseFlyingPet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetMathLibrary.h"

UPetGuideComponent::UPetGuideComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UPetGuideComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPet = Cast<ABaseFlyingPet>(GetOwner());
	if (OwnerPet)
	{
		// 느낌표 위젯 컴포넌트가 이미 부착되어 있는지 확인하고, 없으면 동적으로 생성
		ExclamationWidgetComp = OwnerPet->FindComponentByClass<UWidgetComponent>();
		if (!ExclamationWidgetComp)
		{
			ExclamationWidgetComp = NewObject<UWidgetComponent>(OwnerPet, TEXT("GuideExclamationWidget"));
			if (ExclamationWidgetComp)
			{
				ExclamationWidgetComp->RegisterComponent();
				ExclamationWidgetComp->AttachToComponent(OwnerPet->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				ExclamationWidgetComp->SetRelativeLocation(ExclamationMarkerOffset);
				ExclamationWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
				ExclamationWidgetComp->SetDrawAtDesiredSize(true);
				ExclamationWidgetComp->SetVisibility(false);
			}
		}
		else
		{
			ExclamationWidgetComp->SetVisibility(false);
		}
	}
}

void UPetGuideComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsGuiding || !OwnerPet)
	{
		return;
	}

	// 1. 목표 액터가 있는 경우: OwnerPet->FollowingTarget()이 플레이어 추적과 동일한 이동/보간을 전담합니다.
	if (TargetActor.IsValid())
	{
		if (!bHasArrived)
		{
			FVector GoalLoc = TargetActor->GetActorLocation();
			float Distance = FVector::Dist(OwnerPet->GetActorLocation(), GoalLoc);

			// 도착 판정 (도착 반경 도달 시 느낌표 노출 및 이벤트 통지)
			float EffectiveArrivalRadius = FMath::Max(CurrentArrivalRadius, OwnerPet->FollowSettings.Distance + 80.0f);
			if (Distance <= EffectiveArrivalRadius)
			{
				bHasArrived = true;
				SetExclamationMarkVisible(true);
				OnReachedWaypoint.Broadcast();
			}
		}
		// 도착 후에도 OwnerPet이 FollowingTarget으로 퀘스트 액터 주변 위치를 계속 실시간 유지합니다.
		return;
	}

	// 2. 특정 액터가 아닌 고정 3D 좌표(MoveToLocation)로 안내하는 경우의 보간 이동
	FVector CurrentLoc = OwnerPet->GetActorLocation();

	if (!bHasArrived)
	{
		float Distance = FVector::Dist(CurrentLoc, TargetLocation);

		// 도착 판정
		if (Distance <= CurrentArrivalRadius)
		{
			bHasArrived = true;
			BaseArrivalLocation = TargetLocation;
			HoverTime = 0.0f;

			SetExclamationMarkVisible(true);
			OnReachedWaypoint.Broadcast();
		}
		else
		{
			// 가속/감속 유영 비행 보간
			float MoveInterp = OwnerPet ? OwnerPet->MoveInterpSpeed : 1.0f;
			float MoveSpeedMultiplier = 1.0f + (Distance * 0.040f);
			float FinalInterpSpeed = MoveInterp * MoveSpeedMultiplier;

			FVector NextLoc = FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime, FinalInterpSpeed);

			// 이동 방향으로 부드럽게 회전
			FVector Direction = (TargetLocation - CurrentLoc).GetSafeNormal();
			if (!Direction.IsNearlyZero())
			{
				FRotator TargetRot = Direction.Rotation();
				FRotator NewRot = FMath::RInterpTo(OwnerPet->GetActorRotation(), TargetRot, DeltaTime, RotationInterpSpeed);

				FHitResult Hit;
				OwnerPet->SetActorLocationAndRotation(NextLoc, NewRot, true, &Hit);

				if (Hit.IsValidBlockingHit())
				{
					FVector RemainingDelta = NextLoc - Hit.Location;
					FVector SlideDelta = FVector::VectorPlaneProject(RemainingDelta, Hit.Normal);
					OwnerPet->AddActorWorldOffset(SlideDelta, true);
				}
			}
		}
	}
	else
	{
		// 도착 후 제자리 호버링 (부유 연출)
		HoverTime += DeltaTime;
		float ZOffset = FMath::Sin(HoverTime * HoverFrequency) * HoverAmplitude;
		FVector HoverLoc = BaseArrivalLocation + FVector(0.f, 0.f, ZOffset);
		OwnerPet->SetActorLocation(HoverLoc, false);
	}
}

void UPetGuideComponent::MoveToLocation(const FVector& NewTargetLocation, float ArrivalRadius)
{
	float UpOffset = (OwnerPet && OwnerPet->FollowSettings.UpOffset > 10.0f) ? OwnerPet->FollowSettings.UpOffset : 120.0f;
	TargetLocation = NewTargetLocation + FVector(0.f, 0.f, UpOffset);
	TargetActor = nullptr;
	CurrentArrivalRadius = ArrivalRadius;
	bIsGuiding = true;
	bHasArrived = false;

	SetExclamationMarkVisible(false);

	if (OwnerPet)
	{
		OwnerPet->SetFreeRoaming(false);
	}
}

void UPetGuideComponent::MoveToActor(AActor* GoalActor, FVector Offset, float ArrivalRadius)
{
	TargetActor = GoalActor;
	TargetOffset = Offset;
	CurrentArrivalRadius = ArrivalRadius;
	bIsGuiding = true;
	bHasArrived = false;

	SetExclamationMarkVisible(false);

	if (OwnerPet && GoalActor)
	{
		// 플레이어 추적(FollowingTarget)과 완전히 동일한 보간 이동을 수행하도록 TargetActor 전환
		OwnerPet->SetTargetActor(GoalActor);
	}
}

void UPetGuideComponent::TeleportToLocation(const FVector& NewTargetLocation)
{
	float UpOffset = (OwnerPet && OwnerPet->FollowSettings.UpOffset > 10.0f) ? OwnerPet->FollowSettings.UpOffset : 120.0f;
	FVector AdjustedLoc = NewTargetLocation + FVector(0.f, 0.f, UpOffset);

	TargetLocation = AdjustedLoc;
	BaseArrivalLocation = AdjustedLoc;
	TargetActor = nullptr;
	bIsGuiding = true;
	bHasArrived = true;
	HoverTime = 0.0f;

	if (OwnerPet)
	{
		OwnerPet->SetActorLocation(AdjustedLoc);
		OwnerPet->SetFreeRoaming(false);
	}

	SetExclamationMarkVisible(true);
	OnReachedWaypoint.Broadcast();
}

void UPetGuideComponent::TeleportToActor(AActor* GoalActor)
{
	if (!GoalActor)
	{
		return;
	}

	TargetActor = GoalActor;
	bIsGuiding = true;
	bHasArrived = true;
	HoverTime = 0.0f;

	if (OwnerPet)
	{
		// 펫 코드에 있는 순간이동 로직(이펙트 + 물리 텔레포트 + 지면 높이 1m + 플레이어 마주보기)을 실행
		OwnerPet->TeleportToTargetActor(GoalActor);
	}

	SetExclamationMarkVisible(true);
	OnReachedWaypoint.Broadcast();
}

void UPetGuideComponent::ReturnToFollow()
{
	bIsGuiding = false;
	bHasArrived = false;
	TargetActor = nullptr;
	TargetLocation = FVector::ZeroVector;

	SetExclamationMarkVisible(false);

	if (OwnerPet)
	{
		// 원래 주인인 플레이어로 추적 대상 복귀
		OwnerPet->ReturnToPlayer();
	}
}

void UPetGuideComponent::SetPetMeshHidden(bool bHidden)
{
	if (OwnerPet && OwnerPet->MeshComp)
	{
		OwnerPet->MeshComp->SetVisibility(!bHidden, true);
	}
}

void UPetGuideComponent::SetExclamationMarkVisible(bool bVisible)
{
	if (ExclamationWidgetComp)
	{
		ExclamationWidgetComp->SetVisibility(bVisible);
	}
}

FVector UPetGuideComponent::GetCurrentTargetLocation() const
{
	if (TargetActor.IsValid())
	{
		return TargetActor->GetActorLocation() + TargetOffset;
	}
	return TargetLocation;
}
