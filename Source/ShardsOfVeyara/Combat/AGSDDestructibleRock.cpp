// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AGSDDestructibleRock.h"
#include "Components/StaticMeshComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"

AAGSDDestructibleRock::AAGSDDestructibleRock()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 루트 컴포넌트 생성
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 2. 바위 원본 스태틱 메시 컴포넌트 생성 (실제 바위 형태와 100% 일치하는 정밀 충돌 프록시)
	RockStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RockStaticMesh"));
	RockStaticMesh->SetupAttachment(SceneRoot);
	RockStaticMesh->SetCollisionProfileName(TEXT("BlockAll"));
	RockStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RockStaticMesh->SetHiddenInGame(true); // 비주얼은 GeometryCollection이 담당하므로 충돌만 활성화
	RockStaticMesh->SetCastShadow(false);

	// 3. 카오스 지오메트리 컬렉션 컴포넌트 생성 및 초기화 (비주얼 렌더링 및 폭발 시 파편화 담당)
	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollectionComponent"));
	GeometryCollectionComponent->SetupAttachment(SceneRoot);
	GeometryCollectionComponent->SetNotifyBreaks(false); // 불필요한 브레이크 델리게이트 브로드캐스트 부하 제거
	GeometryCollectionComponent->SetCollisionProfileName(TEXT("Custom"));
	GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ConfigureFractureCollision();

	// 평상시: 물리 OFF (CPU 부하 0, 제자리에 완벽 고정)
	GeometryCollectionComponent->SetSimulatePhysics(false);
	GeometryCollectionComponent->SetEnableGravity(false);
}

void AAGSDDestructibleRock::ConfigureFractureCollision()
{
	if (!GeometryCollectionComponent)
	{
		return;
	}

	// 1. 모든 충돌 채널을 기본 '무시(Ignore)'로 설정하여 파편 간/외부 오브젝트 간 충돌 쌍 연산 90% 이상 제거
	GeometryCollectionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

	// 2. 바닥 및 벽면 등 정적 지형(WorldStatic)에만 충돌(Block)하여 파편이 바닥에 자연스럽게 안착
	GeometryCollectionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
}

void AAGSDDestructibleRock::BeginPlay()
{
	Super::BeginPlay();
	bIsDestroyed = false;

	// 평상시: 바위 형태 스태틱 메시 콜리전 보장 (캐릭터 통과 완벽 차단)
	if (RockStaticMesh)
	{
		RockStaticMesh->SetCollisionProfileName(TEXT("BlockAll"));
		RockStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RockStaticMesh->SetHiddenInGame(true);
	}

	// 평상시: 지오메트리 컬렉션 물리 꺼둠 보장 및 최적화 충돌 채널 설정
	if (GeometryCollectionComponent)
	{
		GeometryCollectionComponent->SetSimulatePhysics(false);
		GeometryCollectionComponent->SetEnableGravity(false);
		GeometryCollectionComponent->SetCollisionProfileName(TEXT("Custom"));
		GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ConfigureFractureCollision();
	}
}

float AAGSDDestructibleRock::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	UE_LOG(LogTemp, Warning, TEXT("[DestructibleRock] TakeDamage: %f from %s"), ActualDamage, DamageCauser ? *DamageCauser->GetName() : TEXT("None"));

	if (!bIsDestroyed && ActualDamage >= DestructionDamageThreshold)
	{
		FVector HitLocation = GetActorLocation();

		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
			HitLocation = PointDamageEvent->HitInfo.ImpactPoint;
		}
		else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
		{
			const FRadialDamageEvent* RadialDamageEvent = static_cast<const FRadialDamageEvent*>(&DamageEvent);
			HitLocation = RadialDamageEvent->Origin;
		}

		TriggerDestruction(HitLocation);
	}

	return ActualDamage;
}

void AAGSDDestructibleRock::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	UE_LOG(LogTemp, Warning, TEXT("[DestructibleRock] ApplyDamage: %f from %s"), Damage, DamageCauser ? *DamageCauser->GetName() : TEXT("None"));

	if (!bIsDestroyed && Damage >= DestructionDamageThreshold)
	{
		TriggerDestruction(DamageLocation, DamageImpulse);
	}
}

void AAGSDDestructibleRock::HandleDeath()
{
	if (!bIsDestroyed)
	{
		TriggerDestruction(GetActorLocation());
	}
}

void AAGSDDestructibleRock::ApplyHealing(float Healing, AActor* Healer)
{
	// 바위는 체력 회복 대상이 아님
}

void AAGSDDestructibleRock::TriggerDestruction(const FVector& HitLocation, const FVector& Impulse)
{
	if (bIsDestroyed)
	{
		return;
	}

	bIsDestroyed = true;
	UE_LOG(LogTemp, Warning, TEXT("[DestructibleRock] TriggerDestruction at HitLocation: %s"), *HitLocation.ToString());

	// 1. 스태틱 메시 콜리전 즉시 해제 (통로 개방)
	if (RockStaticMesh)
	{
		RockStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 2. 카오스 지오메트리 컬렉션 물리 활성화 및 즉시 완전 파편화 (제자리 사방 폭파)
	if (GeometryCollectionComponent)
	{
		// 파편 충돌 최적화: 정적 지형(WorldStatic)에만 안착하고 파편들끼리는 충돌 무시
		ConfigureFractureCollision();

		// 중력 및 물리 활성화
		GeometryCollectionComponent->SetEnableGravity(true);
		GeometryCollectionComponent->SetSimulatePhysics(true);

		// 1) 지오메트리 컬렉션의 모든 활성 클러스터를 즉시 낱개 파편으로 분해
		GeometryCollectionComponent->CrumbleActiveClusters();

		const FVector RockCenter = GetActorLocation();

		// 2) 내부 스트레인을 바위 중심에 강력하게 가해 모든 연결 그래프 해체
		GeometryCollectionComponent->ApplyInternalStrain(0, RockCenter, DestructionImpulseRadius, 10, 1.0f, 10000000.0f);

		// 3) 충격 원점을 바위 중심(RockCenter)으로 설정하여, 한 방향으로 날아가지 않고 그 자리에서 사방으로 빵! 분산
		GeometryCollectionComponent->AddRadialImpulse(
			RockCenter,
			DestructionImpulseRadius,
			DestructionImpulse,
			ERadialImpulseFalloff::RIF_Constant,
			true
		);
	}

	// 3. 블루프린트 연출 훅 호출 (VFX, 사운드 등)
	OnRockDestroyed(HitLocation);

	// 4. 잔해 정리 타이머 설정 (지정된 시간 후 액터 소멸)
	if (CleanupDelaySeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			CleanupTimerHandle,
			this,
			&AAGSDDestructibleRock::OnCleanupTimerExpired,
			CleanupDelaySeconds,
			false
		);
	}
}

void AAGSDDestructibleRock::OnCleanupTimerExpired()
{
	UE_LOG(LogTemp, Warning, TEXT("[DestructibleRock] Cleanup delay reached. Destroying rock actor."));
	Destroy();
}
