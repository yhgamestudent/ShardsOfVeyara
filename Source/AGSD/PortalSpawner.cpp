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

void APortalSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 인벤토리 델리게이트 바인딩 안전 해제
	if (BoundInventoryComp)
	{
		BoundInventoryComp->OnItemAdded.RemoveDynamic(this, &APortalSpawner::OnPlayerItemAdded);
		BoundInventoryComp = nullptr;
	}

	Super::EndPlay(EndPlayReason);
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
		}
	}
	else
	{
		TriggerSpawnPortal();
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
	// 1. GameInstance의 1회성 드롭 기록(AlreadyDroppedItems) 및 조각 개수 확인
	if (UGameInstance* GameInst = GetGameInstance())
	{
		if (USOVGameInstance* GI = Cast<USOVGameInstance>(GameInst))
		{
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
			else if (TargetBoss)
			{
				// RequiredItemID가 비어있고 TargetBoss가 지정된 경우, 해당 보스의 드롭 기록 확인
				const FString BossClassName = TargetBoss->GetClass()->GetName();
				for (const FString& DroppedKey : GI->AlreadyDroppedItems)
				{
					if (DroppedKey.Contains(BossClassName) || DroppedKey.Contains(TargetBoss->GetName()))
					{
						return true;
					}
				}
			}

			// 차원 조각 수량이 이미 존재하는 경우 (이미 1개 이상 획득/제단 등록)
			if (GI->ShardsAmount > 0)
			{
				return true;
			}
		}
	}

	// 2. 플레이어 인벤토리에 이미 해당 아이템 또는 퀘스트 아이템을 소지하고 있는지 확인
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (UAGSDInventoryComponent* InvComp = PlayerPawn->FindComponentByClass<UAGSDInventoryComponent>())
		{
			for (const FStruct_InventorySlotData& Slot : InvComp->GetAllSlots())
			{
				if (!Slot.IsEmpty && Slot.ItemData.CurrentQuantity > 0)
				{
					if (!RequiredItemID.IsEmpty())
					{
						if (Slot.ItemData.ItemID.Equals(RequiredItemID, ESearchCase::IgnoreCase))
						{
							return true;
						}
					}
					else if (Slot.ItemData.ItemType == EItemType::EIT_Quest)
					{
						return true;
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
	if (!PlayerPawn) return;

	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector PlayerForward = PlayerPawn->GetActorForwardVector();
	FVector PlayerRight = PlayerPawn->GetActorRightVector();

	// 1. 플레이어와 최소 300 반경 안전 거리를 유지하는 유효 위치를 탐색합니다.
	FVector FinalLocation = FindSafeSpawnLocation(PlayerLocation, PlayerForward, PlayerRight);

	// 2. 포털의 회전 방향을 계산합니다.
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

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 바닥에서 아주 살짝 띄워서 소환 (겹침 방지)
	FVector SpawnLoc = BaseLocation + FVector(0.f, 0.f, 5.f);

	GetWorld()->SpawnActor<AActor>(PortalClass, SpawnLoc, BaseRotation, SpawnParams);
}
