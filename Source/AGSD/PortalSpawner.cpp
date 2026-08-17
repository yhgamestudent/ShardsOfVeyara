// Fill out your copyright notice in the Description page of Project Settings.

#include "PortalSpawner.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

APortalSpawner::APortalSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APortalSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (TargetBoss)
	{
		// 보스가 파괴(Destroy)될 때를 감지하기 위해 OnDestroyed 이벤트에 바인딩합니다.
		TargetBoss->OnDestroyed.AddDynamic(this, &APortalSpawner::OnBossDestroyed);
	}
}

void APortalSpawner::OnBossDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor || !PortalClass) return;

	// 1. 보스의 위치와 정면 방향을 가져옵니다.
	FVector BossLocation = DestroyedActor->GetActorLocation();
	FVector BossForward = DestroyedActor->GetActorForwardVector();
	FRotator BossRotation = DestroyedActor->GetActorRotation();

	// 2. 보스의 뒤쪽 지점을 계산합니다.
	FVector TargetLocation = BossLocation - (BossForward * SpawnOffsetDistance);

	// 3. 해당 지점에서 바닥을 찾기 위해 위에서 아래로 라인 트레이스를 수행합니다.
	FHitResult HitResult;
	FVector StartTrace = TargetLocation + FVector(0.f, 0.f, 500.f);
	FVector EndTrace = TargetLocation - FVector(0.f, 0.f, 500.f);
	
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(DestroyedActor);
	TraceParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, TraceParams);

	FVector FinalLocation;
	if (bHit)
	{
		// 4. 바닥을 찾았다면 해당 위치를 사용합니다.
		FinalLocation = HitResult.Location;
	}
	else
	{
		// 5. 바닥을 찾지 못했다면(낭떠러지 등) 내비 메시를 이용해 근처 유효한 위치를 찾습니다.
		UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
		if (NavSystem)
		{
			FNavLocation NavLocation;
			if (NavSystem->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
			{
				FinalLocation = NavLocation.Location;
			}
			else
			{
				// 최후의 수단으로 원래 계산된 지점을 사용합니다.
				FinalLocation = TargetLocation;
			}
		}
		else
		{
			FinalLocation = TargetLocation;
		}
	}

	// 6. 포털 소환
	SpawnPortal(FinalLocation, BossRotation);
}

void APortalSpawner::SpawnPortal(const FVector& BaseLocation, const FRotator& BaseRotation)
{
	if (!PortalClass) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 바닥에서 아주 살짝 띄워서 소환 (겹침 방지)
	FVector SpawnLoc = BaseLocation + FVector(0.f, 0.f, 5.f);

	GetWorld()->SpawnActor<AActor>(PortalClass, SpawnLoc, BaseRotation, SpawnParams);
}
