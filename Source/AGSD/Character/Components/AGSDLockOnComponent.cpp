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
#include "Components/CapsuleComponent.h"
#include "Interaction.h"
#include "DrawDebugHelpers.h"

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
	if (!OwnerCharacter) return;

	// 하드 락온 상태일 때: 기존 락온 유지/해제 및 카메라 보간 처리
	if (LockedTarget)
	{
		CurrentSoftLockTarget = nullptr; // 하드 락온 중에는 소프트 락온 타겟 비활성화

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
		return;
	}

	// 비락온 상태일 때: 매 프레임(Tick) 소프트 락온 실시간 감지 및 디버그 시각화
	UpdateSoftLockState(DeltaSeconds);
}

void UAGSDLockOnComponent::UpdateSoftLockState(float DeltaSeconds)
{
	if (!bEnableSoftLockOn || !OwnerCharacter)
	{
		CurrentSoftLockTarget = nullptr;
		return;
	}

	// 매 프레임(Tick) 시야각 및 사거리 내 최적의 소프트 락온 대상 실시간 탐색
	CurrentSoftLockTarget = FindSoftLockTarget();

	// 디버그 시각화 (화면 상단 디버그 텍스트 및 3D 셰이프)
	if (bShowSoftLockDebug)
	{
		if (CurrentSoftLockTarget)
		{
			FVector PlayerLoc = OwnerCharacter->GetActorLocation();
			FVector TargetLoc = CurrentSoftLockTarget->GetActorLocation();
			float Dist = FVector::Dist2D(PlayerLoc, TargetLoc);

			// 카메라 기준 수평 각도 계산
			UCameraComponent* FollowCamera = OwnerCharacter->GetFollowCamera();
			APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
			FRotator ViewRot = PC ? PC->GetControlRotation() : (FollowCamera ? FollowCamera->GetComponentRotation() : OwnerCharacter->GetActorRotation());
			FVector CameraForward2D = FRotationMatrix(FRotator(0.f, ViewRot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
			FVector DirToTarget2D = (TargetLoc - PlayerLoc).GetSafeNormal2D();
			float Dot = FVector::DotProduct(CameraForward2D, DirToTarget2D);
			float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));

			// 1) 화면 상단 디버그 텍스트 (Key 7777로 고정하여 화면 깜빡임 없이 매 프레임 제자리 갱신)
			if (GEngine)
			{
				FString Msg = FString::Printf(TEXT("[SoftLock] Target: %s | Dist: %.0f / %.0f | Angle: %.1f° / %.1f°"),
					*CurrentSoftLockTarget->GetName(), Dist, SoftLockRadius, Angle, SoftLockMaxAngle);
				GEngine->AddOnScreenDebugMessage(7777, 0.05f, FColor::Emerald, Msg);
			}

			// 2) 3D 디버그 셰이프 (적 위치에 와이어프레임 구체 및 카메라-타겟 간 연결 가이드 라인)
			if (UWorld* World = GetWorld())
			{
				FVector Origin, BoxExtent;
				CurrentSoftLockTarget->GetActorBounds(true, Origin, BoxExtent);
				FVector SphereLoc = (BoxExtent.Z > 10.0f) ? (Origin + FVector(0.f, 0.f, BoxExtent.Z * 0.5f)) : (TargetLoc + FVector(0.f, 0.f, 80.f));

				// 적 머리/중심 위에 에메랄드색 구체 마커
				DrawDebugSphere(World, SphereLoc, 35.0f, 16, FColor::Emerald, false, -1.0f, 0, 2.0f);

				// 플레이어 카메라에서 적 중심으로 뻗어나가는 가이드 조준선
				FVector CamLoc = FollowCamera ? FollowCamera->GetComponentLocation() : PlayerLoc;
				DrawDebugLine(World, CamLoc, SphereLoc, FColor(0, 255, 128), false, -1.0f, 0, 1.2f);
			}
		}
		else
		{
			// 시야각 내에 타겟이 없을 때
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(7777, 0.05f, FColor(160, 160, 160), TEXT("[SoftLock] No Target in View Angle"));
			}
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

	TArray<AActor*> CandidateActors;
	UGameplayStatics::GetAllActorsWithTag(OwnerCharacter, FName("Enemy"), CandidateActors);

	AActor* BestTarget = nullptr;
	float BestScore = FLT_MAX;

	UCameraComponent* FollowCamera = OwnerCharacter->GetFollowCamera();
	if (!FollowCamera) return nullptr;

	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector CameraForward = FollowCamera->GetForwardVector();

	for (AActor* Actor : CandidateActors)
	{
		if (Actor && Actor != OwnerCharacter)
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

	TArray<AActor*> CandidateActors;
	UGameplayStatics::GetAllActorsWithTag(OwnerCharacter, FName("Enemy"), CandidateActors);

	AActor* NewTarget = nullptr;
	float MinYDiff = FLT_MAX;

	UCameraComponent* FollowCamera = OwnerCharacter->GetFollowCamera();
	if (!FollowCamera) return;

	FTransform CameraTransform = FollowCamera->GetComponentTransform();
	FVector CurrentTargetLocal = CameraTransform.InverseTransformPosition(LockedTarget->GetActorLocation());

	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector CameraForward = FollowCamera->GetForwardVector();

	for (AActor* Actor : CandidateActors)
	{
		if (Actor && Actor != OwnerCharacter && Actor != LockedTarget)
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

AActor* UAGSDLockOnComponent::GetTargetForAttack(bool& bOutIsHardLocked)
{
	if (LockedTarget && IsValid(LockedTarget))
	{
		bOutIsHardLocked = true;
		return LockedTarget;
	}

	bOutIsHardLocked = false;

	if (bEnableSoftLockOn)
	{
		return FindSoftLockTarget();
	}

	return nullptr;
}

AActor* UAGSDLockOnComponent::FindSoftLockTarget()
{
	if (!OwnerCharacter) return nullptr;

	UCameraComponent* FollowCamera = OwnerCharacter->GetFollowCamera();
	if (!FollowCamera) return nullptr;

	const FVector PlayerLoc = OwnerCharacter->GetActorLocation();
	const FVector CameraLocation = FollowCamera->GetComponentLocation();
	const FVector CameraForward = FollowCamera->GetForwardVector();

	// 폰(Pawn) 오브젝트 타입 제한을 제거하고, "Enemy" 태그를 가진 모든 액터를 탐색
	TArray<AActor*> CandidateActors;
	UGameplayStatics::GetAllActorsWithTag(OwnerCharacter, FName("Enemy"), CandidateActors);

	AActor* BestTarget = nullptr;
	float BestScore = FLT_MAX;

	for (AActor* Actor : CandidateActors)
	{
		if (!Actor || Actor == OwnerCharacter) continue;

		// 적 액터가 유효하고 콜리전이 활성화되어 있는지 확인 (사망한 적 제외)
		ACharacter* EnemyChar = Cast<ACharacter>(Actor);
		if (EnemyChar && EnemyChar->GetCapsuleComponent())
		{
			if (EnemyChar->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}
		}

		// 플레이어와의 수평 거리 검사
		float Distance = FVector::Dist2D(PlayerLoc, Actor->GetActorLocation());
		if (Distance > SoftLockRadius) continue;

		// 카메라 정면 기준 수평(2D) 각도 검사 (플레이어가 마우스로 회전한 방향과 정확히 일치하도록 계산)
		APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
		FRotator ViewRot = PC ? PC->GetControlRotation() : FollowCamera->GetComponentRotation();
		FVector CameraForward2D = FRotationMatrix(FRotator(0.f, ViewRot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
		FVector DirToTarget2D = (Actor->GetActorLocation() - PlayerLoc).GetSafeNormal2D();
		float Dot = FVector::DotProduct(CameraForward2D, DirToTarget2D);
		float AngleOffset = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));

		if (AngleOffset > SoftLockMaxAngle) continue;

		// 시야 차폐 검사 (장애물 뒤에 가려진 적 제외)
		FVector TraceStart = CameraLocation;
		FVector TargetCenter = Actor->GetActorLocation();

		// 액터의 실제 3D 콜리전/메시 바운딩 박스를 통해 지오메트리 중심점(Origin) 계산
		// 피벗이 땅바닥에 있는 허수아비나 일반 액터도 바닥 지형 충돌 없이 몸통 중심으로 시선 레이 발사
		FVector Origin, BoxExtent;
		Actor->GetActorBounds(true, Origin, BoxExtent);
		if (BoxExtent.Z > 10.0f)
		{
			TargetCenter = Origin;
		}
		else if (EnemyChar)
		{
			float HalfHeight = EnemyChar->GetSimpleCollisionHalfHeight();
			TargetCenter.Z += (HalfHeight > 0.0f) ? HalfHeight * 0.5f : 40.0f;
		}
		else
		{
			TargetCenter.Z += 60.0f; // 기본 높이 보정
		}

		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(OwnerCharacter);
		TraceParams.AddIgnoredActor(Actor);

		FHitResult HitResult;
		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TargetCenter, ECC_Visibility, TraceParams);
		if (bHit && HitResult.GetActor())
		{
			AActor* HitActor = HitResult.GetActor();
			// 상호작용 가능한 아이템 등은 시야 차단에서 예외 처리
			if (HitActor->GetClass()->ImplementsInterface(UInteraction::StaticClass()) ||
				HitActor->ActorHasTag(FName("Item")) ||
				HitActor->ActorHasTag(FName("Interactable")))
			{
				bHit = false;
			}
		}

		if (bHit)
		{
			// 벽/장애물에 가려진 경우 타겟에서 제외
			continue;
		}

		// 스코어링 (각도 정규화 점수 + 거리 정규화 점수)
		float NormDistance = Distance / SoftLockRadius;
		float NormAngle = AngleOffset / SoftLockMaxAngle;
		float Score = (NormDistance * 1.0f) + (NormAngle * 1.0f);

		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Actor;
		}
	}

	return BestTarget;
}

