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
	LogData.MapClearTimes.Add(MapName, TimeSeconds);
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
	int32& Count = LogData.MonsterKillCounts.FindOrAdd(MonsterName);
	Count++;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *MonsterName);
	int32& StageCount = LogData.StageMonsterKillCounts.FindOrAdd(Key);
	StageCount++;
}

void UGameplayLogSubsystem::RecordCropHarvest(const FString& CropName, int32 Amount, const FString& StageName)
{
	int32& Count = LogData.CropHarvestCounts.FindOrAdd(CropName);
	Count += Amount;

	FString TargetStage = GetTargetStageName(StageName);
	FString Key = FString::Printf(TEXT("%s_%s"), *TargetStage, *CropName);
	int32& StageCount = LogData.StageCropHarvestCounts.FindOrAdd(Key);
	StageCount += Amount;
}

void UGameplayLogSubsystem::IncrementGuardUsageCount(const FString& StageName)
{
	LogData.GuardUsageCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageGuardCounts.FindOrAdd(TargetStage);
	StageCount++;
}

// ==========================================
// 📌 9대 가설 & 18종 차트 로깅 헬퍼 구현
// ==========================================

void UGameplayLogSubsystem::AddStagePlayTime(const FString& StageName, float DeltaSeconds)
{
	float& Time = LogData.StagePlayTimes.FindOrAdd(StageName);
	Time += DeltaSeconds;
}

void UGameplayLogSubsystem::RecordStageClearPortalTime(const FString& StageName, float TimeSeconds)
{
	LogData.StageClearPortalTimes.Add(StageName, TimeSeconds);
}

void UGameplayLogSubsystem::AddStageMovementDistance(const FString& StageName, float DistanceMeters)
{
	float& Dist = LogData.StageMovementDistances.FindOrAdd(StageName);
	Dist += DistanceMeters;
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
	int32& Count = LogData.StageFallRespawnCounts.FindOrAdd(StageName);
	Count++;
	LogData.FallRespawnPositions.Add(FallLocation);
}

void UGameplayLogSubsystem::IncrementTutorialFullSkip()
{
	LogData.TutorialFullSkipCount++;
}

void UGameplayLogSubsystem::IncrementDialogueLineSkip(const FString& StageName)
{
	LogData.DialogueLineSkipCount++;

	FString TargetStage = GetTargetStageName(StageName);
	int32& StageCount = LogData.StageDialogueLineSkipCounts.FindOrAdd(TargetStage);
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
	int32& Count = LogData.DeathReasonCounts.FindOrAdd(DeathReason);
	Count++;
	LogData.DeathPositions.Add(DeathLocation);
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
	LogData.GimmickClearTimes.Add(GimmickName, TimeSeconds);
	IncrementStageGimmickClear(GimmickName);
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
}

void UGameplayLogSubsystem::RecordBossDeath(const FString& BossName)
{
	int32& Count = LogData.BossDeathCounts.FindOrAdd(BossName);
	Count++;
}

void UGameplayLogSubsystem::RecordBossClear(const FString& BossName, float BattleTimeSeconds, int32 LootAcquiredCount)
{
	int32& ClearCount = LogData.BossClearCounts.FindOrAdd(BossName);
	ClearCount++;
	LogData.BossBattleTimes.Add(BossName, BattleTimeSeconds);
	LogData.BossLootAcquiredCounts.Add(BossName, LootAcquiredCount);
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
	
	CSV += FString::Printf(TEXT("%s,Tutorial,FullSkipCount,,%d\n"), *SessionID, LogData.TutorialFullSkipCount);
	CSV += FString::Printf(TEXT("%s,Tutorial,DialogueLineSkipCount,,%d\n"), *SessionID, LogData.DialogueLineSkipCount);

	CSV += FString::Printf(TEXT("%s,UI,OptionChangeCount,,%d\n"), *SessionID, LogData.OptionChangeCount);
	CSV += FString::Printf(TEXT("%s,UI,DialogueLogRecheckCount,,%d\n"), *SessionID, LogData.DialogueLogRecheckCount);

	CSV += FString::Printf(TEXT("%s,Reward,TributeAltarUsageCount,,%d\n"), *SessionID, LogData.TributeAltarUsageCount);
	CSV += FString::Printf(TEXT("%s,Reward,LatestHealthPercent,,%.2f\n"), *SessionID, LogData.LatestPlayerHealthPercent);
	
	// 2. 스테이지별 이동 및 행동 지표
	for (const auto& Pair : LogData.StagePlayTimes)
	{
		CSV += FString::Printf(TEXT("%s,StagePlayTime,PlayTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageClearPortalTimes)
	{
		CSV += FString::Printf(TEXT("%s,StageClearPortalTime,PortalTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageMovementDistances)
	{
		CSV += FString::Printf(TEXT("%s,StageMovementDistance,DistanceMeters,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
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
		CSV += FString::Printf(TEXT("%s,GimmickStartTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.GimmickClearTimes)
	{
		CSV += FString::Printf(TEXT("%s,GimmickClearTime,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
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
	for (const auto& Pair : LogData.StageCropHarvestCounts)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageCropHarvest,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageElixirUsageCounts)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageElixirUsage,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageCropGrowthDelayDueToWeeds)
	{
		CSV += FString::Printf(TEXT("%s,Farming,StageCropGrowthDelayDueToWeeds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
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
	for (const auto& Pair : LogData.StageDialogueLineSkipCounts)
	{
		CSV += FString::Printf(TEXT("%s,Tutorial,StageDialogueLineSkip,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDialogueLogRecheckCounts)
	{
		CSV += FString::Printf(TEXT("%s,UI,StageDialogueLogRecheck,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 6. 공간 히트맵 좌표 데이터
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

	return CSV;
}

bool UGameplayLogSubsystem::ExportLogsToCSVFile(const FString& FilePath)
{
	FString SavePath = FilePath;
	if (SavePath.IsEmpty())
	{
		SavePath = FPaths::ProjectSavedDir() / TEXT("Logs") / TEXT("ShardsOfVeyara_GameplayLog.csv");
	}

	FString CSVContent = GenerateCSVString();

	// 파일이 이미 존재하면 헤더를 제외하고 데이터 행만 append
	if (IFileManager::Get().FileExists(*SavePath))
	{
		// 첫 담락(헤더 행)을 제거하고 나머지만 우리
		FString DataOnly = CSVContent;
		int32 NewlineIndex;
		if (DataOnly.FindChar(TEXT('\n'), NewlineIndex))
		{
			DataOnly = DataOnly.RightChop(NewlineIndex + 1);
		}
		return FFileHelper::SaveStringToFile(
			DataOnly, *SavePath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(),
			EFileWrite::FILEWRITE_Append);
	}

	// 파일이 없으면 헤더 포함 신규 생성
	return FFileHelper::SaveStringToFile(CSVContent, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
