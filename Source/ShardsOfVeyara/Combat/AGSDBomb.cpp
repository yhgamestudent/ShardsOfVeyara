// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AGSDBomb.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Character/AGSDCharacter.h"
#include "Character/Components/AGSDInteractionComponent.h"
#include "AGSDPlayerController.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"
#include "Variant_Combat/Interfaces/CombatDamageable.h"
#include "Engine/OverlapResult.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EngineUtils.h"
#include "Camera/CameraShakeBase.h"

AAGSDBomb::AAGSDBomb()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 폭탄 스태틱 메시 컴포넌트 (루트 컴포넌트로 승격 및 물리 시뮬레이션 설정)
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	SetRootComponent(BombMesh);
	BombMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	BombMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BombMesh->SetSimulatePhysics(true);
	BombMesh->SetEnableGravity(true);
	BombMesh->SetNotifyRigidBodyCollision(true);

	// 2. 상호작용 감지용 구체 콜리전 (BombMesh에 부착되어 물리 이동 추종)
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(BombMesh);
	InteractionSphere->SetSphereRadius(150.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetGenerateOverlapEvents(true);

	// 3. 도화선 불빛 포인트 라이트 (초기에는 꺼져 있음)
	FuseLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FuseLight"));
	FuseLight->SetupAttachment(BombMesh);
	FuseLight->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	FuseLight->SetLightColor(FLinearColor(1.0f, 0.4f, 0.05f));
	FuseLight->SetIntensity(2000.0f);
	FuseLight->SetAttenuationRadius(250.0f);
	FuseLight->SetVisibility(false);
}

void AAGSDBomb::BeginPlay()
{
	Super::BeginPlay();

	bIsIgnited = false;

	// 물리 시뮬레이션 활성화 보장
	if (BombMesh)
	{
		BombMesh->SetSimulatePhysics(true);
		BombMesh->SetEnableGravity(true);
	}

	// 상호작용 오버랩 델리게이트 바인딩
	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AAGSDBomb::OnInteractionSphereBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AAGSDBomb::OnInteractionSphereEndOverlap);
	}
}

void AAGSDBomb::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAGSDCharacter* Player = Cast<AAGSDCharacter>(OtherActor))
	{
		if (UAGSDInteractionComponent* InterComp = Player->FindComponentByClass<UAGSDInteractionComponent>())
		{
			InterComp->AddInteractableActor(this);
		}
	}
}

void AAGSDBomb::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAGSDCharacter* Player = Cast<AAGSDCharacter>(OtherActor))
	{
		if (UAGSDInteractionComponent* InterComp = Player->FindComponentByClass<UAGSDInteractionComponent>())
		{
			InterComp->RemoveInteractableActor(this);
		}
	}
}

void AAGSDBomb::Interact_Implementation(AAGSDCharacter* player)
{
	if (!bIsIgnited)
	{
		Ignite();
	}
}

void AAGSDBomb::ShowWidget_Implementation(ACharacter* player)
{
	if (bIsIgnited || !player)
	{
		return;
	}

	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
	{
		PlayerController->ShowInteractionWidget(InteractActionText);
	}
}

bool AAGSDBomb::CanInteract_Implementation(AAGSDCharacter* player)
{
	return !bIsIgnited;
}

void AAGSDBomb::Ignite()
{
	if (bIsIgnited)
	{
		return;
	}

	bIsIgnited = true;

	// 점화 시 위젯 숨김 (이미 점화되었으므로 더 이상 점화 안내 불필요)
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (AAGSDPlayerController* AGSDPC = Cast<AAGSDPlayerController>(PC))
			{
				AGSDPC->HideInteractionWidget();
			}
		}
	}

	// 도화선 불빛 활성화 및 깜빡임 효과 시작
	if (FuseLight)
	{
		FuseLight->SetVisibility(true);
		GetWorldTimerManager().SetTimer(FlickerTimerHandle, this, &AAGSDBomb::ToggleFuseFlicker, 0.12f, true);
	}

	// 도화선 사운드 재생
	if (FuseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FuseSound, GetActorLocation());
	}

	// 블루프린트 점화 훅 호출
	OnBombIgnited();

	// 지정된 시간(FuseTime) 후 폭발 실행
	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AAGSDBomb::Explode, FuseTime, false);
}

void AAGSDBomb::ToggleFuseFlicker()
{
	if (FuseLight)
	{
		FuseLight->ToggleVisibility();
	}
}

bool AAGSDBomb::GetExplosionReaction(const FVector& TargetLocation, bool& bOutKnockdown, float& OutHitIntensity, FVector& OutLaunchVelocity) const
{
	bOutKnockdown = false;
	OutHitIntensity = 0.0f;
	OutLaunchVelocity = FVector::ZeroVector;

	const FVector Center = ExplosionCenter.IsZero() ? GetActorLocation() : ExplosionCenter;
	const float Dist = FVector::Dist(Center, TargetLocation);
	if (Dist > ExplosionRadius)
	{
		return false;
	}

	const float SafeKnockdownRadius = FMath::Clamp(KnockdownRadius, 0.0f, ExplosionRadius);

	// 1. 근거리: 에어본 넉다운 + 폭발 반대 방향으로 날려보냄
	if (Dist <= SafeKnockdownRadius)
	{
		bOutKnockdown = true;
		OutHitIntensity = 1.0f;

		FVector AwayDir = (TargetLocation - Center).GetSafeNormal2D();
		if (AwayDir.IsNearlyZero())
		{
			AwayDir = -GetActorForwardVector().GetSafeNormal2D();
		}
		OutLaunchVelocity = AwayDir * KnockdownLaunchHorizontalSpeed + FVector(0.0f, 0.0f, KnockdownLaunchVerticalSpeed);
		return true;
	}

	// 2. 중/원거리: 일반 피격, 넉다운 경계(1.0) → 폭발 외곽(0.0) 선형 보간
	const float Range = ExplosionRadius - SafeKnockdownRadius;
	const float T = (Range > KINDA_SMALL_NUMBER) ? (Dist - SafeKnockdownRadius) / Range : 1.0f;
	OutHitIntensity = 1.0f - FMath::Clamp(T, 0.0f, 1.0f);
	return true;
}

void AAGSDBomb::Explode()
{
	// 타이머들 정리
	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	GetWorldTimerManager().ClearTimer(FlickerTimerHandle);

	// 바닥 굴곡에 의한 시야 차단(Line-of-Sight) 방지를 위해 폭탄 중심 높이에서 폭발 판정 실행
	const FVector BombLocation = GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
	ExplosionCenter = BombLocation; // 플레이어 피격 반응(GetExplosionReaction) 계산 기준점

	// 1. 언리얼 표준 방사형 데미지 적용
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	UGameplayStatics::ApplyRadialDamageWithFalloff(
		this,
		BaseDamage,
		MinimumDamage,
		BombLocation,
		ExplosionRadius * 0.2f, // 내부 최대 데미지 반경
		ExplosionRadius,        // 외부 최소 데미지 반경
		DamageFalloff,
		UDamageType::StaticClass(),
		IgnoreActors,
		this,
		GetInstigatorController(),
		ECC_Visibility
	);

	// 2. 이중 안전망: 물리 엔진 쿼리 누락을 원천 배제하고 반경 내 대상을 직접 순회하여 ICombatDamageable 전달
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* TargetActor = *It;
		if (!TargetActor || TargetActor == this || IgnoreActors.Contains(TargetActor))
		{
			continue;
		}

		const float Dist = FVector::Dist(BombLocation, TargetActor->GetActorLocation());
		if (Dist <= ExplosionRadius)
		{
			if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(TargetActor))
			{
				const float Alpha = FMath::Clamp(Dist / ExplosionRadius, 0.0f, 1.0f);
				const float FalloffDamage = FMath::Lerp(BaseDamage, MinimumDamage, Alpha);
				const FVector ImpulseDir = (TargetActor->GetActorLocation() - BombLocation).GetSafeNormal();

				UE_LOG(LogTemp, Warning, TEXT("[Bomb] Delivering direct ICombatDamageable damage %f (Dist: %.1f) to %s"), FalloffDamage, Dist, *TargetActor->GetName());
				Damageable->ApplyDamage(FalloffDamage, this, BombLocation, ImpulseDir * ExplosionImpulse);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Bomb] Exploded at %s with radius %f and damage %f"), *BombLocation.ToString(), ExplosionRadius, BaseDamage);

	// 2. 폭발 나이아가라 이펙트 스폰
	if (ExplosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionVFX,
			BombLocation,
			FRotator::ZeroRotator,
			FVector(1.0f)
		);
	}

	// 3. 폭발 사운드 재생
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, BombLocation);
	}

	// 4. 월드 카메라 쉐이크 재생 (거리 기반 감쇄 적용)
	if (ExplosionCameraShakeClass)
	{
		const float EffectiveInnerRadius = (CameraShakeInnerRadius > 0.0f) ? CameraShakeInnerRadius : ExplosionRadius;
		UGameplayStatics::PlayWorldCameraShake(
			this,
			ExplosionCameraShakeClass,
			BombLocation,
			EffectiveInnerRadius,
			CameraShakeOuterRadius,
			CameraShakeFalloff
		);
	}

	// 5. 블루프린트 연출 훅 호출
	OnBombExploded();

	// 6. 폭탄 액터 소멸
	Destroy();
}

