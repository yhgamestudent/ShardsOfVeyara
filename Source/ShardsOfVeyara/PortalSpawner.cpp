#include "PortalSpawner.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "GameplayLogSubsystem.h"
#include "SOVGameInstance.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Struct_InventorySlotData.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "Character/AGSDCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Camera/CameraShakeBase.h"

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

	// 보스 처치 후 구슬 조각 등 보상 아이템이 스폰될 중앙 타겟 위치 컴포넌트 생성
	RewardSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RewardSpawnPoint"));
	RewardSpawnPoint->SetupAttachment(RootComponent);
	RewardSpawnPoint->SetRelativeLocation(FVector::ZeroVector);

	// 시네마틱 컷신 연출 카메라 컴포넌트 생성
	CutsceneCameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CutsceneCameraComponent"));
	CutsceneCameraComp->SetupAttachment(RootComponent);
	// 구슬 스폰 지점을 드라마틱하게 바라보도록 기본 위치/회전/화각 설정
	CutsceneCameraComp->SetRelativeLocation(FVector(-450.f, 0.f, 150.f));
	CutsceneCameraComp->SetRelativeRotation(FRotator(15.f, 0.f, 0.f));
	CutsceneCameraComp->SetFieldOfView(65.f);
}

void APortalSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터 뷰포트에서 카메라나 RewardSpawnPoint를 이동/조작할 때 실시간으로 RewardSpawnPoint를 정면 조준하도록 자동 회전
	if (CutsceneCameraComp && RewardSpawnPoint)
	{
		const FVector CameraLoc = CutsceneCameraComp->GetComponentLocation();
		const FVector TargetLoc = RewardSpawnPoint->GetComponentLocation();

		if (!CameraLoc.Equals(TargetLoc, 1.0f))
		{
			const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(CameraLoc, TargetLoc);
			CutsceneCameraComp->SetWorldRotation(LookAtRot);
		}
	}
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

void APortalSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 인벤토리 델리게이트 바인딩 안전 해제
	if (BoundInventoryComp)
	{
		BoundInventoryComp->OnItemAdded.RemoveDynamic(this, &APortalSpawner::OnPlayerItemAdded);
		BoundInventoryComp = nullptr;
	}

	// 모든 타이머 안전하게 해제
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RewardSpawnTimerHandle);
		World->GetTimerManager().ClearTimer(RewardDescentTimerHandle);
		World->GetTimerManager().ClearTimer(PortalActivationTimerHandle);
		World->GetTimerManager().ClearTimer(CutsceneTimerHandle);
		World->GetTimerManager().ClearTimer(CutsceneBlendOutTimerHandle);
		World->GetTimerManager().ClearTimer(RewardPopTimerHandle);
		World->GetTimerManager().ClearTimer(PortalDropTimerHandle);
	}

	// 컷신 도중 종료될 경우 플레이어 상태 복원
	RestorePlayerAfterCutscene();

	Super::EndPlay(EndPlayReason);
}

FString APortalSpawner::GetUniqueRewardKey() const
{
	if (!RequiredItemID.IsEmpty())
	{
		return TEXT("BossReward_") + RequiredItemID;
	}
	if (RewardItemClass)
	{
		return TEXT("BossReward_") + RewardItemClass->GetName();
	}
	if (TargetBoss)
	{
		return TEXT("BossReward_") + TargetBoss->GetClass()->GetName();
	}
	return TEXT("BossReward_Default");
}

void APortalSpawner::OnBossDestroyed(AActor* DestroyedActor)
{
	bBossDefeated = true;

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

	// 1. 아이템 획득 조건이 비활성화된 경우: 보스 사망 즉시 포털 스폰
	if (!bRequireItemAcquisition)
	{
		TriggerSpawnPortal();
		return;
	}

	// 2. 1회성 드롭 아이템인 경우: 이미 드롭되었거나 인벤토리에 소지 중인지 확인 (재처치 시 아이템이 안 나오므로 즉시 스폰)
	if (bCheckAlreadyDropped && IsItemAlreadyDroppedOrAcquired())
	{
		TriggerSpawnPortal();
		return;
	}

	// 3. 최초 처치: 플레이어 인벤토리의 OnItemAdded 델리게이트를 감시
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (UAGSDInventoryComponent* InvComp = PlayerPawn->FindComponentByClass<UAGSDInventoryComponent>())
		{
			BoundInventoryComp = InvComp;
			InvComp->OnItemAdded.AddDynamic(this, &APortalSpawner::OnPlayerItemAdded);
		}
		else
		{
			// 인벤토리 컴포넌트가 없을 경우 즉시 스폰
			TriggerSpawnPortal();
			return;
		}
	}
	else
	{
		TriggerSpawnPortal();
		return;
	}

	// 4. 보스룸 중앙(RewardSpawnPoint)에 보상 구슬 스폰
	if (RewardItemClass && !bRewardSpawned)
	{
		if (bEnableRewardCutscene)
		{
			StartRewardCutscene();
		}
		else if (RewardSpawnDelay > 0.0f && GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(RewardSpawnTimerHandle, this, &APortalSpawner::SpawnRewardItem, RewardSpawnDelay, false);
		}
		else
		{
			SpawnRewardItem();
		}
	}
}

void APortalSpawner::StartRewardCutscene()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		SpawnRewardItem();
		return;
	}

	AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(PC->GetPawn());
	CachedPlayerController = PC;
	CachedPlayerCharacter = PlayerChar;

	// 1. 플레이어 조작 차단 및 애니메이션/물리 프리징 (완전 시간 정지 연출)
	PC->DisableInput(PC);
	if (PlayerChar)
	{
		PlayerChar->CustomTimeDilation = 0.0f;
	}

	// 2. 연출 카메라가 RewardSpawnPoint(구슬 스폰 지점)를 정면에서 프레임 중앙에 담도록 설정
	if (CutsceneCameraComp)
	{
		const FVector TargetLoc = RewardSpawnPoint ? RewardSpawnPoint->GetComponentLocation() : GetActorLocation();

		if (bUseManualCutsceneCameraTransform)
		{
			// 사용자가 에디터에서 맞춘 카메라 월드 위치를 그대로 유지하고, RewardSpawnPoint 중심을 조준하도록 회전만 갱신
			const FVector CameraWorldPos = CutsceneCameraComp->GetComponentLocation();
			const FVector AimTargetLoc = TargetLoc + FVector(0.f, 0.f, 20.f);
			const FRotator LookRot = UKismetMathLibrary::FindLookAtRotation(CameraWorldPos, AimTargetLoc);
			CutsceneCameraComp->SetWorldRotation(LookRot);
		}
		else
		{
			// 플레이어가 서 있는 쪽(시야가 확보된 방향)에서 RewardSpawnPoint를 바라보도록 카메라 오프셋 자동 산출
			FVector ViewDirection = RewardSpawnPoint ? RewardSpawnPoint->GetForwardVector() : GetActorForwardVector();
			if (PlayerChar)
			{
				const FVector FromPointToPlayer = (PlayerChar->GetActorLocation() - TargetLoc).GetSafeNormal2D();
				if (!FromPointToPlayer.IsNearlyZero())
				{
					ViewDirection = FromPointToPlayer;
				}
			}

			// RewardSpawnPoint로부터 약 3.5m 거리, 높이 1.2m 위치에 카메라 자동 배치
			const FVector CameraWorldPos = TargetLoc + (ViewDirection * 350.f) + FVector(0.f, 0.f, 120.f);
			CutsceneCameraComp->SetWorldLocation(CameraWorldPos);

			// 카메라가 RewardSpawnPoint(약간 위쪽 부유 지점)를 똑바로 바라보도록 회전 설정
			const FVector AimTargetLoc = TargetLoc + FVector(0.f, 0.f, 30.f);
			const FRotator LookRot = UKismetMathLibrary::FindLookAtRotation(CameraWorldPos, AimTargetLoc);
			CutsceneCameraComp->SetWorldRotation(LookRot);
		}
	}

	// 3. 시네마틱 연출 카메라로 부드럽게 전환
	PC->SetViewTargetWithBlend(this, CutsceneBlendInDuration, EViewTargetBlendFunction::VTBlend_EaseInOut, 2.0f);

	// 4. 카메라 전환 완료 후 + 프리징 대기(긴장감 정적) 후 구슬 스폰 호출
	const float TotalPreSpawnDelay = CutsceneBlendInDuration + CutscenePreSpawnFreezeDelay;
	World->GetTimerManager().SetTimer(
		CutsceneTimerHandle,
		this,
		&APortalSpawner::OnCutscenePreSpawnFreezeFinished,
		TotalPreSpawnDelay,
		false
	);
}

void APortalSpawner::OnCutscenePreSpawnFreezeFinished()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. 구슬 스폰
	SpawnRewardItem();

	// 2. "뽕!" 하고 튀어나오는 스케일 팝업 애니메이션 시작
	if (SpawnedRewardActor.IsValid())
	{
		SpawnedRewardActor->SetActorScale3D(FVector(0.05f));
		PopElapsedTime = 0.0f;
		World->GetTimerManager().SetTimer(
			RewardPopTimerHandle,
			this,
			&APortalSpawner::UpdateRewardPopScale,
			0.016f,
			true
		);
	}

	// 3. 구슬 하강 연출 감상 후 플레이어 카메라로 복귀 타이머 등록
	World->GetTimerManager().SetTimer(
		CutsceneTimerHandle,
		this,
		&APortalSpawner::StartCutsceneBlendOut,
		CutsceneHoldDuration,
		false
	);
}

void APortalSpawner::UpdateRewardPopScale()
{
	if (!SpawnedRewardActor.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RewardPopTimerHandle);
		}
		return;
	}

	PopElapsedTime += 0.016f;
	const float Duration = FMath::Max(0.05f, RewardPopDuration);
	const float Progress = FMath::Clamp(PopElapsedTime / Duration, 0.0f, 1.0f);

	float CurrentScale = 1.0f;
	if (Progress < 0.65f)
	{
		// 0.0 -> 0.65 구간: 0.05에서 1.25로 급격히 팽창 ("뽕!")
		const float Alpha = Progress / 0.65f;
		CurrentScale = FMath::Lerp(0.05f, 1.25f, FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f));
	}
	else
	{
		// 0.65 -> 1.0 구간: 1.25에서 1.0으로 탄력 있게 수렴
		const float Alpha = (Progress - 0.65f) / 0.35f;
		CurrentScale = FMath::Lerp(1.25f, 1.0f, FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f));
	}

	SpawnedRewardActor->SetActorScale3D(FVector(CurrentScale));

	if (Progress >= 1.0f)
	{
		SpawnedRewardActor->SetActorScale3D(FVector(1.0f));
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RewardPopTimerHandle);
		}
	}
}

void APortalSpawner::StartRewardDescent()
{
	if (!SpawnedRewardActor.IsValid() || !GetWorld()) return;

	// 이미 하강 타이머가 돌고 있다면 중복 실행 방지
	if (GetWorld()->GetTimerManager().IsTimerActive(RewardDescentTimerHandle)) return;

	// 소환 위치와 목표 위치 사이에 높이 차이가 있을 경우 서서히 하강하는 애니메이션 타이머 시작
	const float HeightDifference = FMath::Abs(RewardStartLocation.Z - RewardTargetLocation.Z);
	if (HeightDifference > 10.f)
	{
		DescentElapsedTime = 0.0f;
		GetWorld()->GetTimerManager().SetTimer(
			RewardDescentTimerHandle,
			this,
			&APortalSpawner::UpdateRewardDescent,
			0.016f,
			true
		);
	}
}

void APortalSpawner::StartCutsceneBlendOut()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 카메라가 플레이어 시점으로 돌아가기 시작할 때 구슬 하강 연출 개시
	StartRewardDescent();

	if (CachedPlayerController.IsValid() && CachedPlayerCharacter.IsValid())
	{
		// 플레이어 시점으로 천천히 부드럽게 복귀 (기본 1.8초)
		CachedPlayerController->SetViewTargetWithBlend(
			CachedPlayerCharacter.Get(),
			CutsceneBlendOutDuration,
			EViewTargetBlendFunction::VTBlend_Cubic,
			2.0f
		);

		// 블렌드 완료 시점에 플레이어 상태 복원
		World->GetTimerManager().SetTimer(
			CutsceneBlendOutTimerHandle,
			this,
			&APortalSpawner::RestorePlayerAfterCutscene,
			CutsceneBlendOutDuration,
			false
		);
	}
	else
	{
		RestorePlayerAfterCutscene();
	}
}

void APortalSpawner::RestorePlayerAfterCutscene()
{
	if (CachedPlayerCharacter.IsValid())
	{
		CachedPlayerCharacter->CustomTimeDilation = 1.0f;
	}

	if (CachedPlayerController.IsValid())
	{
		CachedPlayerController->EnableInput(CachedPlayerController.Get());
	}

	UE_LOG(LogTemp, Log, TEXT("[PortalSpawner] Cutscene finished! Player controls and time restored."));
}

void APortalSpawner::SpawnRewardItem()
{
	if (bRewardSpawned || !RewardItemClass || !GetWorld()) return;

	bRewardSpawned = true;

	// 1회성 드롭 기록 등록
	if (bCheckAlreadyDropped)
	{
		if (UGameInstance* GameInst = GetGameInstance())
		{
			if (USOVGameInstance* GI = Cast<USOVGameInstance>(GameInst))
			{
				const FString UniqueKey = GetUniqueRewardKey();
				if (!GI->AlreadyDroppedItems.Contains(UniqueKey))
				{
					GI->AlreadyDroppedItems.Add(UniqueKey);
				}
			}
		}
	}

	// 1. 소환 시작 위치: RewardSpawnPoint 월드 위치 그대로 사용
	const FVector SpawnLoc = RewardSpawnPoint ? RewardSpawnPoint->GetComponentLocation() : CalculateClampedSpawnLocation(GetActorLocation());
	const FRotator SpawnRot = RewardSpawnPoint ? RewardSpawnPoint->GetComponentRotation() : FRotator::ZeroRotator;

	RewardStartLocation = SpawnLoc;
	RewardTargetLocation = SpawnLoc; // 기본값: 바닥이 없거나 이미 바닥 근처면 소환 위치 유지

	// 2. 바닥 감지: 소환 위치에서 수직 아래로 라인 트레이스 수행
	FHitResult FloorHit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if (TargetBoss)
	{
		TraceParams.AddIgnoredActor(TargetBoss);
	}

	const FVector TraceStart = SpawnLoc;
	const FVector TraceEnd = SpawnLoc - FVector(0.f, 0.f, 2000.f); // 아래로 최대 20m 탐색
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
	{
		// 바닥이 감지됨! 바닥에서 일정 높이(RewardTargetFloatHeight, 기본 80cm) 위의 목표 위치 계산
		const float FloorZ = FloorHit.ImpactPoint.Z;
		const float DesiredStopZ = FloorZ + RewardTargetFloatHeight;

		// 소환 위치가 바닥 목표 높이보다 유의미하게(10cm 이상) 위에 있다면 하강 목표로 설정
		if (SpawnLoc.Z > DesiredStopZ + 10.f)
		{
			RewardTargetLocation = FVector(SpawnLoc.X, SpawnLoc.Y, DesiredStopZ);
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// RewardSpawnPoint 위치에서 구슬 스폰
	AActor* SpawnedReward = GetWorld()->SpawnActor<AActor>(RewardItemClass, RewardStartLocation, SpawnRot, SpawnParams);
	if (SpawnedReward)
	{
		SpawnedRewardActor = SpawnedReward;

		// 3. 중력 및 물리 시뮬레이션 전면 비활성화 (공중 부유 및 자유 하강 보장)
		TArray<UPrimitiveComponent*> PrimComponents;
		SpawnedReward->GetComponents<UPrimitiveComponent>(PrimComponents);
		for (UPrimitiveComponent* PrimComp : PrimComponents)
		{
			if (PrimComp)
			{
				PrimComp->SetSimulatePhysics(false);
				PrimComp->SetEnableGravity(false);
			}
		}

		// 4. 스폰되는 첫 순간부터 착지 이후까지 끊김 없이 회전하도록 RotatingMovementComponent 부착
		if (RewardDescentRotationSpeed > 0.0f)
		{
			URotatingMovementComponent* RotComp = NewObject<URotatingMovementComponent>(SpawnedReward, TEXT("RewardRotatingComp"));
			if (RotComp)
			{
				RotComp->RotationRate = FRotator(0.f, RewardDescentRotationSpeed, 0.f);
				RotComp->RegisterComponent();
			}
		}

		// 5. 컷신 연출이 비활성화된 일반 스폰일 때만 즉시 하강 시작 (컷신 연출 중에는 카메라 복귀 시점 StartCutsceneBlendOut에서 하강 개시)
		if (!bEnableRewardCutscene)
		{
			StartRewardDescent();
		}
	}

	// 나이아가라 이펙트 재생 (소환 위치에서 화려하게 개화)
	if (RewardSpawnEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RewardSpawnEffect, RewardStartLocation, SpawnRot);
	}

	// 효과음 재생
	if (RewardSpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RewardSpawnSound, RewardStartLocation);
	}
}

void APortalSpawner::UpdateRewardDescent()
{
	if (!SpawnedRewardActor.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RewardDescentTimerHandle);
		}
		return;
	}

	const float DeltaSec = 0.016f;
	DescentElapsedTime += DeltaSec;

	const float Duration = FMath::Max(RewardDescentDuration, 0.1f);
	const float Alpha = FMath::Clamp(DescentElapsedTime / Duration, 0.0f, 1.0f);

	// InterpEaseOut 곡선을 적용하여 위에서 부드럽게 출발하고 목표 지점에 다다를수록 천천히 감속 착지
	const float SmoothAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
	const FVector CurrentLoc = FMath::Lerp(RewardStartLocation, RewardTargetLocation, SmoothAlpha);
	SpawnedRewardActor->SetActorLocation(CurrentLoc);

	// 하강 완료 시 타이머 종료
	if (Alpha >= 1.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RewardDescentTimerHandle);
		}
	}
}

void APortalSpawner::OnPlayerItemAdded(int32 SlotIndex, const FStruct_ItemData& ItemData)
{
	if (!bBossDefeated || bPortalSpawned)
	{
		return;
	}

	// RequiredItemID가 지정되어 있으면 ID 완전 일치 검사, 비어있으면 퀘스트 아이템(EIT_Quest) 타입 검사
	const bool bIsMatch = !RequiredItemID.IsEmpty()
		? ItemData.ItemID.Equals(RequiredItemID, ESearchCase::IgnoreCase)
		: (ItemData.ItemType == EItemType::EIT_Quest);

	if (bIsMatch)
	{
		if (BoundInventoryComp)
		{
			BoundInventoryComp->OnItemAdded.RemoveDynamic(this, &APortalSpawner::OnPlayerItemAdded);
			BoundInventoryComp = nullptr;
		}

		TriggerSpawnPortal();
	}
}

bool APortalSpawner::IsItemAlreadyDroppedOrAcquired() const
{
	// 1. GameInstance의 1회성 드롭 기록(AlreadyDroppedItems) 확인
	if (UGameInstance* GameInst = GetGameInstance())
	{
		if (USOVGameInstance* GI = Cast<USOVGameInstance>(GameInst))
		{
			const FString UniqueKey = GetUniqueRewardKey();
			if (GI->AlreadyDroppedItems.Contains(UniqueKey))
			{
				return true;
			}

			// 1) TargetBoss가 지정된 경우, 해당 보스 엔티티의 드롭 기록 확인
			if (TargetBoss)
			{
				const FString BossClassName = TargetBoss->GetClass()->GetName();
				const FString BossName = TargetBoss->GetName();
				for (const FString& DroppedKey : GI->AlreadyDroppedItems)
				{
					if (DroppedKey.Contains(BossClassName) || DroppedKey.Contains(BossName))
					{
						return true;
					}
				}
			}
			// 2) RequiredItemID가 명시적으로 지정된 경우 해당 아이템의 드롭 기록 확인
			if (!RequiredItemID.IsEmpty())
			{
				for (const FString& DroppedKey : GI->AlreadyDroppedItems)
				{
					if (DroppedKey.Contains(RequiredItemID))
					{
						return true;
					}
				}
			}
		}
	}

	// 2. 플레이어 인벤토리에 '정확히 해당 RequiredItemID'를 소지하고 있는지 확인 (RequiredItemID가 명시된 경우만)
	if (!RequiredItemID.IsEmpty())
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (UAGSDInventoryComponent* InvComp = PlayerPawn->FindComponentByClass<UAGSDInventoryComponent>())
			{
				for (const FStruct_InventorySlotData& Slot : InvComp->GetAllSlots())
				{
					if (!Slot.IsEmpty && Slot.ItemData.CurrentQuantity > 0)
					{
						if (Slot.ItemData.ItemID.Equals(RequiredItemID, ESearchCase::IgnoreCase))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void APortalSpawner::TriggerSpawnPortal()
{
	if (bPortalSpawned) return;
	if (!PortalClass || !GetWorld()) return;

	bPortalSpawned = true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	FVector FinalLocation;
	FRotator FinalRotation = GetActorRotation();

	// 1. 보상 아이템 위치(중앙)에 포털 소환 모드인 경우
	if (bSpawnPortalAtRewardLocation && RewardSpawnPoint)
	{
		if (PlayerPawn)
		{
			const FVector PlayerLocation = PlayerPawn->GetActorLocation();
			const FVector PlayerForward = PlayerPawn->GetActorForwardVector();
			const FVector PlayerRight = PlayerPawn->GetActorRightVector();

			// 플레이어가 구슬을 먹은 위치 기준으로 전방 약 300 유닛 떨어진 안전 지점 계산
			const float SafeDistance = FMath::Max(SpawnOffsetDistance, MinPlayerDistance);
			const FVector DesiredPortalLoc = PlayerLocation + (PlayerForward * SafeDistance);
			FinalLocation = CalculateClampedSpawnLocation(DesiredPortalLoc);

			// 만약 전방 위치가 벽이나 장애물로 인해 플레이어와 너무 가깝다면(MinPlayerDistance 미만) 8방향 안전 위치 탐색으로 보정
			if (FVector::Dist2D(FinalLocation, PlayerLocation) < MinPlayerDistance)
			{
				FinalLocation = FindSafeSpawnLocation(PlayerLocation, PlayerForward, PlayerRight);
			}

			// 소환된 포털이 플레이어를 정면으로 바라보도록 회전 정렬
			if (bFacePlayer)
			{
				FVector DirectionToPlayer = PlayerLocation - FinalLocation;
				DirectionToPlayer.Z = 0.f;
				if (!DirectionToPlayer.IsNearlyZero())
				{
					FinalRotation = DirectionToPlayer.Rotation();
				}
			}
		}
		else
		{
			FinalLocation = CalculateClampedSpawnLocation(RewardSpawnPoint->GetComponentLocation());
			FinalRotation = RewardSpawnPoint->GetComponentRotation();
		}
	}
	else
	{
		// 2. 플레이어 전방 8방향 안전 위치 탐색 모드
		if (!PlayerPawn) return;

		FVector PlayerLocation = PlayerPawn->GetActorLocation();
		FVector PlayerForward = PlayerPawn->GetActorForwardVector();
		FVector PlayerRight = PlayerPawn->GetActorRightVector();

		FinalLocation = FindSafeSpawnLocation(PlayerLocation, PlayerForward, PlayerRight);

		if (bFacePlayer)
		{
			FVector DirectionToPlayer = PlayerLocation - FinalLocation;
			DirectionToPlayer.Z = 0.f; // 수평(Yaw) 회전만 적용
			if (!DirectionToPlayer.IsNearlyZero())
			{
				FinalRotation = DirectionToPlayer.Rotation();
			}
		}
	}

	// 3. 포털 소환
	SpawnPortal(FinalLocation, FinalRotation);
}

FVector APortalSpawner::FindSafeSpawnLocation(const FVector& PlayerLocation, const FVector& PlayerForward, const FVector& PlayerRight) const
{
	// 기본 검색 거리 (최소 안전 거리 이상으로 설정)
	const float SearchDistance = FMath::Max(SpawnOffsetDistance, MinPlayerDistance);

	// 1. 플레이어 기준 8방향 후보 벡터 생성 (정면 -> 대각 앞 -> 좌우 -> 대각 뒤 -> 후방)
	TArray<FVector> CandidateDirections;
	CandidateDirections.Add(PlayerForward);                                                      // 0도 정면
	CandidateDirections.Add((PlayerForward + PlayerRight).GetSafeNormal());                     // 45도 우측 앞
	CandidateDirections.Add((PlayerForward - PlayerRight).GetSafeNormal());                     // 45도 좌측 앞
	CandidateDirections.Add(PlayerRight);                                                       // 90도 우측
	CandidateDirections.Add(-PlayerRight);                                                      // 90도 좌측
	CandidateDirections.Add((-PlayerForward + PlayerRight).GetSafeNormal());                    // 135도 우측 뒤
	CandidateDirections.Add((-PlayerForward - PlayerRight).GetSafeNormal());                    // 135도 좌측 뒤
	CandidateDirections.Add(-PlayerForward);                                                    // 180도 후방

	FVector BestLocation = PlayerLocation + (PlayerForward * SearchDistance);
	float MaxFoundDist2D = 0.f;

	for (const FVector& Dir : CandidateDirections)
	{
		const FVector DesiredLoc = PlayerLocation + (Dir * SearchDistance);
		const FVector ClampedLoc = CalculateClampedSpawnLocation(DesiredLoc);

		const float Dist2D = FVector::Dist2D(ClampedLoc, PlayerLocation);

		// 플레이어와의 거리가 최소 안전 거리(MinPlayerDistance, 기본 300) 이상이면 즉시 채택
		if (Dist2D >= MinPlayerDistance)
		{
			return ClampedLoc;
		}

		if (Dist2D > MaxFoundDist2D)
		{
			MaxFoundDist2D = Dist2D;
			BestLocation = ClampedLoc;
		}
	}

	// 2. 8방향 탐색으로도 300 거리가 안 나오는 경우(구석에 갇힌 경우 등), 스폰 영역 박스 중심점도 확인
	if (SpawnAreaBox)
	{
		const FVector BoxCenterLoc = CalculateClampedSpawnLocation(SpawnAreaBox->GetComponentLocation());
		const float DistToCenter = FVector::Dist2D(BoxCenterLoc, PlayerLocation);
		if (DistToCenter >= MinPlayerDistance)
		{
			return BoxCenterLoc;
		}
		if (DistToCenter > MaxFoundDist2D)
		{
			BestLocation = BoxCenterLoc;
		}
	}

	return BestLocation;
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

	// 1. 목표 바닥 착지 위치
	PortalDropTargetLocation = BaseLocation + FVector(0.f, 0.f, 5.f);

	// 2. 공중 상공 시작 위치 (PortalDropHeight, 기본 +400cm)
	PortalDropStartLocation = PortalDropTargetLocation + FVector(0.f, 0.f, FMath::Max(100.f, PortalDropHeight));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 공중 상공에서 포탈 스폰!
	AActor* NewPortal = GetWorld()->SpawnActor<AActor>(PortalClass, PortalDropStartLocation, BaseRotation, SpawnParams);
	if (NewPortal)
	{
		SpawnedPortalActor = NewPortal;

		// 초기 스케일 0.1로 작게 시작 (공중 팝업 효과)
		NewPortal->SetActorScale3D(FVector(0.1f));

		// 낙하 및 착지 완료 전까지 물리 및 텔레포트 오버랩 차단
		TArray<UPrimitiveComponent*> PrimComponents;
		NewPortal->GetComponents<UPrimitiveComponent>(PrimComponents);
		for (UPrimitiveComponent* PrimComp : PrimComponents)
		{
			if (PrimComp)
			{
				PrimComp->SetGenerateOverlapEvents(false);
				PrimComp->SetEnableGravity(false);
				PrimComp->SetSimulatePhysics(false);
			}
		}

		// 공중 "뿅!" 나이아가라 이펙트 및 사운드 재생
		if (PortalPopEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PortalPopEffect, PortalDropStartLocation, BaseRotation);
		}
		if (PortalPopSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PortalPopSound, PortalDropStartLocation, PortalPopSoundVolume, PortalPopSoundPitch);
		}

		// 지면을 향해 급강하하는 타이머 시작 (60fps)
		PortalDropElapsedTime = 0.0f;
		GetWorld()->GetTimerManager().SetTimer(
			PortalDropTimerHandle,
			this,
			&APortalSpawner::UpdatePortalDrop,
			0.016f,
			true
		);
	}
}

void APortalSpawner::UpdatePortalDrop()
{
	if (!SpawnedPortalActor.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PortalDropTimerHandle);
		}
		return;
	}

	const float DeltaSec = 0.016f;
	PortalDropElapsedTime += DeltaSec;

	const float Duration = FMath::Max(0.1f, PortalDropDuration);
	const float Alpha = FMath::Clamp(PortalDropElapsedTime / Duration, 0.0f, 1.0f);

	// 1. 스케일 팝업: 0.1f -> 1.0f로 빠르게 확장
	const float ScaleAlpha = FMath::Clamp(PortalDropElapsedTime / (Duration * 0.5f), 0.0f, 1.0f);
	const float CurrentScale = FMath::Lerp(0.1f, 1.0f, FMath::InterpEaseOut(0.0f, 1.0f, ScaleAlpha, 2.0f));
	SpawnedPortalActor->SetActorScale3D(FVector(CurrentScale));

	// 2. 가속 급강하: 실제 중력 가속도(InterpEaseIn)로 지면을 향해 쐐기처럼 낙하
	const float DropAlpha = FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.5f);
	const FVector CurrentLoc = FMath::Lerp(PortalDropStartLocation, PortalDropTargetLocation, DropAlpha);
	SpawnedPortalActor->SetActorLocation(CurrentLoc);

	// 3. 지면 착지 완료!
	if (Alpha >= 1.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PortalDropTimerHandle);
		}
		SpawnedPortalActor->SetActorLocation(PortalDropTargetLocation);
		SpawnedPortalActor->SetActorScale3D(FVector(1.0f));

		OnPortalHitGround();
	}
}

void APortalSpawner::OnPortalHitGround()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. 캐릭터 화면에 지진 효과 (CameraShake) 발생
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (PortalImpactCameraShakeClass)
		{
			PC->ClientStartCameraShake(PortalImpactCameraShakeClass, 1.5f);
		}
		else
		{
			PC->ClientStartCameraShake(UCameraShakeBase::StaticClass(), 1.0f);
		}
	}

	// 2. 지면 착지 임팩트 나이아가라 이펙트 재생
	if (PortalImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, PortalImpactEffect, PortalDropTargetLocation, FRotator::ZeroRotator);
	}

	// 3. 지면 충돌 쿵! 사운드 재생
	if (PortalImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PortalImpactSound, PortalDropTargetLocation, PortalImpactSoundVolume, PortalImpactSoundPitch, 0.0f, PortalImpactSoundAttenuation);
	}

	// 4. 안전 딜레이(PortalActivationDelay) 후 텔레포트 오버랩 활성화
	if (PortalActivationDelay > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			PortalActivationTimerHandle,
			this,
			&APortalSpawner::ActivatePortalOverlap,
			PortalActivationDelay,
			false
		);
	}
	else
	{
		ActivatePortalOverlap();
	}
}

void APortalSpawner::ActivatePortalOverlap()
{
	if (SpawnedPortalActor.IsValid())
	{
		TArray<UPrimitiveComponent*> PrimComponents;
		SpawnedPortalActor->GetComponents<UPrimitiveComponent>(PrimComponents);
		for (UPrimitiveComponent* PrimComp : PrimComponents)
		{
			if (PrimComp)
			{
				PrimComp->SetGenerateOverlapEvents(true);
			}
		}
	}
}
