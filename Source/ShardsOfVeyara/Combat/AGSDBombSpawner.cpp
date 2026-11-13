// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AGSDBombSpawner.h"
#include "Combat/AGSDBomb.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

AAGSDBombSpawner::AAGSDBombSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 루트 씬 컴포넌트
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 2. 거치대/받침대 메쉬 (기본 콜리전 활성화, 메쉬가 없으면 투명하게 작동)
	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	PedestalMesh->SetupAttachment(SceneRoot);
	PedestalMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PedestalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 3. 폭탄 스폰 위치 (받침대 상단 높이에 맞춰 기본 오프셋 부여)
	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SceneRoot);
	SpawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	BombClass = AAGSDBomb::StaticClass();
}

void AAGSDBombSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		if (InitialSpawnDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AAGSDBombSpawner::OnRespawnTimerExpired, InitialSpawnDelay, false);
		}
		else
		{
			SpawnBomb();
		}
	}
}

void AAGSDBombSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	if (CurrentBomb.IsValid())
	{
		CurrentBomb->OnDestroyed.RemoveDynamic(this, &AAGSDBombSpawner::OnCurrentBombDestroyed);
	}

	Super::EndPlay(EndPlayReason);
}

void AAGSDBombSpawner::SpawnBomb()
{
	// 이미 살아있는 폭탄이 있다면 중복 스폰 방지
	if (CurrentBomb.IsValid())
	{
		return;
	}

	if (!BombClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BombSpawner] %s에 BombClass가 지정되지 않았습니다."), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FTransform SpawnTransform = SpawnPoint ? SpawnPoint->GetComponentTransform() : GetActorTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AAGSDBomb* NewBomb = World->SpawnActor<AAGSDBomb>(BombClass, SpawnTransform, SpawnParams);
	if (NewBomb)
	{
		CurrentBomb = NewBomb;
		NewBomb->OnDestroyed.AddDynamic(this, &AAGSDBombSpawner::OnCurrentBombDestroyed);

		// 스폰 나이아가라 이펙트
		if (SpawnVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				SpawnVFX,
				SpawnTransform.GetLocation(),
				SpawnTransform.GetRotation().Rotator()
			);
		}

		// 스폰 사운드
		if (SpawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, SpawnTransform.GetLocation());
		}

		// 블루프린트 연출 훅 호출
		OnBombSpawned(NewBomb);

		UE_LOG(LogTemp, Log, TEXT("[BombSpawner] 새 폭탄(%s)이 %s 위치에 생성되었습니다."), *NewBomb->GetName(), *SpawnTransform.GetLocation().ToString());
	}
}

void AAGSDBombSpawner::OnCurrentBombDestroyed(AActor* DestroyedActor)
{
	CurrentBomb.Reset();
	OnBombDestroyed();

	// 리젠 타이머 시작
	if (RespawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AAGSDBombSpawner::OnRespawnTimerExpired, RespawnDelay, false);
		UE_LOG(LogTemp, Log, TEXT("[BombSpawner] 폭탄이 파괴되었습니다. %.1f초 후 새 폭탄이 리젠됩니다."), RespawnDelay);
	}
	else
	{
		SpawnBomb();
	}
}

void AAGSDBombSpawner::OnRespawnTimerExpired()
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	SpawnBomb();
}

