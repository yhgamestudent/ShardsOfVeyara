#include "AGSDLockOnComponent.h"
#include "AGSDCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "MotionWarpingComponent.h"
#include "Interaction.h"

UAGSDLockOnComponent::UAGSDLockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 옵션 1 적용: 컴포넌트 틱은 꺼둡니다.
}

void UAGSDLockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AAGSDCharacter>(GetOwner());
}

void UAGSDLockOnComponent::UpdateLockOnState(float DeltaSeconds)
{
	if (!LockedTarget || !OwnerCharacter) return;

	bool bShouldRelease = false;

	// 1. 적이 유효한지 검사
	if (!IsValid(LockedTarget)) 
	{
		bShouldRelease = true;
	}
	else
	{
		// 2. 락온 유지 한계 거리 체크
		float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), LockedTarget->GetActorLocation());
		if (Distance > MaxLockOnDistance)
		{
			bShouldRelease = true;
		}
	}

	if (bShouldRelease)
	{
		ToggleLockOn();
		return;
	}

	// 3. 장애물 시야 차단 체크 (Line of Sight - Visibility 채널)
	UCameraComponent* FollowCamera = OwnerCharacter->GetFollowCamera();
	if (FollowCamera)
	{
		FVector TraceStart = FollowCamera->GetComponentLocation();
		
		float TargetHalfHeight = LockedTarget->GetSimpleCollisionHalfHeight();
		FVector TargetVisualCenter = LockedTarget->GetActorLocation();
		TargetVisualCenter.Z += (TargetHalfHeight > 0.0f) ? TargetHalfHeight : 50.0f;

		FVector TraceEnd = TargetVisualCenter;
		
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(OwnerCharacter);
		TraceParams.AddIgnoredActor(LockedTarget);

		FHitResult HitResult;
		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

		if (bHit && HitResult.GetActor())
		{
			AActor* HitActor = HitResult.GetActor();
			// 특정 상호작용 액터나 아이템 태그는 시야 차단에서 예외 처리
			if (HitActor->GetClass()->ImplementsInterface(UInteraction::StaticClass()) ||
				HitActor->ActorHasTag(FName("Item")) ||
				HitActor->ActorHasTag(FName("Interactable")))
			{
				bHit = false;
			}
		}

		if (bHit)
		{
			if (!bIsLineOfSightBlocked)
			{
				bIsLineOfSightBlocked = true;
				GetWorld()->GetTimerManager().SetTimer(
					LineOfSightTimerHandle, 
					this, 
					&UAGSDLockOnComponent::OnLineOfSightTimeout, 
					LineOfSightTimeoutDuration, 
					false
				);
			}
		}
		else
		{
			if (bIsLineOfSightBlocked)
			{
				bIsLineOfSightBlocked = false;
				GetWorld()->GetTimerManager().ClearTimer(LineOfSightTimerHandle);
			}
		}

		// 4. 카메라 회전 보간 처리 (옵션 1에 맞춰 캐릭터 컨트롤러 회전을 부드럽게 조정)
		APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
		if (PC && !bIsLineOfSightBlocked)
		{
			FVector CameraLocation = FollowCamera->GetComponentLocation();
			FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(CameraLocation, TargetVisualCenter);
			FRotator CurrentRotation = PC->GetControlRotation();
			
			TargetRotation.Pitch = CurrentRotation.Pitch;
			TargetRotation.Roll = 0.0f;

			FRotator SmoothedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 8.0f);
			PC->SetControlRotation(SmoothedRotation);
		}
	}
}

void UAGSDLockOnComponent::ToggleLockOn()
{
	if (!OwnerCharacter) return;

	if (LockedTarget)
	{
		// 1. 이미 락온 중이라면 조준선 끄고 락온 해제
		SetLockOnMarkerState(LockedTarget, false);
		LockedTarget = nullptr;

		// 시야 차단 타이머 및 상태 초기화
		GetWorld()->GetTimerManager().ClearTimer(LineOfSightTimerHandle);
		bIsLineOfSightBlocked = false;

		// 모션 워프 타겟 제거
		UMotionWarpingComponent* MotionWarping = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>();
		if (MotionWarping)
		{
			MotionWarping->RemoveWarpTarget(FName("WarpTarget"));
		}
	}
	else
	{
		// Spear를 들고 있을 때만 LockOn 가능
		if (OwnerCharacter->HoldingWeapon != EHoldingWeapon::Spear)
		{
			return;
		}

		// 2. 락온 중이 아니라면 주변의 가장 가까운 적 탐색
		LockedTarget = FindNearestLockOnTarget();
		if (LockedTarget)
		{
			SetLockOnMarkerState(LockedTarget, true);
		}
	}

	// 캐릭터의 락온 상태 변경 델리게이트 브로드캐스트 호출
	OwnerCharacter->OnLockOnStateChanged.Broadcast(LockedTarget != nullptr);
}

AActor* UAGSDLockOnComponent::FindNearestLockOnTarget()
{
	if (!OwnerCharacter) return nullptr;

	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	UKismetSystemLibrary::SphereOverlapActors(
		OwnerCharacter,
		OwnerCharacter->GetActorLocation(),
		LockOnRadius,
		{ UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn) },
		AActor::StaticClass(),
		ActorsToIgnore,
		OverlappingActors
	);

	AActor* BestTarget = nullptr;
	float BestScore = FLT_MAX;

	UCameraComponent* FollowCamera = OwnerCharacter->GetFollowCamera();
	if (!FollowCamera) return nullptr;

	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector CameraForward = FollowCamera->GetForwardVector();

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && Actor->ActorHasTag(FName("Enemy")))
		{
			float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Actor->GetActorLocation());
			if (Distance > LockOnRadius) continue;

			FVector DirToTarget = (Actor->GetActorLocation() - CameraLocation).GetSafeNormal();
			float Dot = FVector::DotProduct(CameraForward, DirToTarget);
			float AngleOffset = FMath::RadiansToDegrees(FMath::Acos(Dot));

			if (AngleOffset <= 60.0f) // 시야각 120도 이내 필터링
			{
				float NormDistance = Distance / LockOnRadius;
				float NormAngle = AngleOffset / 60.0f;
				float Score = (NormDistance * 1.0f) + (NormAngle * 1.0f);

				if (Score < BestScore)
				{
					BestScore = Score;
					BestTarget = Actor;
				}
			}
		}
	}
	return BestTarget;
}

void UAGSDLockOnComponent::SwitchTargetLeft()
{
	SwitchTarget(true);
}

void UAGSDLockOnComponent::SwitchTargetRight()
{
	SwitchTarget(false);
}

void UAGSDLockOnComponent::SwitchTarget(bool bLookLeft)
{
	if (!LockedTarget || !OwnerCharacter) return;

	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	UKismetSystemLibrary::SphereOverlapActors(
		OwnerCharacter,
		OwnerCharacter->GetActorLocation(),
		LockOnRadius,
		{ UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn) },
		AActor::StaticClass(),
		ActorsToIgnore,
		OverlappingActors
	);

	AActor* NewTarget = nullptr;
	float MinYDiff = FLT_MAX;

	UCameraComponent* FollowCamera = OwnerCharacter->GetFollowCamera();
	if (!FollowCamera) return;

	FTransform CameraTransform = FollowCamera->GetComponentTransform();
	FVector CurrentTargetLocal = CameraTransform.InverseTransformPosition(LockedTarget->GetActorLocation());

	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector CameraForward = FollowCamera->GetForwardVector();

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && Actor->ActorHasTag(FName("Enemy")) && Actor != LockedTarget)
		{
			FVector DirToTarget = (Actor->GetActorLocation() - CameraLocation).GetSafeNormal();
			float Dot = FVector::DotProduct(CameraForward, DirToTarget);
			float AngleOffset = FMath::RadiansToDegrees(FMath::Acos(Dot));

			if (AngleOffset > 60.0f) continue;

			FVector EnemyLocal = CameraTransform.InverseTransformPosition(Actor->GetActorLocation());
			float YDiff = EnemyLocal.Y - CurrentTargetLocal.Y;

			if (bLookLeft)
			{
				if (YDiff < 0.0f)
				{
					float AbsDiff = FMath::Abs(YDiff);
					if (AbsDiff < MinYDiff)
					{
						MinYDiff = AbsDiff;
						NewTarget = Actor;
					}
				}
			}
			else
			{
				if (YDiff > 0.0f)
				{
					float AbsDiff = FMath::Abs(YDiff);
					if (AbsDiff < MinYDiff)
					{
						MinYDiff = AbsDiff;
						NewTarget = Actor;
					}
				}
			}
		}
	}

	if (NewTarget)
	{
		SetLockOnMarkerState(LockedTarget, false);
		LockedTarget = NewTarget;
		SetLockOnMarkerState(LockedTarget, true);

		GetWorld()->GetTimerManager().ClearTimer(LineOfSightTimerHandle);
		bIsLineOfSightBlocked = false;

		OwnerCharacter->OnLockOnStateChanged.Broadcast(true);
	}
}

void UAGSDLockOnComponent::OnLineOfSightTimeout()
{
	if (LockedTarget && bIsLineOfSightBlocked)
	{
		ToggleLockOn();
	}
}

void UAGSDLockOnComponent::SetLockOnMarkerState(AActor* TargetActor, bool bActive)
{
	if (!TargetActor) return;

	TArray<UWidgetComponent*> WidgetComps;
	TargetActor->GetComponents<UWidgetComponent>(WidgetComps);

	for (UWidgetComponent* Comp : WidgetComps)
	{
		if (Comp && Comp->ComponentHasTag(FName("LockOnMarker")))
		{
			Comp->SetVisibility(bActive);

			UUserWidget* UserWidget = Comp->GetUserWidgetObject();
			if (UserWidget)
			{
				UFunction* AnimFunc = UserWidget->FindFunction(FName("PlayLockOnAnim"));
				if (AnimFunc)
				{
					struct FPlayLockOnAnimArgs
					{
						bool bPlay;
					};
					FPlayLockOnAnimArgs Args;
					Args.bPlay = bActive;
					
					UserWidget->ProcessEvent(AnimFunc, &Args);
				}
			}
			break;
		}
	}
}
