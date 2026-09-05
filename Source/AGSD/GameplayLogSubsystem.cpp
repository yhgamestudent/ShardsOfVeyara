#include "GameplayLogSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"

void UGameplayLogSubsystem::InitNewSession()
{
	// 로그 데이터를 완전히 초기화하고 새 UUID를 세션 ID로 할당합니다
	LogData = FGameplayLogData();
	LogData.PlayerSessionID = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
	bIsBossBattleActive = false;
	CurrentActiveBossName.Empty();
	ClearedBossStages.Empty();
}

FString UGameplayLogSubsystem::GetTargetStageName(const FString& InStageName) const
{
	if (!InStageName.IsEmpty())
	{
		return InStageName;
	}

	if (UWorld* World = GetWorld())
	{
		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
		if (!MapName.IsEmpty())
		{
			return MapName;
		}
	}

	return TEXT("Unknown_Stage");
}

bool UGameplayLogSubsystem::IsEtcStage(const FString& StageName) const
{
	FString Target = GetTargetStageName(StageName);
	for (const FString& Kw : EtcStageKeywords)
	{
		if (Target.Contains(Kw, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

bool UGameplayLogSubsystem::IsExplorationStage(const FString& StageName) const
{
	FString Target = GetTargetStageName(StageName);
	for (const FString& Kw : ExplorationStageKeywords)
	{
		if (Target.Contains(Kw, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	// 명시적 탐험 키워드에 없더라도 기타(거점/마을/메뉴) 맵이 아니면 탐험 맵으로 취급
	return !IsEtcStage(Target);
}

void UGameplayLogSubsystem::SetBossBattleActive(bool bActive, const FString& BossName)
{
	bIsBossBattleActive = bActive;
	if (bActive)
	{
		if (!BossName.IsEmpty())
		{
			CurrentActiveBossName = BossName;
		}
	}
	else
	{
		CurrentActiveBossName.Empty();
	}
}

bool UGameplayLogSubsystem::IsStageBossCleared(const FString& StageName) const
{
	FString Target = GetTargetStageName(StageName);
	return ClearedBossStages.Contains(Target);
}

void UGameplayLogSubsystem::SetStageBossCleared(const FString& StageName, bool bCleared)
{
	FString Target = GetTargetStageName(StageName);
	if (bCleared)
	{
		ClearedBossStages.Add(Target);
	}
	else
	{
		ClearedBossStages.Remove(Target);
	}
}

void UGameplayLogSubsystem::AddCropGrowthDelayDueToWeeds(float DelaySeconds, const FString& StageName)
{
	LogData.TotalCropGrowthDelayDueToWeeds += DelaySeconds;

	FString TargetStage = GetTargetStageName(StageName);
	float& StageDelay = LogData.StageCropGrowthDelayDueToWeeds.FindOrAdd(TargetStage);
	StageDelay += DelaySeconds;
}

void UGameplayLogSubsystem::RecordPlayerPositionAtDateChange(int32 Day, FVector Position)
{
	LogData.PlayerPositionsAtDateChange.Add(Day, Position);
}

void UGameplayLogSubsystem::IncrementFailedPotionCrafting()
{
	LogData.FailedPotionCraftingCount++;
}

void UGameplayLogSubsystem::RecordUsedCombo(const FString& ComboName, const FString& StageName)
{
	int32& Count = LogData.UsedComboCounts.FindOrAdd(ComboName);
	Count++;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *ComboName);
	int32& StageCount = LogData.StageUsedComboCounts.FindOrAdd(Key);
	StageCount++;
}

void UGameplayLogSubsystem::RecordCompletedCombo(const FString& ComboName, const FString& StageName)
{
	int32& Count = LogData.CompletedComboCounts.FindOrAdd(ComboName);
	Count++;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *ComboName);
	int32& StageCount = LogData.StageCompletedComboCounts.FindOrAdd(Key);
	StageCount++;
}

void UGameplayLogSubsystem::RecordElixirUsageOnCrop(const FString& CropName, const FString& StageName)
{
	int32& Count = LogData.ElixirUsagePerCrop.FindOrAdd(CropName);
	Count++;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *CropName);
	int32& StageCount = LogData.StageElixirUsageCounts.FindOrAdd(Key);
	StageCount++;
}

void UGameplayLogSubsystem::AddAcquiredCoins(int32 Amount, const FString& StageName)
{
	LogData.TotalAcquiredCoins += Amount;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageAmount = LogData.StageAcquiredCoins.FindOrAdd(TargetStage);
	StageAmount += Amount;
}

void UGameplayLogSubsystem::AddConsumedCoins(int32 Amount, const FString& StageName)
{
	LogData.TotalConsumedCoins += Amount;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageAmount = LogData.StageConsumedCoins.FindOrAdd(TargetStage);
	StageAmount += Amount;
}

void UGameplayLogSubsystem::RecordNPCTransaction(const FString& NPCName, const FString& ItemOrDetail, int32 CoinCost, int32 Quantity, const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);

	// 1. 코인 총 소모량 및 맵별 소모량 누적
	LogData.TotalConsumedCoins += CoinCost;
	LogData.StageConsumedCoins.FindOrAdd(TargetStage) += CoinCost;

	// 2. 맵_NPC_아이템 소모 누적 TMap
	FString DetailKey = FString::Printf(TEXT("%s_%s_%s"), *TargetStage, *NPCName, *ItemOrDetail);
	LogData.StageNPCCoinConsumption.FindOrAdd(DetailKey) += CoinCost;

	// 3. 개별 거래 이력 구조체 추가
	FNPCTransactionRecord Record;
	Record.StageName = TargetStage;
	Record.NPCName = NPCName;
	Record.ItemOrDetail = ItemOrDetail;
	Record.Cost = CoinCost;
	Record.Quantity = Quantity;
	LogData.NPCTransactions.Add(Record);
}

void UGameplayLogSubsystem::RecordMapClearTime(const FString& MapName, float TimeSeconds)
{
	FString TargetStage = GetTargetStageName(MapName);
	LogData.MapClearTimes.Add(TargetStage, TimeSeconds);

	// 🏆 최초 1회차 완주 스냅샷 자동 기록
	SnapshotStageFirstClear(TargetStage);
}

void UGameplayLogSubsystem::AddDamageMitigatedByGuard(float MitigatedDamage, const FString& StageName)
{
	LogData.TotalDamageMitigatedByGuard += MitigatedDamage;

	FString TargetStage = GetTargetStageName(StageName);
	float& StageMitigated = LogData.StageDamageMitigatedByGuard.FindOrAdd(TargetStage);
	StageMitigated += MitigatedDamage;
}

void UGameplayLogSubsystem::AddPlayTime(float TimeSeconds)
{
	LogData.TotalPlayTime += TimeSeconds;
}

void UGameplayLogSubsystem::IncrementPauseCount(const FString& StageName)
{
	LogData.PauseCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StagePauseCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementItemsAcquiredCount(const FString& StageName)
{
	LogData.TotalItemsAcquired++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageItemsAcquired.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementItemsDiscardedCount(const FString& StageName)
{
	LogData.TotalItemsDiscarded++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageItemsDiscarded.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementInventoryFullOccurrence(const FString& StageName)
{
	LogData.InventoryFullOccurrenceCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageInventoryFullCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementInventoryOpenCount(const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageInventoryOpenCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::RecordNPCDialogue(const FString& NPCName, const FString& StageName)
{
	int32& Count = LogData.NPCDialogueCounts.FindOrAdd(NPCName);
	Count++;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *NPCName);
	int32& StageCount = LogData.StageNPCDialogueCounts.FindOrAdd(Key);
	StageCount++;
}

void UGameplayLogSubsystem::RecordMonsterKill(const FString& MonsterName, const FString& StageName)
{
	FString CleanMonsterName = MonsterName;
	int32 LastUnderscoreIndex = INDEX_NONE;
	if (CleanMonsterName.FindLastChar(TEXT('_'), LastUnderscoreIndex))
	{
		FString Suffix = CleanMonsterName.Mid(LastUnderscoreIndex + 1);
		if (Suffix.IsNumeric())
		{
			CleanMonsterName = CleanMonsterName.Left(LastUnderscoreIndex);
		}
	}

	int32& Count = LogData.MonsterKillCounts.FindOrAdd(CleanMonsterName);
	Count++;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *CleanMonsterName);
	int32& StageCount = LogData.StageMonsterKillCounts.FindOrAdd(Key);
	StageCount++;

	int32& StageTotal = LogData.StageTotalMonsterKillCounts.FindOrAdd(TargetStage);
	StageTotal++;
}

void UGameplayLogSubsystem::RecordCropHarvest(const FString& CropName, int32 Amount, const FString& StageName)
{
	int32& Count = LogData.CropHarvestCounts.FindOrAdd(CropName);
	Count += Amount;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *CropName);
	int32& StageCount = LogData.StageCropHarvestCounts.FindOrAdd(Key);
	StageCount += Amount;

	// 🌾 수확 행동 시행 횟수 (1회 상호작용 = 1회 행동)
	LogData.TotalCropHarvestActionCount++;
	int32& ActionCount = LogData.StageCropHarvestActionCounts.FindOrAdd(TargetStage);
	ActionCount++;
}

void UGameplayLogSubsystem::RecordCropPlant(const FString& CropOrSeedName, const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);

	LogData.TotalCropPlantCount++;
	int32& Count = LogData.StageCropPlantCounts.FindOrAdd(TargetStage);
	Count++;

	float CurrentTotalPlayTime = LogData.TotalPlayTime;
	if (LogData.FirstCropPlantTime <= 0.0f)
	{
		LogData.FirstCropPlantTime = CurrentTotalPlayTime;
	}
	LogData.CropPlantTimestamps.Add(CurrentTotalPlayTime);
}

void UGameplayLogSubsystem::RecordInteractionAction(const FString& ActionType, const FString& StageName)
{
	if (ActionType.IsEmpty()) return;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *ActionType);
	int32& Count = LogData.StageInteractionActionCounts.FindOrAdd(Key);
	Count++;
}

void UGameplayLogSubsystem::IncrementGuardUsageCount(const FString& StageName)
{
	LogData.GuardUsageCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageGuardCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::RecordHealthPotionUsage(const FString& PotionName, const FString& StageName)
{
	LogData.TotalHealthPotionUsageCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageHealthPotionUsageCounts.FindOrAdd(TargetStage);
	StageCount++;
}

// ==========================================
// 📌 9대 가설 & 18종 차트 로깅 헬퍼 구현
// ==========================================

void UGameplayLogSubsystem::AddStagePlayTime(const FString& StageName, float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	float& Time = LogData.StagePlayTimes.FindOrAdd(TargetStage);
	Time += DeltaSeconds;

	// 세분화된 카테고리(탐험, 보스전, 기타) 자동 분배 누적
	AddCategorizedPlayTime(TargetStage, DeltaSeconds);
}

void UGameplayLogSubsystem::AddCategorizedPlayTime(const FString& StageName, float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);

	// 1. 거점 / 튜토리얼 / 마을 / UI 등 기타 맵인 경우
	if (IsEtcStage(TargetStage))
	{
		AddEtcPlayTime(TargetStage, DeltaSeconds);
	}
	// 2. 탐험 맵인 경우
	else
	{
		// 2-1. 보스전 진행 중인 경우 (보스 몬스터와 전투 시작 후 ~ 격파/사망 전)
		if (bIsBossBattleActive)
		{
			AddBossPlayTime(TargetStage, DeltaSeconds);
		}
		// 2-2. 해당 탐험 맵의 보스를 이미 처치 완료한 경우 (포탈 탑승 대기 및 아이템 정리 중 -> '기타' 시간으로 기록)
		else if (ClearedBossStages.Contains(TargetStage))
		{
			AddEtcPlayTime(TargetStage, DeltaSeconds);
		}
		// 2-3. 보스전 시작 전 일반 탐험/필드 진행 중인 경우 -> '탐험' 시간으로 기록
		else
		{
			AddExplorationPlayTime(TargetStage, DeltaSeconds);
		}
	}
}

void UGameplayLogSubsystem::AddExplorationPlayTime(const FString& StageName, float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	LogData.TotalExplorationTime += DeltaSeconds;
	float& StageTime = LogData.StageExplorationTimes.FindOrAdd(TargetStage);
	StageTime += DeltaSeconds;
}

void UGameplayLogSubsystem::AddBossPlayTime(const FString& StageName, float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	LogData.TotalBossTime += DeltaSeconds;
	float& StageTime = LogData.StageBossTimes.FindOrAdd(TargetStage);
	StageTime += DeltaSeconds;
}

void UGameplayLogSubsystem::AddEtcPlayTime(const FString& StageName, float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	LogData.TotalEtcTime += DeltaSeconds;
	float& StageTime = LogData.StageEtcTimes.FindOrAdd(TargetStage);
	StageTime += DeltaSeconds;
}

void UGameplayLogSubsystem::RecordStageClearPortalTime(const FString& StageName, float TimeSeconds)
{
	FString TargetStage = GetTargetStageName(StageName);
	LogData.StageClearPortalTimes.Add(TargetStage, TimeSeconds);

	// 🏆 최초 1회차 완주 스냅샷 자동 기록
	SnapshotStageFirstClear(TargetStage);
}

void UGameplayLogSubsystem::AddStageMovementDistance(const FString& StageName, float DistanceMeters)
{
	if (DistanceMeters <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	float& Dist = LogData.StageMovementDistances.FindOrAdd(TargetStage);
	Dist += DistanceMeters;

	// 세분화된 카테고리(탐험, 보스전, 기타) 자동 분배 누적
	AddCategorizedMovementDistance(TargetStage, DistanceMeters);
}

void UGameplayLogSubsystem::AddCategorizedMovementDistance(const FString& StageName, float DistanceMeters)
{
	if (DistanceMeters <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);

	// 1. 거점 / 튜토리얼 / 마을 / UI 등 기타 맵인 경우
	if (IsEtcStage(TargetStage))
	{
		AddEtcMovementDistance(TargetStage, DistanceMeters);
	}
	// 2. 탐험 맵인 경우
	else
	{
		// 2-1. 보스전 진행 중인 경우 (보스 몬스터와 전투 시작 후 ~ 격파/사망 전)
		if (bIsBossBattleActive)
		{
			AddBossMovementDistance(TargetStage, DistanceMeters);
		}
		// 2-2. 해당 탐험 맵의 보스를 이미 처치 완료한 경우 (포탈 탑승 대기 및 아이템 정리 중 -> '기타' 이동거리로 기록)
		else if (ClearedBossStages.Contains(TargetStage))
		{
			AddEtcMovementDistance(TargetStage, DistanceMeters);
		}
		// 2-3. 보스전 시작 전 일반 탐험/필드 진행 중인 경우 -> '탐험' 이동거리로 기록
		else
		{
			AddExplorationMovementDistance(TargetStage, DistanceMeters);
		}
	}
}

void UGameplayLogSubsystem::AddExplorationMovementDistance(const FString& StageName, float DistanceMeters)
{
	if (DistanceMeters <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	LogData.TotalExplorationDistance += DistanceMeters;
	float& StageDist = LogData.StageExplorationDistances.FindOrAdd(TargetStage);
	StageDist += DistanceMeters;
}

void UGameplayLogSubsystem::AddBossMovementDistance(const FString& StageName, float DistanceMeters)
{
	if (DistanceMeters <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	LogData.TotalBossDistance += DistanceMeters;
	float& StageDist = LogData.StageBossDistances.FindOrAdd(TargetStage);
	StageDist += DistanceMeters;
}

void UGameplayLogSubsystem::AddEtcMovementDistance(const FString& StageName, float DistanceMeters)
{
	if (DistanceMeters <= 0.0f) return;

	FString TargetStage = GetTargetStageName(StageName);
	LogData.TotalEtcDistance += DistanceMeters;
	float& StageDist = LogData.StageEtcDistances.FindOrAdd(TargetStage);
	StageDist += DistanceMeters;
}

void UGameplayLogSubsystem::IncrementStageInteraction(const FString& StageName, const FString& ActorName)
{
	// 1. FInteractionCountMap 구조체를 FindOrAdd로 가져옵니다.
	FInteractionCountMap& InteractionMap = LogData.StageInteractionCounts.FindOrAdd(StageName);

	// 2. 구조체 내부의 ActorCounts TMap에 카운트 누적
	int32& Count = InteractionMap.ActorCounts.FindOrAdd(ActorName);
	Count++;
}

void UGameplayLogSubsystem::RecordStageFallRespawn(const FString& StageName, FVector FallLocation)
{
	FString TargetStage = GetTargetStageName(StageName);
	int32& Count = LogData.StageFallRespawnCounts.FindOrAdd(TargetStage);
	Count++;
	LogData.FallRespawnPositions.Add(FallLocation);

	// 맵별 체크포인트 총 재시도 횟수
	LogData.StageCheckpointRetryCounts.FindOrAdd(TargetStage)++;

	// 현재 진행 중이던 체크포인트 구간별 재시도 횟수
	int32 CurrentCp = LogData.MaxReachedCheckpointIndex.FindRef(TargetStage);
	FString SectionKey = FString::Printf(TEXT("%s_CP%d"), *TargetStage, CurrentCp);
	LogData.CheckpointSectionRetryCounts.FindOrAdd(SectionKey)++;
}

void UGameplayLogSubsystem::IncrementTutorialFullSkip()
{
	LogData.TutorialFullSkipCount++;
}

void UGameplayLogSubsystem::IncrementDialogueFullSkip(const FString& StageName)
{
	LogData.DialogueFullSkipCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageDialogueFullSkipCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementDialogueLineSkip(const FString& StageName)
{
	LogData.DialogueLineSkipCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageDialogueLineSkipCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementDialogueLineRead(const FString& StageName)
{
	LogData.DialogueLineReadCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageDialogueLineReadCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementOptionChange(const FString& StageName)
{
	LogData.OptionChangeCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageOptionChangeCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::IncrementDialogueLogRecheck(const FString& StageName)
{
	LogData.DialogueLogRecheckCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageDialogueLogRecheckCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::RecordPlayerDeath(const FString& StageName, const FString& DeathReason, FVector DeathLocation)
{
	FString TargetStage = GetTargetStageName(StageName);
	int32& Count = LogData.DeathReasonCounts.FindOrAdd(DeathReason);
	Count++;
	LogData.DeathPositions.Add(DeathLocation);

	// 맵별 총 플레이어 사망 횟수
	LogData.StageTotalDeathCounts.FindOrAdd(TargetStage)++;

	// 맵별 체크포인트 총 재시도 횟수
	LogData.StageCheckpointRetryCounts.FindOrAdd(TargetStage)++;

	// 현재 진행 중이던 체크포인트 구간별 재시도 횟수
	int32 CurrentCp = LogData.MaxReachedCheckpointIndex.FindRef(TargetStage);
	FString SectionKey = FString::Printf(TEXT("%s_CP%d"), *TargetStage, CurrentCp);
	LogData.CheckpointSectionRetryCounts.FindOrAdd(SectionKey)++;
}

void UGameplayLogSubsystem::IncrementStageJump(const FString& StageName)
{
	int32& Count = LogData.StageJumpCounts.FindOrAdd(StageName);
	Count++;
}

void UGameplayLogSubsystem::IncrementStageDash(const FString& StageName)
{
	int32& Count = LogData.StageDashCounts.FindOrAdd(StageName);
	Count++;
}

void UGameplayLogSubsystem::IncrementStageGimmickClear(const FString& StageName, const FString& GimmickType)
{
	int32& Count = LogData.StageGimmickClearCounts.FindOrAdd(StageName);
	Count++;

	if (!GimmickType.IsEmpty())
	{
		int32& GimmickCount = LogData.GimmickClearCounts.FindOrAdd(GimmickType);
		GimmickCount++;
	}
}

void UGameplayLogSubsystem::RecordGimmickStart(const FString& GimmickName, const FString& StageName, float TimeSeconds)
{
	FString TargetStage = GetTargetStageName(StageName);
	LogData.GimmickStartTimes.Add(GimmickName, TimeSeconds);
}

void UGameplayLogSubsystem::RecordGimmickClear(const FString& GimmickName, const FString& StageName, float TimeSeconds)
{
	FString TargetStage = GetTargetStageName(StageName);
	if (TimeSeconds > 0.0f)
	{
		LogData.GimmickClearTimes.Add(GimmickName, TimeSeconds);
	}
	IncrementStageGimmickClear(TargetStage, GimmickName);
}

void UGameplayLogSubsystem::IncrementBossPatternDodge(const FString& StageName)
{
	int32& Count = LogData.BossPatternDodgeCounts.FindOrAdd(StageName);
	Count++;
}

void UGameplayLogSubsystem::RecordBossEnter(const FString& BossName)
{
	int32& Count = LogData.BossEnterCounts.FindOrAdd(BossName);
	Count++;

	float CurrentWorldTime = 0.0f;
	if (UWorld* World = GetWorld())
	{
		CurrentWorldTime = World->GetTimeSeconds();
	}

	LogData.ActiveBossBattleStartTimes.Add(BossName, CurrentWorldTime);

	FString TargetStage = GetTargetStageName();
	LogData.ActiveBossBattleStartTimes.Add(TargetStage, CurrentWorldTime);

	// ⚔️ 실시간 보스전 활성화 (이 순간부터 맵 플레이 시간이 보스 시간으로 전환 누적됨)
	bIsBossBattleActive = true;
	CurrentActiveBossName = BossName;
}

void UGameplayLogSubsystem::RecordBossDeath(const FString& BossName)
{
	int32& Count = LogData.BossDeathCounts.FindOrAdd(BossName);
	Count++;

	// 플레이어 사망 시 보스전 타이머 초기화 (재도전 시 재측정)
	LogData.ActiveBossBattleStartTimes.Remove(BossName);
	FString TargetStage = GetTargetStageName();
	LogData.ActiveBossBattleStartTimes.Remove(TargetStage);

	// 🛡️ 보스전 비활성화 (사망 후 리스폰/재도전 전까지는 탐험 시간으로 복귀)
	bIsBossBattleActive = false;
	CurrentActiveBossName.Empty();
}

void UGameplayLogSubsystem::RecordBossClear(const FString& BossName, float BattleTimeSeconds, int32 LootAcquiredCount)
{
	int32& ClearCount = LogData.BossClearCounts.FindOrAdd(BossName);
	ClearCount++;

	float FinalDuration = BattleTimeSeconds;
	if (FinalDuration <= 0.0f)
	{
		float CurrentWorldTime = 0.0f;
		if (UWorld* World = GetWorld())
		{
			CurrentWorldTime = World->GetTimeSeconds();
		}

		// 1. 정확한 보스명 키로 검색
		float* StartTime = LogData.ActiveBossBattleStartTimes.Find(BossName);

		// 2. 현재 스테이지 이름으로 검색
		if (!StartTime)
		{
			FString TargetStage = GetTargetStageName();
			StartTime = LogData.ActiveBossBattleStartTimes.Find(TargetStage);
		}

		// 3. 부분 매칭으로 검색 (예: Forest_Boss vs BP_Boss_SkeletonMage)
		if (!StartTime)
		{
			for (const auto& Pair : LogData.ActiveBossBattleStartTimes)
			{
				if (BossName.Contains(Pair.Key) || Pair.Key.Contains(BossName) ||
					(BossName.Contains(TEXT("SkeletonMage")) && Pair.Key.Contains(TEXT("Forest"))) ||
					(BossName.Contains(TEXT("Assassin")) && Pair.Key.Contains(TEXT("Dungeon"))))
				{
					StartTime = const_cast<float*>(&Pair.Value);
					break;
				}
			}
		}

		if (StartTime)
		{
			FinalDuration = FMath::Max(0.0f, CurrentWorldTime - *StartTime);
		}
		else
		{
			// 진입 타이머가 없는 경우, 직전 체크포인트 도달 시각과의 차이를 차선책으로 활용
			FString TargetStage = GetTargetStageName();
			float* LastCpTime = LogData.LastCheckpointTimes.Find(TargetStage);
			if (LastCpTime)
			{
				FinalDuration = FMath::Max(0.0f, CurrentWorldTime - *LastCpTime);
			}
		}
	}

	LogData.BossBattleTimes.Add(BossName, FinalDuration);
	LogData.BossLootAcquiredCounts.Add(BossName, LootAcquiredCount);

	LogData.ActiveBossBattleStartTimes.Remove(BossName);
	FString TargetStage = GetTargetStageName();
	LogData.ActiveBossBattleStartTimes.Remove(TargetStage);

	// 🛡️ 보스전 비활성화 및 해당 스테이지 보스 처치 완료 등록 (이후 포탈 진입 전까지의 체류 시간은 '기타' 시간으로 누적)
	bIsBossBattleActive = false;
	CurrentActiveBossName.Empty();
	ClearedBossStages.Add(TargetStage);
	LogData.BossMinHealthRatios.Add(BossName, 0.0f);
	LogData.BossMinHealthRatios.Add(TargetStage, 0.0f);

	// 🏆 최초 1회차 완주 스냅샷 자동 기록
	SnapshotStageFirstClear(TargetStage, BossName);
}

void UGameplayLogSubsystem::UpdateBossHealthRatio(const FString& BossName, float CurrentHealthRatio)
{
	float ClampedRatio = FMath::Clamp(CurrentHealthRatio, 0.0f, 1.0f);
	FString TargetStage = GetTargetStageName();

	// 보스명 및 스테이지명 기준으로 최저 잔여 체력 비율 갱신
	float* ExistingBossRatio = LogData.BossMinHealthRatios.Find(BossName);
	if (ExistingBossRatio)
	{
		*ExistingBossRatio = FMath::Min(*ExistingBossRatio, ClampedRatio);
	}
	else
	{
		LogData.BossMinHealthRatios.Add(BossName, ClampedRatio);
	}

	float* ExistingStageRatio = LogData.BossMinHealthRatios.Find(TargetStage);
	if (ExistingStageRatio)
	{
		*ExistingStageRatio = FMath::Min(*ExistingStageRatio, ClampedRatio);
	}
	else
	{
		LogData.BossMinHealthRatios.Add(TargetStage, ClampedRatio);
	}
}

void UGameplayLogSubsystem::SnapshotStageFirstClear(const FString& StageName, const FString& BossName)
{
	FString TargetStage = GetTargetStageName(StageName);

	// 🛡️ 이미 최초 1회차 스냅샷이 기록되어 있다면 영구 보존 (2회차 이후 재진입 시 덮어쓰기 방지)
	if (LogData.StageFirstClearTimes.Contains(TargetStage))
	{
		return;
	}

	float CurrentPlayTime = LogData.StagePlayTimes.FindRef(TargetStage);
	float CurrentExplorationTime = LogData.StageExplorationTimes.FindRef(TargetStage);
	float CurrentBossTime = LogData.StageBossTimes.FindRef(TargetStage);
	float CurrentEtcTime = LogData.StageEtcTimes.FindRef(TargetStage);
	float CurrentDistance = LogData.StageMovementDistances.FindRef(TargetStage);
	float CurrentExplorationDistance = LogData.StageExplorationDistances.FindRef(TargetStage);
	float CurrentBossDistance = LogData.StageBossDistances.FindRef(TargetStage);
	float CurrentEtcDistance = LogData.StageEtcDistances.FindRef(TargetStage);
	float CurrentDamageTaken = LogData.StageDamageTaken.FindRef(TargetStage);
	int32 CurrentFallRespawns = LogData.StageFallRespawnCounts.FindRef(TargetStage);
	int32 CurrentDeaths = LogData.StageTotalDeathCounts.FindRef(TargetStage);
	int32 CurrentMonsterKills = LogData.StageTotalMonsterKillCounts.FindRef(TargetStage);

	LogData.StageFirstClearTimes.Add(TargetStage, CurrentPlayTime);
	LogData.StageFirstClearExplorationTimes.Add(TargetStage, CurrentExplorationTime);
	LogData.StageFirstClearBossTimes.Add(TargetStage, CurrentBossTime);
	LogData.StageFirstClearEtcTimes.Add(TargetStage, CurrentEtcTime);
	LogData.StageFirstClearDistances.Add(TargetStage, CurrentDistance);
	LogData.StageFirstClearExplorationDistances.Add(TargetStage, CurrentExplorationDistance);
	LogData.StageFirstClearBossDistances.Add(TargetStage, CurrentBossDistance);
	LogData.StageFirstClearEtcDistances.Add(TargetStage, CurrentEtcDistance);
	LogData.StageFirstClearDamageTaken.Add(TargetStage, CurrentDamageTaken);
	LogData.StageFirstClearFallRespawns.Add(TargetStage, CurrentFallRespawns);
	LogData.StageFirstClearDeaths.Add(TargetStage, CurrentDeaths);
	LogData.StageFirstClearMonsterKills.Add(TargetStage, CurrentMonsterKills);

	// 🚩 체크포인트 구간 소요 시간 & 이동거리 1회차 스냅샷 기록
	for (const auto& Pair : LogData.StageCheckpointSectionTimes)
	{
		if (Pair.Key.StartsWith(TargetStage) || Pair.Key.Contains(TargetStage))
		{
			LogData.StageFirstClearCheckpointSectionTimes.Add(Pair.Key, Pair.Value);
		}
	}
	for (const auto& Pair : LogData.StageCheckpointSectionDistances)
	{
		if (Pair.Key.StartsWith(TargetStage) || Pair.Key.Contains(TargetStage))
		{
			LogData.StageFirstClearCheckpointSectionDistances.Add(Pair.Key, Pair.Value);
		}
	}

	// 🏰 던전 기믹 방 소요 시간 & 이동거리 1회차 스냅샷 기록
	for (const auto& Pair : LogData.StageGimmickRoomTimes)
	{
		if (Pair.Key.StartsWith(TargetStage) || Pair.Key.Contains(TargetStage))
		{
			LogData.StageFirstClearGimmickRoomTimes.Add(Pair.Key, Pair.Value);
		}
	}
	for (const auto& Pair : LogData.StageGimmickRoomDistances)
	{
		if (Pair.Key.StartsWith(TargetStage) || Pair.Key.Contains(TargetStage))
		{
			LogData.StageFirstClearGimmickRoomDistances.Add(Pair.Key, Pair.Value);
		}
	}

	if (!BossName.IsEmpty())
	{
		float BossTime = LogData.BossBattleTimes.FindRef(BossName);
		if (BossTime <= 0.0f)
		{
			for (const auto& Pair : LogData.BossBattleTimes)
			{
				if (Pair.Key.Contains(BossName) || BossName.Contains(Pair.Key) ||
					(BossName.Contains(TEXT("SkeletonMage")) && Pair.Key.Contains(TEXT("Forest"))) ||
					(BossName.Contains(TEXT("Assassin")) && Pair.Key.Contains(TEXT("Dungeon"))))
				{
					BossTime = Pair.Value;
					break;
				}
			}
		}
		if (BossTime > 0.0f)
		{
			LogData.FirstBossBattleTimes.Add(BossName, BossTime);
		}
	}
}

void UGameplayLogSubsystem::RecordPotionUsage(const FString& StageName, const FString& PotionType)
{
	FString TargetStage = GetTargetStageName(StageName);
	int32& Count = LogData.PotionUsagePerMap.FindOrAdd(TargetStage);
	Count++;

	if (!PotionType.IsEmpty())
	{
		int32& TypeCount = LogData.PotionUsageByType.FindOrAdd(PotionType);
		TypeCount++;
	}
}

void UGameplayLogSubsystem::UpdateLatestHealthPercent(float HealthPercent)
{
	LogData.LatestPlayerHealthPercent = HealthPercent;
}

void UGameplayLogSubsystem::IncrementTributeAltarUsage(const FString& StageName)
{
	LogData.TributeAltarUsageCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageTributeAltarUsageCounts.FindOrAdd(TargetStage);
	StageCount++;
}

void UGameplayLogSubsystem::AddStageDamageDealt(const FString& StageName, float Damage)
{
	float& Dmg = LogData.StageDamageDealt.FindOrAdd(StageName);
	Dmg += Damage;
}

void UGameplayLogSubsystem::AddStageDamageTaken(const FString& StageName, float Damage)
{
	float& Dmg = LogData.StageDamageTaken.FindOrAdd(StageName);
	Dmg += Damage;
}

// ==========================================
// 📌 누적 막대 그래프(Stacked Bar Chart) 정밀 로깅 함수 구현
// ==========================================

void UGameplayLogSubsystem::RecordCheckpointReach(int32 CheckpointIndex, const FString& StageName, float CustomDeltaSeconds, float CustomDeltaDistance)
{
	FString TargetStage = GetTargetStageName(StageName);
	int32& MaxReached = LogData.MaxReachedCheckpointIndex.FindOrAdd(TargetStage);

	// 🛡️ 방어 로직: 이미 도달했던 이전 체크포인트로 역주행/재방문한 경우 시간/거리 왜곡 및 중복 덮어쓰기 방지
	if (CheckpointIndex <= MaxReached && MaxReached > 0 && CustomDeltaSeconds <= 0.0f && CustomDeltaDistance <= 0.0f)
	{
		return;
	}

	float CurrentWorldTime = 0.0f;
	if (UWorld* World = GetWorld())
	{
		CurrentWorldTime = World->GetTimeSeconds();
	}

	float SectionDuration = CustomDeltaSeconds;
	if (SectionDuration <= 0.0f)
	{
		float* LastTime = LogData.LastCheckpointTimes.Find(TargetStage);
		if (LastTime)
		{
			SectionDuration = FMath::Max(0.0f, CurrentWorldTime - *LastTime);
		}
		else
		{
			// 첫 체크포인트 도달 시 스테이지 시작 이후 누적 시간 사용
			float* StagePlayTime = LogData.StagePlayTimes.Find(TargetStage);
			SectionDuration = StagePlayTime ? *StagePlayTime : CurrentWorldTime;
		}
	}

	// 📏 구간 이동거리 자동 계산
	float CurrentDistance = LogData.StageMovementDistances.FindRef(TargetStage);
	float SectionDistance = CustomDeltaDistance;
	if (SectionDistance <= 0.0f)
	{
		float* LastDist = LogData.LastCheckpointDistances.Find(TargetStage);
		if (LastDist)
		{
			SectionDistance = FMath::Max(0.0f, CurrentDistance - *LastDist);
		}
		else
		{
			SectionDistance = CurrentDistance;
		}
	}

	LogData.LastCheckpointTimes.Add(TargetStage, CurrentWorldTime);
	LogData.LastCheckpointDistances.Add(TargetStage, CurrentDistance);
	MaxReached = FMath::Max(MaxReached, CheckpointIndex);

	FString Key = FString::Printf(TEXT("%s_CP%d"), *TargetStage, CheckpointIndex);
	LogData.StageCheckpointSectionTimes.Add(Key, SectionDuration);
	LogData.StageCheckpointSectionDistances.Add(Key, SectionDistance);
}

bool UGameplayLogSubsystem::IsGimmickRoomCleared(const FString& RoomName, const FString& StageName) const
{
	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *RoomName);
	const bool* bCleared = LogData.ClearedGimmickRooms.Find(Key);
	return bCleared ? *bCleared : false;
}

void UGameplayLogSubsystem::StartGimmickRoom(const FString& RoomName, const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *RoomName);

	// 🛡️ 방어 로직: 이미 클리어 완료된 기믹 방에 다시 들어간 경우 풀이 시간/거리 재측정 방지
	if (IsGimmickRoomCleared(RoomName, TargetStage))
	{
		return;
	}

	float CurrentWorldTime = 0.0f;
	if (UWorld* World = GetWorld())
	{
		CurrentWorldTime = World->GetTimeSeconds();
	}

	float CurrentDistance = LogData.StageMovementDistances.FindRef(TargetStage);

	LogData.ActiveGimmickRoomStartTimes.Add(Key, CurrentWorldTime);
	LogData.ActiveGimmickRoomStartDistances.Add(Key, CurrentDistance);
}

void UGameplayLogSubsystem::EndGimmickRoom(const FString& RoomName, const FString& StageName, bool bMarkAsCleared, float CustomDurationSeconds, float CustomDistanceMeters)
{
	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *RoomName);

	// 🛡️ 방어 로직: 이미 클리어된 방이면 무시
	if (IsGimmickRoomCleared(RoomName, TargetStage) && CustomDurationSeconds <= 0.0f && CustomDistanceMeters <= 0.0f)
	{
		return;
	}

	float Duration = CustomDurationSeconds;
	if (Duration <= 0.0f)
	{
		float* StartTime = LogData.ActiveGimmickRoomStartTimes.Find(Key);
		if (StartTime && GetWorld())
		{
			Duration = FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - *StartTime);
			LogData.ActiveGimmickRoomStartTimes.Remove(Key);
		}
	}

	float Distance = CustomDistanceMeters;
	if (Distance <= 0.0f)
	{
		float* StartDist = LogData.ActiveGimmickRoomStartDistances.Find(Key);
		if (StartDist)
		{
			float CurrentDist = LogData.StageMovementDistances.FindRef(TargetStage);
			Distance = FMath::Max(0.0f, CurrentDist - *StartDist);
			LogData.ActiveGimmickRoomStartDistances.Remove(Key);
		}
	}

	LogData.StageGimmickRoomTimes.FindOrAdd(Key) += Duration;
	LogData.StageGimmickRoomDistances.FindOrAdd(Key) += Distance;

	if (bMarkAsCleared)
	{
		LogData.ClearedGimmickRooms.Add(Key, true);
	}
}

void UGameplayLogSubsystem::RecordGimmickRoomTime(const FString& RoomName, float DurationSeconds, const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *RoomName);
	LogData.StageGimmickRoomTimes.FindOrAdd(Key) += DurationSeconds;
}

void UGameplayLogSubsystem::RecordGimmickRoomDistance(const FString& RoomName, float DistanceMeters, const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *RoomName);
	LogData.StageGimmickRoomDistances.FindOrAdd(Key) += DistanceMeters;
}

void UGameplayLogSubsystem::AddStageActionDuration(const FString& ActionCategory, float DeltaSeconds, const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *ActionCategory);
	float& TotalDuration = LogData.StageActionDurations.FindOrAdd(Key);
	TotalDuration += DeltaSeconds;
}

void UGameplayLogSubsystem::IncrementSuccessfulPotionCrafting(int32 Amount)
{
	LogData.SuccessfulPotionCraftingCount += Amount;
}

void UGameplayLogSubsystem::RecordPotionCrafted(const FString& PotionItemID)
{
	if (!PotionItemID.IsEmpty())
	{
		int32& Count = LogData.CraftedPotionTypeCounts.FindOrAdd(PotionItemID);
		Count++;
	}
}

void UGameplayLogSubsystem::SetTargetCropTypeCount(int32 Count)
{
	if (Count > 0)
	{
		LogData.TotalTargetCropTypes = Count;
	}
}

void UGameplayLogSubsystem::SetTargetPotionRecipeCount(int32 Count)
{
	if (Count > 0)
	{
		LogData.TotalTargetPotionRecipes = Count;
	}
}

void UGameplayLogSubsystem::SetMaxTributeSteps(int32 Count)
{
	if (Count > 0)
	{
		LogData.MaxTargetTributeSteps = Count;
	}
}

void UGameplayLogSubsystem::UpdateCurrentTributeLevel(int32 Level)
{
	LogData.CurrentTributeLevel = FMath::Max(LogData.CurrentTributeLevel, Level);
}

void UGameplayLogSubsystem::IncrementSleepCount(const FString& StageName)
{
	FString TargetStage = GetTargetStageName(StageName);
	int32& Count = LogData.StageSleepCounts.FindOrAdd(TargetStage);
	Count++;
}

void UGameplayLogSubsystem::SetGameCompleted(bool bCompleted)
{
	LogData.bIsGameCompleted = bCompleted;
}

// ==========================================
void UGameplayLogSubsystem::Deinitialize()
{
	// 게임 종료 또는 GameInstance 해제 시 자동으로 CSV 내보내기 수행
	ExportLogsToCSVFile(TEXT(""));
	Super::Deinitialize();
}

// ==========================================
// 📌 CSV 내보내기 (구글 시트 연동)
// ==========================================

FString UGameplayLogSubsystem::GenerateCSVString() const
{
	FString SessionID = LogData.PlayerSessionID.IsEmpty() ? TEXT("Unknown") : LogData.PlayerSessionID;
	FString CSV = FString::Printf(TEXT("PlayerSessionID,Category,MetricName,Key,Value\n"));

	// 1. 단일 수치 지표 (General, Tutorial, UI, Reward)
	CSV += FString::Printf(TEXT("%s,General,TotalPlayTime,,%.2f\n"), *SessionID, LogData.TotalPlayTime);
	CSV += FString::Printf(TEXT("%s,General,TotalExplorationTime,,%.2f\n"), *SessionID, LogData.TotalExplorationTime);
	CSV += FString::Printf(TEXT("%s,General,TotalBossTime,,%.2f\n"), *SessionID, LogData.TotalBossTime);
	CSV += FString::Printf(TEXT("%s,General,TotalEtcTime,,%.2f\n"), *SessionID, LogData.TotalEtcTime);
	CSV += FString::Printf(TEXT("%s,General,TotalExplorationDistance,,%.2f\n"), *SessionID, LogData.TotalExplorationDistance);
	CSV += FString::Printf(TEXT("%s,General,TotalBossDistance,,%.2f\n"), *SessionID, LogData.TotalBossDistance);
	CSV += FString::Printf(TEXT("%s,General,TotalEtcDistance,,%.2f\n"), *SessionID, LogData.TotalEtcDistance);
	CSV += FString::Printf(TEXT("%s,General,TotalAcquiredCoins,,%d\n"), *SessionID, LogData.TotalAcquiredCoins);
	CSV += FString::Printf(TEXT("%s,General,TotalConsumedCoins,,%d\n"), *SessionID, LogData.TotalConsumedCoins);
	CSV += FString::Printf(TEXT("%s,General,FailedPotionCraftingCount,,%d\n"), *SessionID, LogData.FailedPotionCraftingCount);
	CSV += FString::Printf(TEXT("%s,General,PauseCount,,%d\n"), *SessionID, LogData.PauseCount);
	CSV += FString::Printf(TEXT("%s,General,TotalItemsAcquired,,%d\n"), *SessionID, LogData.TotalItemsAcquired);
	CSV += FString::Printf(TEXT("%s,General,TotalItemsDiscarded,,%d\n"), *SessionID, LogData.TotalItemsDiscarded);
	CSV += FString::Printf(TEXT("%s,General,InventoryFullOccurrenceCount,,%d\n"), *SessionID, LogData.InventoryFullOccurrenceCount);
	CSV += FString::Printf(TEXT("%s,General,GuardUsageCount,,%d\n"), *SessionID, LogData.GuardUsageCount);
	CSV += FString::Printf(TEXT("%s,General,TotalDamageMitigatedByGuard,,%.2f\n"), *SessionID, LogData.TotalDamageMitigatedByGuard);
	CSV += FString::Printf(TEXT("%s,General,TotalCropGrowthDelayDueToWeeds,,%.2f\n"), *SessionID, LogData.TotalCropGrowthDelayDueToWeeds);
	CSV += FString::Printf(TEXT("%s,General,bIsGameCompleted,,%s\n"), *SessionID, LogData.bIsGameCompleted ? TEXT("True") : TEXT("False"));

	CSV += FString::Printf(TEXT("%s,Tutorial,FullSkipCount,,%d\n"), *SessionID, LogData.TutorialFullSkipCount);
	CSV += FString::Printf(TEXT("%s,Tutorial,DialogueFullSkipCount,,%d\n"), *SessionID, LogData.DialogueFullSkipCount);
	CSV += FString::Printf(TEXT("%s,Tutorial,DialogueLineSkipCount,,%d\n"), *SessionID, LogData.DialogueLineSkipCount);
	CSV += FString::Printf(TEXT("%s,Tutorial,DialogueLineReadCount,,%d\n"), *SessionID, LogData.DialogueLineReadCount);

	CSV += FString::Printf(TEXT("%s,UI,OptionChangeCount,,%d\n"), *SessionID, LogData.OptionChangeCount);
	CSV += FString::Printf(TEXT("%s,UI,DialogueLogRecheckCount,,%d\n"), *SessionID, LogData.DialogueLogRecheckCount);

	CSV += FString::Printf(TEXT("%s,Reward,TributeAltarUsageCount,,%d\n"), *SessionID, LogData.TributeAltarUsageCount);
	CSV += FString::Printf(TEXT("%s,Reward,SuccessfulPotionCraftingCount,,%d\n"), *SessionID, LogData.SuccessfulPotionCraftingCount);
	CSV += FString::Printf(TEXT("%s,Reward,LatestHealthPercent,,%.2f\n"), *SessionID, LogData.LatestPlayerHealthPercent);

	// 2. 스테이지별 이동 및 행동 지표
	for (const auto& Pair : LogData.StagePlayTimes)
	{
		CSV += FString::Printf(TEXT("%s,StagePlayTime,PlayTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageExplorationTimes)
	{
		CSV += FString::Printf(TEXT("%s,StagePlayTimeCategorized,Exploration,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageBossTimes)
	{
		CSV += FString::Printf(TEXT("%s,StagePlayTimeCategorized,Boss,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageEtcTimes)
	{
		CSV += FString::Printf(TEXT("%s,StagePlayTimeCategorized,Etc,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageClearPortalTimes)
	{
		CSV += FString::Printf(TEXT("%s,StageClearPortalTime,PortalTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageMovementDistances)
	{
		CSV += FString::Printf(TEXT("%s,StageMovementDistance,DistanceMeters,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageExplorationDistances)
	{
		CSV += FString::Printf(TEXT("%s,StageMovementDistanceCategorized,Exploration,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageBossDistances)
	{
		CSV += FString::Printf(TEXT("%s,StageMovementDistanceCategorized,Boss,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageEtcDistances)
	{
		CSV += FString::Printf(TEXT("%s,StageMovementDistanceCategorized,Etc,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& StagePair : LogData.StageInteractionCounts)
	{
		// StagePair.Value.ActorCounts 로 TMap에 접근하여 순회
		for (const auto& ActorPair : StagePair.Value.ActorCounts)
		{
			CSV += FString::Printf(TEXT("%s,StageInteractionCount,Interactions,%s_%s,%d\n"),
				*SessionID,
				*StagePair.Key,
				*ActorPair.Key,
				ActorPair.Value);
		}
	}
	for (const auto& Pair : LogData.StageFallRespawnCounts)
	{
		CSV += FString::Printf(TEXT("%s,StageFallRespawnCount,FallRespawns,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageJumpCounts)
	{
		CSV += FString::Printf(TEXT("%s,StageJumpCount,Jumps,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDashCounts)
	{
		CSV += FString::Printf(TEXT("%s,StageDashCount,Dashes,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.GimmickStartTimes)
	{
		CSV += FString::Printf(TEXT("%s,GimmickStartTime,StartTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.GimmickClearTimes)
	{
		CSV += FString::Printf(TEXT("%s,GimmickClearTime,ClearTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageGimmickClearCounts)
	{
		CSV += FString::Printf(TEXT("%s,StageGimmickClearCount,StageClears,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.GimmickClearCounts)
	{
		CSV += FString::Printf(TEXT("%s,GimmickClearCount,GimmickClears,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossPatternDodgeCounts)
	{
		CSV += FString::Printf(TEXT("%s,BossPatternDodgeCount,PatternDodges,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 3. 전투 및 보스전 지표
	for (const auto& Pair : LogData.BossEnterCounts)
	{
		CSV += FString::Printf(TEXT("%s,BossEnterCount,BossEnters,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossDeathCounts)
	{
		CSV += FString::Printf(TEXT("%s,BossDeathCount,BossDeaths,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossClearCounts)
	{
		CSV += FString::Printf(TEXT("%s,BossClearCount,BossClears,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossBattleTimes)
	{
		CSV += FString::Printf(TEXT("%s,BossBattleTime,BattleSeconds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossLootAcquiredCounts)
	{
		CSV += FString::Printf(TEXT("%s,BossLootCount,LootAmount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.PotionUsagePerMap)
	{
		CSV += FString::Printf(TEXT("%s,PotionUsagePerMap,PotionCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.PotionUsageByType)
	{
		CSV += FString::Printf(TEXT("%s,PotionUsageByType,PotionCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDamageDealt)
	{
		CSV += FString::Printf(TEXT("%s,StageDamageDealt,DamageDealt,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDamageTaken)
	{
		CSV += FString::Printf(TEXT("%s,StageDamageTaken,DamageTaken,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.DeathReasonCounts)
	{
		CSV += FString::Printf(TEXT("%s,DeathReason,Deaths,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.MonsterKillCounts)
	{
		CSV += FString::Printf(TEXT("%s,MonsterKill,Kills,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 4. 거점/파밍/상호작용/콤보 지표
	for (const auto& Pair : LogData.CropHarvestCounts)
	{
		CSV += FString::Printf(TEXT("%s,CropHarvest,HarvestAmount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.ElixirUsagePerCrop)
	{
		CSV += FString::Printf(TEXT("%s,ElixirUsage,ElixirCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.UsedComboCounts)
	{
		CSV += FString::Printf(TEXT("%s,UsedCombo,ComboUsed,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.CompletedComboCounts)
	{
		CSV += FString::Printf(TEXT("%s,CompletedCombo,ComboCompleted,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.NPCDialogueCounts)
	{
		CSV += FString::Printf(TEXT("%s,NPCDialogue,Dialogues,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.MapClearTimes)
	{
		CSV += FString::Printf(TEXT("%s,MapClearTime,ClearSeconds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 5. 맵(Stage)별 정밀 세부 지표
	for (const auto& Pair : LogData.StageGuardCounts)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageGuardCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDamageMitigatedByGuard)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageDamageMitigatedByGuard,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageAcquiredCoins)
	{
		CSV += FString::Printf(TEXT("%s,Economy,StageAcquiredCoins,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageConsumedCoins)
	{
		CSV += FString::Printf(TEXT("%s,Economy,StageConsumedCoins,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageNPCCoinConsumption)
	{
		CSV += FString::Printf(TEXT("%s,Economy,StageNPCCoinConsumption,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (int32 i = 0; i < LogData.NPCTransactions.Num(); ++i)
	{
		const FNPCTransactionRecord& Rec = LogData.NPCTransactions[i];
		CSV += FString::Printf(TEXT("%s,NPCTransaction,Trade_%d,%s_%s_%s_Qty%d,%d\n"),
			*SessionID,
			i + 1,
			*Rec.StageName,
			*Rec.NPCName,
			*Rec.ItemOrDetail,
			Rec.Quantity,
			Rec.Cost);
	}
	for (const auto& Pair : LogData.StageItemsAcquired)
	{
		CSV += FString::Printf(TEXT("%s,Inventory,StageItemsAcquired,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageItemsDiscarded)
	{
		CSV += FString::Printf(TEXT("%s,Inventory,StageItemsDiscarded,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageInventoryFullCounts)
	{
		CSV += FString::Printf(TEXT("%s,Inventory,StageInventoryFullCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StagePauseCounts)
	{
		CSV += FString::Printf(TEXT("%s,UI,StagePauseCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageUsedComboCounts)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageUsedCombo,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageCompletedComboCounts)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageCompletedCombo,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageMonsterKillCounts)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageMonsterKill,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageTotalMonsterKillCounts)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageMonsterKillCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageTotalDeathCounts)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageDeathCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageCheckpointRetryCounts)
	{
		CSV += FString::Printf(TEXT("%s,Stage,StageCheckpointRetryCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.CheckpointSectionRetryCounts)
	{
		CSV += FString::Printf(TEXT("%s,Stage,CheckpointSectionRetryCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageCropHarvestCounts)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageCropHarvestQuantity,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	CSV += FString::Printf(TEXT("%s,Farming,TotalCropHarvestActionCount,,%d\n"), *SessionID, LogData.TotalCropHarvestActionCount);
	for (const auto& Pair : LogData.StageCropHarvestActionCounts)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageCropHarvestActionCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	CSV += FString::Printf(TEXT("%s,Farming,TotalCropPlantCount,,%d\n"), *SessionID, LogData.TotalCropPlantCount);
	CSV += FString::Printf(TEXT("%s,Farming,FirstCropPlantTimeSeconds,,%.2f\n"), *SessionID, LogData.FirstCropPlantTime);
	for (const auto& Pair : LogData.StageCropPlantCounts)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageCropPlantCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (int32 i = 0; i < LogData.CropPlantTimestamps.Num(); ++i)
	{
		CSV += FString::Printf(TEXT("%s,CropPlantTimestamp,PlayTimeSeconds,%d,%.2f\n"), *SessionID, i + 1, LogData.CropPlantTimestamps[i]);
	}

	for (const auto& Pair : LogData.StageElixirUsageCounts)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageElixirUsage,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageCropGrowthDelayDueToWeeds)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageCropGrowthDelayDueToWeeds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageInteractionActionCounts)
	{
		CSV += FString::Printf(TEXT("%s,InteractionAction,Count,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	CSV += FString::Printf(TEXT("%s,Combat,TotalHealthPotionUsageCount,,%d\n"), *SessionID, LogData.TotalHealthPotionUsageCount);
	for (const auto& Pair : LogData.StageHealthPotionUsageCounts)
	{
		CSV += FString::Printf(TEXT("%s,Combat,StageHealthPotionUsageCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageNPCDialogueCounts)
	{
		CSV += FString::Printf(TEXT("%s,NPC,StageNPCDialogue,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageTributeAltarUsageCounts)
	{
		CSV += FString::Printf(TEXT("%s,Reward,StageTributeAltarUsage,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageOptionChangeCounts)
	{
		CSV += FString::Printf(TEXT("%s,UI,StageOptionChange,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDialogueFullSkipCounts)
	{
		CSV += FString::Printf(TEXT("%s,Tutorial,StageDialogueFullSkip,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDialogueLineSkipCounts)
	{
		CSV += FString::Printf(TEXT("%s,Tutorial,StageDialogueLineSkip,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDialogueLineReadCounts)
	{
		CSV += FString::Printf(TEXT("%s,Tutorial,StageDialogueLineRead,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDialogueLogRecheckCounts)
	{
		CSV += FString::Printf(TEXT("%s,UI,StageDialogueLogRecheck,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageSleepCounts)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageSleepCount,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 6. 누적 막대 그래프(Stacked Bar Chart) 정밀 시간 및 이동거리 데이터
	// 6-1. 체크포인트 구간별 소요 시간 & 이동거리 및 기타(Other) 잔여 데이터
	TMap<FString, float> StageTotalCheckpointTimes;
	for (const auto& Pair : LogData.StageCheckpointSectionTimes)
	{
		CSV += FString::Printf(TEXT("%s,CheckpointSectionTime,SectionSeconds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);

		// Key 포맷: {StageName}_CP{Index} (예: Forest_Sky_Island_Streaming_CP1 -> Forest_Sky_Island_Streaming)
		FString StageName;
		int32 CpIndex = Pair.Key.Find(TEXT("_CP"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (CpIndex != INDEX_NONE)
		{
			StageName = Pair.Key.Left(CpIndex);
		}
		else
		{
			StageName = Pair.Key;
		}

		StageTotalCheckpointTimes.FindOrAdd(StageName) += Pair.Value;
	}

	for (const auto& StageTimePair : StageTotalCheckpointTimes)
	{
		const FString& StageName = StageTimePair.Key;
		float TotalCPTime = StageTimePair.Value;

		const float* StageTotalPlayTime = LogData.StagePlayTimes.Find(StageName);
		if (!StageTotalPlayTime)
		{
			for (const auto& PlayTimePair : LogData.StagePlayTimes)
			{
				if (PlayTimePair.Key.Contains(StageName) || StageName.Contains(PlayTimePair.Key))
				{
					StageTotalPlayTime = &PlayTimePair.Value;
					break;
				}
			}
		}

		if (StageTotalPlayTime)
		{
			float StageBossTime = 0.0f;
			for (const auto& BossPair : LogData.BossBattleTimes)
			{
				if (BossPair.Key.Contains(StageName) ||
					(StageName.Contains(TEXT("Forest")) && (BossPair.Key.Contains(TEXT("SkeletonMage")) || BossPair.Key.Contains(TEXT("Forest")))) ||
					(StageName.Contains(TEXT("Dungeon")) && (BossPair.Key.Contains(TEXT("Assassin")) || BossPair.Key.Contains(TEXT("Dungeon")))))
				{
					StageBossTime += BossPair.Value;
				}
			}

			float OtherTime = FMath::Max(0.0f, *StageTotalPlayTime - (TotalCPTime + StageBossTime));
			CSV += FString::Printf(TEXT("%s,CheckpointSectionTime,SectionSeconds,%s_Other,%.2f\n"), *SessionID, *StageName, OtherTime);
		}
	}

	TMap<FString, float> StageTotalCheckpointDistances;
	for (const auto& Pair : LogData.StageCheckpointSectionDistances)
	{
		CSV += FString::Printf(TEXT("%s,CheckpointSectionDistance,SectionMeters,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);

		FString StageName;
		int32 CpIndex = Pair.Key.Find(TEXT("_CP"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (CpIndex != INDEX_NONE)
		{
			StageName = Pair.Key.Left(CpIndex);
		}
		else
		{
			StageName = Pair.Key;
		}

		StageTotalCheckpointDistances.FindOrAdd(StageName) += Pair.Value;
	}

	for (const auto& StageDistPair : StageTotalCheckpointDistances)
	{
		const FString& StageName = StageDistPair.Key;
		float TotalCPDist = StageDistPair.Value;

		const float* StageTotalDistance = LogData.StageMovementDistances.Find(StageName);
		if (!StageTotalDistance)
		{
			for (const auto& DistPair : LogData.StageMovementDistances)
			{
				if (DistPair.Key.Contains(StageName) || StageName.Contains(DistPair.Key))
				{
					StageTotalDistance = &DistPair.Value;
					break;
				}
			}
		}

		if (StageTotalDistance)
		{
			float OtherDist = FMath::Max(0.0f, *StageTotalDistance - TotalCPDist);
			CSV += FString::Printf(TEXT("%s,CheckpointSectionDistance,SectionMeters,%s_Other,%.2f\n"), *SessionID, *StageName, OtherDist);
		}
	}

	// 6-2. 던전 기믹 방별 소요 시간 & 이동거리 및 복도/탐색 잔여 데이터
	TMap<FString, float> StageTotalGimmickTimes;
	for (const auto& Pair : LogData.StageGimmickRoomTimes)
	{
		CSV += FString::Printf(TEXT("%s,GimmickRoomTime,RoomSeconds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);

		FString StageName;
		int32 LastUnderscoreIndex = INDEX_NONE;
		if (Pair.Key.FindLastChar(TEXT('_'), LastUnderscoreIndex))
		{
			StageName = Pair.Key.Left(LastUnderscoreIndex);
		}
		else
		{
			StageName = Pair.Key;
		}
		StageTotalGimmickTimes.FindOrAdd(StageName) += Pair.Value;
	}

	for (const auto& StageTimePair : StageTotalGimmickTimes)
	{
		const FString& StageName = StageTimePair.Key;
		float TotalGimmickTime = StageTimePair.Value;

		const float* StageTotalPlayTime = LogData.StagePlayTimes.Find(StageName);
		if (!StageTotalPlayTime)
		{
			for (const auto& PlayTimePair : LogData.StagePlayTimes)
			{
				if (PlayTimePair.Key.Contains(StageName) || StageName.Contains(PlayTimePair.Key))
				{
					StageTotalPlayTime = &PlayTimePair.Value;
					break;
				}
			}
		}

		if (StageTotalPlayTime)
		{
			float StageBossTime = 0.0f;
			for (const auto& BossPair : LogData.BossBattleTimes)
			{
				if (BossPair.Key.Contains(StageName) ||
					(StageName.Contains(TEXT("Dungeon")) && (BossPair.Key.Contains(TEXT("Assassin")) || BossPair.Key.Contains(TEXT("Dungeon")))))
				{
					StageBossTime += BossPair.Value;
				}
			}
			float CorridorTime = FMath::Max(0.0f, *StageTotalPlayTime - (TotalGimmickTime + StageBossTime));
			CSV += FString::Printf(TEXT("%s,GimmickRoomTime,RoomSeconds,%s_Corridor_Exploration,%.2f\n"), *SessionID, *StageName, CorridorTime);
		}
	}

	TMap<FString, float> StageTotalGimmickDistances;
	for (const auto& Pair : LogData.StageGimmickRoomDistances)
	{
		CSV += FString::Printf(TEXT("%s,GimmickRoomDistance,RoomMeters,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);

		FString StageName;
		int32 LastUnderscoreIndex = INDEX_NONE;
		if (Pair.Key.FindLastChar(TEXT('_'), LastUnderscoreIndex))
		{
			StageName = Pair.Key.Left(LastUnderscoreIndex);
		}
		else
		{
			StageName = Pair.Key;
		}
		StageTotalGimmickDistances.FindOrAdd(StageName) += Pair.Value;
	}

	for (const auto& StageDistPair : StageTotalGimmickDistances)
	{
		const FString& StageName = StageDistPair.Key;
		float TotalGimmickDist = StageDistPair.Value;

		const float* StageTotalDistance = LogData.StageMovementDistances.Find(StageName);
		if (!StageTotalDistance)
		{
			for (const auto& DistPair : LogData.StageMovementDistances)
			{
				if (DistPair.Key.Contains(StageName) || StageName.Contains(DistPair.Key))
				{
					StageTotalDistance = &DistPair.Value;
					break;
				}
			}
		}

		if (StageTotalDistance)
		{
			float CorridorDist = FMath::Max(0.0f, *StageTotalDistance - TotalGimmickDist);
			CSV += FString::Printf(TEXT("%s,GimmickRoomDistance,RoomMeters,%s_Corridor_Exploration,%.2f\n"), *SessionID, *StageName, CorridorDist);
		}
	}

	// 6-3. 맵별 6대 행동 소요 시간 (Movement, Airborne, Combat, Interaction, PuzzleOrGimmick, UI_Pause)
	for (const auto& Pair : LogData.StageActionDurations)
	{
		CSV += FString::Printf(TEXT("%s,ActionDuration,DurationSeconds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 7. 공간 히트맵 좌표 데이터
	for (const auto& Pair : LogData.PlayerPositionsAtDateChange)
	{
		CSV += FString::Printf(TEXT("%s,PlayerPositionAtDateChange,Day_%d,%s,1\n"), *SessionID, Pair.Key, *Pair.Value.ToString());
	}
	for (int32 i = 0; i < LogData.DeathPositions.Num(); ++i)
	{
		CSV += FString::Printf(TEXT("%s,DeathPosition,DeathIndex_%d,%s,1\n"), *SessionID, i + 1, *LogData.DeathPositions[i].ToString());
	}
	for (int32 i = 0; i < LogData.FallRespawnPositions.Num(); ++i)
	{
		CSV += FString::Printf(TEXT("%s,FallRespawnPosition,FallIndex_%d,%s,1\n"), *SessionID, i + 1, *LogData.FallRespawnPositions[i].ToString());
	}

	// 8. 최초 1회차 완주 스냅샷 데이터 (First Clear Snapshot - 영구 보존)
	for (const auto& Pair : LogData.StageFirstClearTimes)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,ClearTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearExplorationTimes)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,ExplorationTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearBossTimes)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,BossTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearEtcTimes)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,EtcTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearDistances)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,MovementDistance,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearExplorationDistances)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,ExplorationDistance,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearBossDistances)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,BossDistance,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearEtcDistances)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,EtcDistance,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearDamageTaken)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,DamageTaken,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearFallRespawns)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,FallRespawns,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearDeaths)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,MonsterDeaths,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearMonsterKills)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,MonsterKills,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.FirstBossBattleTimes)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,BossBattleTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearCheckpointSectionTimes)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,CheckpointSectionTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 1회차 체크포인트 잔여 기타(Other) 소요 시간 산출
	TMap<FString, float> FirstClearTotalCPTimes;
	for (const auto& Pair : LogData.StageFirstClearCheckpointSectionTimes)
	{
		FString StageName;
		int32 CpIndex = Pair.Key.Find(TEXT("_CP"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (CpIndex != INDEX_NONE)
		{
			StageName = Pair.Key.Left(CpIndex);
		}
		else
		{
			StageName = Pair.Key;
		}
		FirstClearTotalCPTimes.FindOrAdd(StageName) += Pair.Value;
	}
	for (const auto& StageTimePair : FirstClearTotalCPTimes)
	{
		const FString& StageName = StageTimePair.Key;
		float TotalCPTime = StageTimePair.Value;

		const float* StageFirstClearTime = LogData.StageFirstClearTimes.Find(StageName);
		if (!StageFirstClearTime)
		{
			for (const auto& PlayTimePair : LogData.StageFirstClearTimes)
			{
				if (PlayTimePair.Key.Contains(StageName) || StageName.Contains(PlayTimePair.Key))
				{
					StageFirstClearTime = &PlayTimePair.Value;
					break;
				}
			}
		}

		if (StageFirstClearTime)
		{
			float StageBossTime = 0.0f;
			for (const auto& BossPair : LogData.FirstBossBattleTimes)
			{
				if (BossPair.Key.Contains(StageName) ||
					(StageName.Contains(TEXT("Forest")) && (BossPair.Key.Contains(TEXT("SkeletonMage")) || BossPair.Key.Contains(TEXT("Forest")))) ||
					(StageName.Contains(TEXT("Dungeon")) && (BossPair.Key.Contains(TEXT("Assassin")) || BossPair.Key.Contains(TEXT("Dungeon")))))
				{
					StageBossTime += BossPair.Value;
				}
			}

			float OtherTime = FMath::Max(0.0f, *StageFirstClearTime - (TotalCPTime + StageBossTime));
			CSV += FString::Printf(TEXT("%s,FirstClear,CheckpointSectionTime,%s_Other,%.2f\n"), *SessionID, *StageName, OtherTime);
		}
	}

	for (const auto& Pair : LogData.StageFirstClearCheckpointSectionDistances)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,CheckpointSectionDistance,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 1회차 체크포인트 잔여 기타(Other) 이동거리 산출
	TMap<FString, float> FirstClearTotalCPDistances;
	for (const auto& Pair : LogData.StageFirstClearCheckpointSectionDistances)
	{
		FString StageName;
		int32 CpIndex = Pair.Key.Find(TEXT("_CP"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (CpIndex != INDEX_NONE)
		{
			StageName = Pair.Key.Left(CpIndex);
		}
		else
		{
			StageName = Pair.Key;
		}
		FirstClearTotalCPDistances.FindOrAdd(StageName) += Pair.Value;
	}
	for (const auto& StageDistPair : FirstClearTotalCPDistances)
	{
		const FString& StageName = StageDistPair.Key;
		float TotalCPDist = StageDistPair.Value;

		const float* StageFirstDist = LogData.StageFirstClearDistances.Find(StageName);
		if (!StageFirstDist)
		{
			for (const auto& DistPair : LogData.StageFirstClearDistances)
			{
				if (DistPair.Key.Contains(StageName) || StageName.Contains(DistPair.Key))
				{
					StageFirstDist = &DistPair.Value;
					break;
				}
			}
		}

		if (StageFirstDist)
		{
			float OtherDist = FMath::Max(0.0f, *StageFirstDist - TotalCPDist);
			CSV += FString::Printf(TEXT("%s,FirstClear,CheckpointSectionDistance,%s_Other,%.2f\n"), *SessionID, *StageName, OtherDist);
		}
	}

	for (const auto& Pair : LogData.StageFirstClearGimmickRoomTimes)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,GimmickRoomTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFirstClearGimmickRoomDistances)
	{
		CSV += FString::Printf(TEXT("%s,FirstClear,GimmickRoomDistance,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 9. 📌 8+1대 행동 횟수 & 행동 시간 1:1 매칭 데이터 (숲/던전 6종 + 거점 2종 + 기타/대기 1종)
	TSet<FString> AllRecordedStages;
	for (const auto& Pair : LogData.StagePlayTimes) { AllRecordedStages.Add(Pair.Key); }
	for (const auto& Pair : LogData.StageActionDurations)
	{
		int32 LastUnder = INDEX_NONE;
		if (Pair.Key.FindLastChar(TEXT('_'), LastUnder))
		{
			AllRecordedStages.Add(Pair.Key.Left(LastUnder));
		}
	}

	for (const FString& Stage : AllRecordedStages)
	{
		if (Stage.IsEmpty()) continue;

		// 1. 달리기 (Sprint)
		int32 Count_Sprint = LogData.StageDashCounts.FindRef(Stage);
		float Duration_Sprint = LogData.StageActionDurations.FindRef(Stage + TEXT("_Sprint"));

		// 2. 공중 체공 (Airborne)
		int32 Count_Airborne = LogData.StageJumpCounts.FindRef(Stage);
		float Duration_Airborne = LogData.StageActionDurations.FindRef(Stage + TEXT("_Airborne"));

		// 3. 전투 행동 (Combat)
		int32 Count_Combat = LogData.StageGuardCounts.FindRef(Stage);
		for (const auto& ComboPair : LogData.StageUsedComboCounts)
		{
			if (ComboPair.Key.StartsWith(Stage))
			{
				Count_Combat += ComboPair.Value;
			}
		}
		float Duration_Combat = LogData.StageActionDurations.FindRef(Stage + TEXT("_Combat"));

		// 문 / 스위치 / 레버 / 기믹 조작 (오브젝트 조작은 순간 상호작용이므로 기타로 합산)
		int32 Count_Object = LogData.StageGimmickClearCounts.FindRef(Stage);
		int32 ChestOpenCount = 0;
		if (const FInteractionCountMap* MapPtr = LogData.StageInteractionCounts.Find(Stage))
		{
			for (const auto& ActorCountPair : MapPtr->ActorCounts)
			{
				const FString& Name = ActorCountPair.Key;
				if (Name.Contains(TEXT("Chest")))
				{
					ChestOpenCount += ActorCountPair.Value;
				}
				else if (Name.Contains(TEXT("Door")) || Name.Contains(TEXT("Switch")) ||
					Name.Contains(TEXT("Gimmick")) || Name.Contains(TEXT("Lever")) || Name.Contains(TEXT("Object")))
				{
					Count_Object += ActorCountPair.Value;
				}
			}
		}
		float Duration_Object = LogData.StageActionDurations.FindRef(Stage + TEXT("_GimmickObject"));

		// 4. 아이템 및 인벤토리 관리 (Item & Inventory: 아이템 줍기 + 상자 열기 + 인벤토리 열기)
		int32 ItemPickupCount = LogData.StageItemsAcquired.FindRef(Stage);
		int32 InventoryOpenCount = LogData.StageInventoryOpenCounts.FindRef(Stage);
		int32 Count_ItemAndInventory = ItemPickupCount + ChestOpenCount + InventoryOpenCount;
		float Duration_ItemAndInventory = LogData.StageActionDurations.FindRef(Stage + TEXT("_Inventory"));

		// 5. 대화 상호작용 (Dialogue)
		int32 Count_Dialogue = LogData.StageDialogueLineReadCounts.FindRef(Stage);
		for (const auto& NpcPair : LogData.StageNPCDialogueCounts)
		{
			if (NpcPair.Key.StartsWith(Stage))
			{
				Count_Dialogue += NpcPair.Value;
			}
		}
		float Duration_Dialogue = LogData.StageActionDurations.FindRef(Stage + TEXT("_Dialogue"));

		// 6. 농경 활동 (Farming: 작물 심기 + 수확 + 잡초 제거)
		int32 Count_Farming = LogData.StageCropPlantCounts.FindRef(Stage) +
			LogData.StageCropHarvestActionCounts.FindRef(Stage) +
			LogData.StageInteractionActionCounts.FindRef(Stage + TEXT("_Weed"));
		float Duration_Farming = LogData.StageActionDurations.FindRef(Stage + TEXT("_Farming")) +
			LogData.StageActionDurations.FindRef(Stage + TEXT("_Weeding"));

		// 7. 제작 및 봉헌 (Crafting & Tribute: 포션 제조 + 봉헌 + 차원조각)
		int32 Count_CraftingTribute = LogData.StageTributeAltarUsageCounts.FindRef(Stage) +
			LogData.StageInteractionActionCounts.FindRef(Stage + TEXT("_PotionCrafting")) +
			LogData.StageInteractionActionCounts.FindRef(Stage + TEXT("_ShardsAltar"));
		float Duration_CraftingTribute = LogData.StageActionDurations.FindRef(Stage + TEXT("_PotionCrafting")) +
			LogData.StageActionDurations.FindRef(Stage + TEXT("_TributeAltar")) +
			LogData.StageActionDurations.FindRef(Stage + TEXT("_NPC_Service"));

		// 8. 기타 및 관찰/대기 (Etc & Idle: 정지 관찰 + 걷기 + 오브젝트 조작 + 수면)
		int32 Count_EtcIdle = LogData.StageSleepCounts.FindRef(Stage) + Count_Object;
		float Duration_EtcIdle = LogData.StageActionDurations.FindRef(Stage + TEXT("_Idle")) +
			LogData.StageActionDurations.FindRef(Stage + TEXT("_Walk")) +
			Duration_Object +
			LogData.StageActionDurations.FindRef(Stage + TEXT("_Sleep"));

		// CSV 행 추가 (정확히 8대 범례 1:1 매칭 데이터)
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_1_Sprint,%s,%d\n"), *SessionID, *Stage, Count_Sprint);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_1_Sprint,%s,%.2f\n"), *SessionID, *Stage, Duration_Sprint);

		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_2_Airborne,%s,%d\n"), *SessionID, *Stage, Count_Airborne);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_2_Airborne,%s,%.2f\n"), *SessionID, *Stage, Duration_Airborne);

		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_3_Combat,%s,%d\n"), *SessionID, *Stage, Count_Combat);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_3_Combat,%s,%.2f\n"), *SessionID, *Stage, Duration_Combat);

		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_4_ItemAndInventory,%s,%d\n"), *SessionID, *Stage, Count_ItemAndInventory);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_4_ItemAndInventory,%s,%.2f\n"), *SessionID, *Stage, Duration_ItemAndInventory);

		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_5_Dialogue,%s,%d\n"), *SessionID, *Stage, Count_Dialogue);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_5_Dialogue,%s,%.2f\n"), *SessionID, *Stage, Duration_Dialogue);

		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_6_Farming,%s,%d\n"), *SessionID, *Stage, Count_Farming);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_6_Farming,%s,%.2f\n"), *SessionID, *Stage, Duration_Farming);

		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_7_CraftingTribute,%s,%d\n"), *SessionID, *Stage, Count_CraftingTribute);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_7_CraftingTribute,%s,%.2f\n"), *SessionID, *Stage, Duration_CraftingTribute);

		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Count_8_EtcIdle,%s,%d\n"), *SessionID, *Stage, Count_EtcIdle);
		CSV += FString::Printf(TEXT("%s,MatchedActionMetric,Duration_8_EtcIdle,%s,%.2f\n"), *SessionID, *Stage, Duration_EtcIdle);
	}

	// 10. 🏆 8대 도전과제(Milestone) 목표 달성률(%) & 도전 횟수 자동 산출 (교수님 보고 및 콤보 차트용)

	// ─── [1. 튜토리얼 완주] ───
	float Progress_1_Tutorial = 0.0f;
	int32 TutorialRespawns = 0;
	for (const auto& Pair : LogData.StageFallRespawnCounts)
	{
		if (Pair.Key.Contains(TEXT("Tutorial"))) { TutorialRespawns += Pair.Value; }
	}
	for (const auto& Pair : LogData.StageTotalDeathCounts)
	{
		if (Pair.Key.Contains(TEXT("Tutorial"))) { TutorialRespawns += Pair.Value; }
	}
	int32 Attempt_1_Tutorial = 1 + TutorialRespawns;

	bool bTutorialCleared = false;
	for (const auto& Pair : LogData.MapClearTimes)
	{
		if (Pair.Key.Contains(TEXT("Tutorial"))) { bTutorialCleared = true; break; }
	}
	if (!bTutorialCleared)
	{
		for (const auto& Pair : LogData.StagePlayTimes)
		{
			if (Pair.Key.Contains(TEXT("Farm")) || Pair.Key.Contains(TEXT("Forest")) || Pair.Key.Contains(TEXT("Dungeon")))
			{
				bTutorialCleared = true;
				break;
			}
		}
	}

	if (bTutorialCleared)
	{
		Progress_1_Tutorial = 100.0f;
	}
	else
	{
		bool bEnteredSkyIsland = false;
		for (const auto& Pair : LogData.StagePlayTimes)
		{
			if (Pair.Key.Contains(TEXT("Tutorial")) && Pair.Key.Contains(TEXT("Sky")))
			{
				bEnteredSkyIsland = true;
				break;
			}
		}
		Progress_1_Tutorial = bEnteredSkyIsland ? 50.0f : (LogData.StagePlayTimes.Num() > 0 ? 25.0f : 0.0f);
	}

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,1_Tutorial_Escape,%.1f\n"), *SessionID, Progress_1_Tutorial);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,1_Tutorial_Escape,%d\n"), *SessionID, Attempt_1_Tutorial);

	// ─── [2. 거점: 모든 작물 키워보기] ───
	const int32 TotalTargetCropTypes = FMath::Max(1, LogData.TotalTargetCropTypes);
	int32 HarvestedCropTypes = LogData.CropHarvestCounts.Num();
	float Progress_2_AllCrops = FMath::Clamp(((float)HarvestedCropTypes / (float)TotalTargetCropTypes) * 100.0f, 0.0f, 100.0f);

	int32 TotalPlantedCrops = LogData.TotalCropPlantCount;
	int32 WeedDelayDays = (int32)LogData.TotalCropGrowthDelayDueToWeeds;
	int32 WeedingActions = 0;
	for (const auto& Pair : LogData.StageInteractionActionCounts)
	{
		if (Pair.Key.Contains(TEXT("Weed"))) { WeedingActions += Pair.Value; }
	}
	int32 Attempt_2_AllCrops = TotalPlantedCrops + WeedDelayDays + WeedingActions;
	if (Attempt_2_AllCrops == 0 && HarvestedCropTypes > 0)
	{
		Attempt_2_AllCrops = HarvestedCropTypes;
	}

	float WeedAffectedRate = (TotalPlantedCrops > 0) ? FMath::Clamp(((float)WeedDelayDays / (float)TotalPlantedCrops) * 100.0f, 0.0f, 100.0f) : 0.0f;
	float CropGrowthDelayRate = ((TotalPlantedCrops * 3 + WeedDelayDays) > 0) ? (((float)WeedDelayDays / (float)(TotalPlantedCrops * 3 + WeedDelayDays)) * 100.0f) : 0.0f;

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,2_Farm_AllCrops,%.1f\n"), *SessionID, Progress_2_AllCrops);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,WeedAffectedCropRate,2_Farm_AllCrops,%.1f\n"), *SessionID, WeedAffectedRate);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,CropDelayRate,2_Farm_AllCrops,%.1f\n"), *SessionID, CropGrowthDelayRate);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,2_Farm_AllCrops,%d\n"), *SessionID, Attempt_2_AllCrops);

	// ─── [3. 거점: 모든 포션 제조하기] ───
	const int32 TotalTargetPotionRecipes = FMath::Max(1, LogData.TotalTargetPotionRecipes);
	int32 CraftedPotionTypes = FMath::Max(LogData.CraftedPotionTypeCounts.Num(), (LogData.SuccessfulPotionCraftingCount > 0 ? 1 : 0));
	float Progress_3_AllPotions = FMath::Clamp(((float)CraftedPotionTypes / (float)TotalTargetPotionRecipes) * 100.0f, 0.0f, 100.0f);
	int32 Attempt_3_AllPotions = LogData.SuccessfulPotionCraftingCount + LogData.FailedPotionCraftingCount;
	if (Attempt_3_AllPotions == 0 && CraftedPotionTypes > 0)
	{
		Attempt_3_AllPotions = CraftedPotionTypes;
	}

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,3_Farm_AllPotions,%.1f\n"), *SessionID, Progress_3_AllPotions);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,3_Farm_AllPotions,%d\n"), *SessionID, Attempt_3_AllPotions);

	// ─── [4. 거점: 봉헌 끝까지 도달하기] ───
	const int32 MaxTargetTributeSteps = FMath::Max(1, LogData.MaxTargetTributeSteps);
	int32 TotalTributeActions = 0;
	for (const auto& Pair : LogData.StageTributeAltarUsageCounts)
	{
		TotalTributeActions += Pair.Value;
	}
	for (const auto& Pair : LogData.StageInteractionActionCounts)
	{
		if (Pair.Key.Contains(TEXT("ShardsAltar")) || Pair.Key.Contains(TEXT("Tribute")))
		{
			TotalTributeActions += Pair.Value;
		}
	}
	// 실제 달성한 TributeLevel 기준 (없을 경우 상호작용 횟수로 폴백)
	int32 AchievedTributeLevel = (LogData.CurrentTributeLevel > 0) ? LogData.CurrentTributeLevel : (LogData.TributeAltarUsageCount > 0 ? LogData.TributeAltarUsageCount : 0);
	float Progress_4_MaxTribute = FMath::Clamp(((float)AchievedTributeLevel / (float)MaxTargetTributeSteps) * 100.0f, 0.0f, 100.0f);
	int32 Attempt_4_MaxTribute = TotalTributeActions;

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,4_Farm_MaxTribute,%.1f\n"), *SessionID, Progress_4_MaxTribute);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,4_Farm_MaxTribute,%d\n"), *SessionID, Attempt_4_MaxTribute);

	// ─── [5. 숲: 끝까지 도달 (보스룸)] ───
	const int32 TotalForestCheckpoints = 11;
	int32 ForestReachedCP = 0;
	for (const auto& Pair : LogData.MaxReachedCheckpointIndex)
	{
		if (Pair.Key.Contains(TEXT("Forest"))) { ForestReachedCP = FMath::Max(ForestReachedCP, Pair.Value + 1); }
	}
	bool bForestBossReached = false;
	for (const auto& Pair : LogData.BossEnterCounts)
	{
		if (Pair.Key.Contains(TEXT("SkeletonMage")) || Pair.Key.Contains(TEXT("Forest"))) { bForestBossReached = true; break; }
	}
	for (const auto& Pair : LogData.BossBattleTimes)
	{
		if (Pair.Key.Contains(TEXT("SkeletonMage")) || Pair.Key.Contains(TEXT("Forest"))) { bForestBossReached = true; break; }
	}
	float Progress_5_ForestReach = bForestBossReached ? 100.0f : FMath::Clamp(((float)ForestReachedCP / (float)TotalForestCheckpoints) * 100.0f, 0.0f, 100.0f);

	int32 ForestRespawns = 0;
	for (const auto& Pair : LogData.StageFallRespawnCounts)
	{
		if (Pair.Key.Contains(TEXT("Forest"))) { ForestRespawns += Pair.Value; }
	}
	for (const auto& Pair : LogData.StageTotalDeathCounts)
	{
		if (Pair.Key.Contains(TEXT("Forest"))) { ForestRespawns += Pair.Value; }
	}
	int32 Attempt_5_ForestReach = 1 + ForestRespawns;

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,5_Forest_ReachBoss,%.1f\n"), *SessionID, Progress_5_ForestReach);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,5_Forest_ReachBoss,%d\n"), *SessionID, Attempt_5_ForestReach);

	// ─── [6. 숲: 보스 격파] ───
	int32 ForestBossEnter = 0;
	for (const auto& Pair : LogData.BossEnterCounts)
	{
		if (Pair.Key.Contains(TEXT("SkeletonMage")) || Pair.Key.Contains(TEXT("Forest"))) { ForestBossEnter += Pair.Value; }
	}
	int32 ForestBossClear = 0;
	for (const auto& Pair : LogData.BossClearCounts)
	{
		if (Pair.Key.Contains(TEXT("SkeletonMage")) || Pair.Key.Contains(TEXT("Forest"))) { ForestBossClear += Pair.Value; }
	}
	for (const auto& ClearedStage : ClearedBossStages)
	{
		if (ClearedStage.Contains(TEXT("Forest"))) { ForestBossClear = FMath::Max(ForestBossClear, 1); }
	}

	float ForestBossMinHP = 1.0f;
	for (const auto& Pair : LogData.BossMinHealthRatios)
	{
		if (Pair.Key.Contains(TEXT("SkeletonMage")) || Pair.Key.Contains(TEXT("Forest")))
		{
			ForestBossMinHP = FMath::Min(ForestBossMinHP, Pair.Value);
		}
	}
	if (ForestBossClear > 0)
	{
		ForestBossMinHP = 0.0f;
	}
	float Progress_6_ForestBoss = (ForestBossClear > 0) ? 100.0f : ((bForestBossReached || ForestBossEnter > 0) ? FMath::Clamp((1.0f - ForestBossMinHP) * 100.0f, 0.0f, 100.0f) : 0.0f);
	int32 Attempt_6_ForestBoss = FMath::Max(ForestBossEnter, ForestBossClear);
	if (Attempt_6_ForestBoss == 0 && bForestBossReached) { Attempt_6_ForestBoss = 1; }
	float ClearRate_6_ForestBoss = (Attempt_6_ForestBoss > 0) ? (((float)ForestBossClear / (float)Attempt_6_ForestBoss) * 100.0f) : 0.0f;

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,6_Forest_DefeatBoss,%.1f\n"), *SessionID, Progress_6_ForestBoss);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,ClearRate,6_Forest_DefeatBoss,%.1f\n"), *SessionID, ClearRate_6_ForestBoss);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,6_Forest_DefeatBoss,%d\n"), *SessionID, Attempt_6_ForestBoss);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,ClearCount,6_Forest_DefeatBoss,%d\n"), *SessionID, ForestBossClear);

	// ─── [7. 던전: 끝까지 도달 (보스룸)] ───
	const int32 TotalDungeonGimmicks = 4;
	int32 DungeonClearedGimmicks = 0;
	for (const auto& Pair : LogData.ClearedGimmickRooms)
	{
		if (Pair.Key.Contains(TEXT("Dungeon")) && Pair.Value) { DungeonClearedGimmicks++; }
	}
	bool bDungeonBossReached = false;
	for (const auto& Pair : LogData.BossEnterCounts)
	{
		if (Pair.Key.Contains(TEXT("Assassin")) || Pair.Key.Contains(TEXT("Dungeon"))) { bDungeonBossReached = true; break; }
	}
	for (const auto& Pair : LogData.BossBattleTimes)
	{
		if (Pair.Key.Contains(TEXT("Assassin")) || Pair.Key.Contains(TEXT("Dungeon"))) { bDungeonBossReached = true; break; }
	}
	float Progress_7_DungeonReach = bDungeonBossReached ? 100.0f : FMath::Clamp(((float)DungeonClearedGimmicks / (float)TotalDungeonGimmicks) * 100.0f, 0.0f, 100.0f);

	int32 DungeonRespawns = 0;
	for (const auto& Pair : LogData.StageFallRespawnCounts)
	{
		if (Pair.Key.Contains(TEXT("Dungeon"))) { DungeonRespawns += Pair.Value; }
	}
	for (const auto& Pair : LogData.StageTotalDeathCounts)
	{
		if (Pair.Key.Contains(TEXT("Dungeon"))) { DungeonRespawns += Pair.Value; }
	}
	int32 Attempt_7_DungeonReach = 1 + DungeonRespawns;

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,7_Dungeon_ReachBoss,%.1f\n"), *SessionID, Progress_7_DungeonReach);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,7_Dungeon_ReachBoss,%d\n"), *SessionID, Attempt_7_DungeonReach);

	// ─── [8. 던전: 보스 격파] ───
	int32 DungeonBossEnter = 0;
	for (const auto& Pair : LogData.BossEnterCounts)
	{
		if (Pair.Key.Contains(TEXT("Assassin")) || Pair.Key.Contains(TEXT("Dungeon"))) { DungeonBossEnter += Pair.Value; }
	}
	int32 DungeonBossClear = 0;
	for (const auto& Pair : LogData.BossClearCounts)
	{
		if (Pair.Key.Contains(TEXT("Assassin")) || Pair.Key.Contains(TEXT("Dungeon"))) { DungeonBossClear += Pair.Value; }
	}
	for (const auto& ClearedStage : ClearedBossStages)
	{
		if (ClearedStage.Contains(TEXT("Dungeon"))) { DungeonBossClear = FMath::Max(DungeonBossClear, 1); }
	}

	float DungeonBossMinHP = 1.0f;
	for (const auto& Pair : LogData.BossMinHealthRatios)
	{
		if (Pair.Key.Contains(TEXT("Assassin")) || Pair.Key.Contains(TEXT("Dungeon")))
		{
			DungeonBossMinHP = FMath::Min(DungeonBossMinHP, Pair.Value);
		}
	}
	if (DungeonBossClear > 0)
	{
		DungeonBossMinHP = 0.0f;
	}
	float Progress_8_DungeonBoss = (DungeonBossClear > 0) ? 100.0f : ((bDungeonBossReached || DungeonBossEnter > 0) ? FMath::Clamp((1.0f - DungeonBossMinHP) * 100.0f, 0.0f, 100.0f) : 0.0f);
	int32 Attempt_8_DungeonBoss = FMath::Max(DungeonBossEnter, DungeonBossClear);
	if (Attempt_8_DungeonBoss == 0 && bDungeonBossReached) { Attempt_8_DungeonBoss = 1; }
	float ClearRate_8_DungeonBoss = (Attempt_8_DungeonBoss > 0) ? (((float)DungeonBossClear / (float)Attempt_8_DungeonBoss) * 100.0f) : 0.0f;

	CSV += FString::Printf(TEXT("%s,GoalAchievement,ProgressPercent,8_Dungeon_DefeatBoss,%.1f\n"), *SessionID, Progress_8_DungeonBoss);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,ClearRate,8_Dungeon_DefeatBoss,%.1f\n"), *SessionID, ClearRate_8_DungeonBoss);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,AttemptCount,8_Dungeon_DefeatBoss,%d\n"), *SessionID, Attempt_8_DungeonBoss);
	CSV += FString::Printf(TEXT("%s,GoalAchievement,ClearCount,8_Dungeon_DefeatBoss,%d\n"), *SessionID, DungeonBossClear);

	return CSV;
}

bool UGameplayLogSubsystem::ExportLogsToCSVFile(const FString& FilePath)
{
	FString SavePath = FilePath;
	if (SavePath.IsEmpty())
	{
		SavePath = FPaths::ProjectSavedDir() / TEXT("Logs") / TEXT("ShardsOfVeyara_GameplayLog.csv");
	}

	FString CurrentSessionID = LogData.PlayerSessionID.IsEmpty() ? TEXT("Unknown") : LogData.PlayerSessionID;
	FString CSVContent = GenerateCSVString();

	// 🛡️ 세션별 자동 갱신(Upsert) 로직:
	// 파일이 이미 존재하면 동일한 PlayerSessionID의 이전 중간 기록을 필터링하여 최신 스냅샷으로 덮어쓰고,
	// 다른 플레이어 세션 데이터는 완벽히 보존합니다.
	if (IFileManager::Get().FileExists(*SavePath))
	{
		FString ExistingContent;
		if (FFileHelper::LoadFileToString(ExistingContent, *SavePath))
		{
			TArray<FString> ExistingLines;
			ExistingContent.ParseIntoArrayLines(ExistingLines);

			FString HeaderLine = TEXT("PlayerSessionID,Category,MetricName,Key,Value");
			TArray<FString> PreservedLines;
			PreservedLines.Add(HeaderLine);

			for (const FString& Line : ExistingLines)
			{
				FString TrimmedLine = Line.TrimStartAndEnd();
				if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("PlayerSessionID")))
				{
					continue;
				}

				// 첫 번째 쉼표 전까지가 PlayerSessionID
				FString LineSessionID;
				int32 CommaIndex;
				if (TrimmedLine.FindChar(TEXT(','), CommaIndex))
				{
					LineSessionID = TrimmedLine.Left(CommaIndex);
				}
				else
				{
					LineSessionID = TrimmedLine;
				}

				// 다른 세션의 데이터만 보존 (현재 세션의 이전 중간 데이터는 제외)
				if (!LineSessionID.Equals(CurrentSessionID, ESearchCase::IgnoreCase))
				{
					PreservedLines.Add(TrimmedLine);
				}
			}

			// 이번 세션의 최신 완성 스냅샷 데이터 행들 추가
			TArray<FString> NewLines;
			CSVContent.ParseIntoArrayLines(NewLines);
			for (const FString& NewLine : NewLines)
			{
				FString TrimmedNewLine = NewLine.TrimStartAndEnd();
				if (!TrimmedNewLine.IsEmpty() && !TrimmedNewLine.StartsWith(TEXT("PlayerSessionID")))
				{
					PreservedLines.Add(TrimmedNewLine);
				}
			}

			FString FinalCSV = FString::Join(PreservedLines, TEXT("\n")) + TEXT("\n");
			return FFileHelper::SaveStringToFile(FinalCSV, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8);
		}
	}

	// 파일이 없으면 UTF-8 with BOM(ForceUTF8)으로 신규 생성 (엑셀 한글 깨짐 방지)
	return FFileHelper::SaveStringToFile(CSVContent, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8);
}
