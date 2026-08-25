// Fill out your copyright notice in the Description page of Project Settings.
 
#include "PortalSpawner.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "GameplayLogSubsystem.h"

APortalSpawner::APortalSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 에디터에서 범위를 지정할 수 있는 콜리전 박스 컴포넌트 생성
	SpawnAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnAreaBox"));
	RootComponent = SpawnAreaBox;

	// 기본 박스 크기 및 설정
	SpawnAreaBox->SetBoxExtent(FVector(500.f, 500.f, 300.f));
	SpawnAreaBox->SetCollisionProfileName(TEXT("NoCollision"));
	SpawnAreaBox->SetGenerateOverlapEvents(false);
	SpawnAreaBox->bHiddenInGame = true;
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
	if (UGameInstance* GameInst = GetGameInstance())
	{
		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())
		{
			FString MapName = TEXT("Unknown");
			float PlayTime = 0.f;
			if (UWorld* World = GetWorld())
			{
				MapName = World->GetMapName();
				MapName.RemoveFromStart(World->StreamingLevelsPrefix);
				PlayTime = LogSubsystem->GetLogData().StagePlayTimes.FindRef(MapName);
			}

			FString BossName = DestroyedActor ? DestroyedActor->GetName() : TEXT("Boss");
			LogSubsystem->RecordBossClear(BossName, 0.0f, 0);
			LogSubsystem->RecordStageClearPortalTime(MapName, PlayTime);
			LogSubsystem->RecordMapClearTime(MapName, PlayTime);
		}
	}

	// 보스 파괴 후 포털 스폰 실행
	TriggerSpawnPortal();
}

void APortalSpawner::TriggerSpawnPortal()
{
	if (!PortalClass || !GetWorld()) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return;

	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector PlayerForward = PlayerPawn->GetActorForwardVector();

	// 1. 플레이어의 앞쪽 목표 지점을 계산합니다.
	FVector DesiredLocation = PlayerLocation + (PlayerForward * SpawnOffsetDistance);

	// 2. 콜리전 박스 내부로 클램핑하고 바닥/NavMesh 위치로 보정합니다.
	FVector FinalLocation = CalculateClampedSpawnLocation(DesiredLocation);

	// 3. 포털의 회전 방향을 계산합니다.
	FRotator FinalRotation = GetActorRotation();
	if (bFacePlayer)
	{
		FVector DirectionToPlayer = PlayerLocation - FinalLocation;
		DirectionToPlayer.Z = 0.f; // 수평(Yaw) 회전만 적용
		if (!DirectionToPlayer.IsNearlyZero())
		{
			FinalRotation = DirectionToPlayer.Rotation();
		}
	}

	// 4. 포털 소환
	SpawnPortal(FinalLocation, FinalRotation);
}

FVector APortalSpawner::CalculateClampedSpawnLocation(const FVector& DesiredLocation) const
{
	FVector ClampedLocation = DesiredLocation;

	if (SpawnAreaBox)
	{
		// 1. 박스 컴포넌트의 로컬 좌표계로 변환 (박스 회전/트랜스폼 지원)
		const FTransform& BoxTransform = SpawnAreaBox->GetComponentTransform();
		FVector LocalPoint = BoxTransform.InverseTransformPosition(DesiredLocation);

		// 2. 박스의 언스케일된 Extent 범위로 Clamp
		FVector BoxExtent = SpawnAreaBox->GetUnscaledBoxExtent();
		LocalPoint.X = FMath::Clamp(LocalPoint.X, -BoxExtent.X, BoxExtent.X);
		LocalPoint.Y = FMath::Clamp(LocalPoint.Y, -BoxExtent.Y, BoxExtent.Y);
		LocalPoint.Z = FMath::Clamp(LocalPoint.Z, -BoxExtent.Z, BoxExtent.Z);

		// 3. 다시 월드 좌표계로 변환
		ClampedLocation = BoxTransform.TransformPosition(LocalPoint);
	}

	// 4. 해당 지점에서 바닥을 찾기 위해 위에서 아래로 라인 트레이스 수행
	float TraceHalfHeight = SpawnAreaBox ? FMath::Max(SpawnAreaBox->GetScaledBoxExtent().Z + 200.f, 500.f) : 500.f;
	FVector StartTrace = ClampedLocation + FVector(0.f, 0.f, TraceHalfHeight);
	FVector EndTrace = ClampedLocation - FVector(0.f, 0.f, TraceHalfHeight);

	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if (TargetBoss)
	{
		TraceParams.AddIgnoredActor(TargetBoss);
	}

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, TraceParams);

	FVector FinalLocation;
	if (bHit)
	{
		// 바닥을 찾았다면 해당 위치 사용
		FinalLocation = HitResult.Location;
	}
	else
	{
		// 바닥을 찾지 못했다면(낭떠러지 등) 내비 메시를 이용해 근처 유효한 위치를 찾습니다.
		UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
		if (NavSystem)
		{
			FNavLocation NavLocation;
			if (NavSystem->ProjectPointToNavigation(ClampedLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
			{
				FinalLocation = NavLocation.Location;
			}
			else
			{
				FinalLocation = ClampedLocation;
			}
		}
		else
		{
			FinalLocation = ClampedLocation;
		}
	}

	return FinalLocation;
}

void APortalSpawner::SpawnPortal(const FVector& BaseLocation, const FRotator& BaseRotation)
{
	if (!PortalClass || !GetWorld()) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 바닥에서 아주 살짝 띄워서 소환 (겹침 방지)
	FVector SpawnLoc = BaseLocation + FVector(0.f, 0.f, 5.f);

	GetWorld()->SpawnActor<AActor>(PortalClass, SpawnLoc, BaseRotation, SpawnParams);
}
