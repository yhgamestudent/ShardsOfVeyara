#include "GameplayLogSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UGameplayLogSubsystem::AddCropGrowthDelayDueToWeeds(float DelaySeconds)
{
	LogData.TotalCropGrowthDelayDueToWeeds += DelaySeconds;
}

void UGameplayLogSubsystem::RecordPlayerPositionAtDateChange(int32 Day, FVector Position)
{
	LogData.PlayerPositionsAtDateChange.Add(Day, Position);
}

void UGameplayLogSubsystem::IncrementFailedPotionCrafting()
{
	LogData.FailedPotionCraftingCount++;
}

void UGameplayLogSubsystem::RecordUsedCombo(const FString& ComboName)
{
	int32& Count = LogData.UsedComboCounts.FindOrAdd(ComboName);
	Count++;
}

void UGameplayLogSubsystem::RecordCompletedCombo(const FString& ComboName)
{
	int32& Count = LogData.CompletedComboCounts.FindOrAdd(ComboName);
	Count++;
}

void UGameplayLogSubsystem::RecordElixirUsageOnCrop(const FString& CropName)
{
	int32& Count = LogData.ElixirUsagePerCrop.FindOrAdd(CropName);
	Count++;
}

void UGameplayLogSubsystem::AddAcquiredCoins(int32 Amount)
{
	LogData.TotalAcquiredCoins += Amount;
}

void UGameplayLogSubsystem::AddConsumedCoins(int32 Amount)
{
	LogData.TotalConsumedCoins += Amount;
}

void UGameplayLogSubsystem::RecordMapClearTime(const FString& MapName, float TimeSeconds)
{
	LogData.MapClearTimes.Add(MapName, TimeSeconds);
}

void UGameplayLogSubsystem::AddDamageMitigatedByGuard(float MitigatedDamage)
{
	LogData.TotalDamageMitigatedByGuard += MitigatedDamage;
}

void UGameplayLogSubsystem::AddPlayTime(float TimeSeconds)
{
	LogData.TotalPlayTime += TimeSeconds;
}

void UGameplayLogSubsystem::IncrementPauseCount()
{
	LogData.PauseCount++;
}

void UGameplayLogSubsystem::IncrementItemsAcquiredCount()
{
	LogData.TotalItemsAcquired++;
}

void UGameplayLogSubsystem::IncrementItemsDiscardedCount()
{
	LogData.TotalItemsDiscarded++;
}

void UGameplayLogSubsystem::IncrementInventoryFullOccurrence()
{
	LogData.InventoryFullOccurrenceCount++;
}

void UGameplayLogSubsystem::RecordNPCDialogue(const FString& NPCName)
{
	int32& Count = LogData.NPCDialogueCounts.FindOrAdd(NPCName);
	Count++;
}

void UGameplayLogSubsystem::RecordMonsterKill(const FString& MonsterName)
{
	int32& Count = LogData.MonsterKillCounts.FindOrAdd(MonsterName);
	Count++;
}

void UGameplayLogSubsystem::RecordCropHarvest(const FString& CropName, int32 Amount)
{
	int32& Count = LogData.CropHarvestCounts.FindOrAdd(CropName);
	Count += Amount;
}

void UGameplayLogSubsystem::IncrementGuardUsageCount()
{
	LogData.GuardUsageCount++;
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

void UGameplayLogSubsystem::IncrementStageInteraction(const FString& StageName)
{
	int32& Count = LogData.StageInteractionCounts.FindOrAdd(StageName);
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

void UGameplayLogSubsystem::IncrementDialogueLineSkip()
{
	LogData.DialogueLineSkipCount++;
}

void UGameplayLogSubsystem::IncrementOptionChange()
{
	LogData.OptionChangeCount++;
}

void UGameplayLogSubsystem::IncrementDialogueLogRecheck()
{
	LogData.DialogueLogRecheckCount++;
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

void UGameplayLogSubsystem::RecordGimmickClear(const FString& GimmickName, const FString& StageName)
{
	FString TargetStage = StageName;
	if (TargetStage.IsEmpty() && GetWorld())
	{
		TargetStage = GetWorld()->GetMapName();
		TargetStage.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
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
	int32& Count = LogData.PotionUsagePerMap.FindOrAdd(StageName);
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

void UGameplayLogSubsystem::IncrementTributeAltarUsage()
{
	LogData.TributeAltarUsageCount++;
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
	FString CSV = TEXT("Category,MetricName,Key,Value\n");

	// 1. 단일 수치 지표 (General, Tutorial, UI, Reward)
	CSV += FString::Printf(TEXT("General,TotalPlayTime,,%.2f\n"), LogData.TotalPlayTime);
	CSV += FString::Printf(TEXT("General,TotalAcquiredCoins,,%d\n"), LogData.TotalAcquiredCoins);
	CSV += FString::Printf(TEXT("General,TotalConsumedCoins,,%d\n"), LogData.TotalConsumedCoins);
	CSV += FString::Printf(TEXT("General,FailedPotionCraftingCount,,%d\n"), LogData.FailedPotionCraftingCount);
	CSV += FString::Printf(TEXT("General,PauseCount,,%d\n"), LogData.PauseCount);
	CSV += FString::Printf(TEXT("General,TotalItemsAcquired,,%d\n"), LogData.TotalItemsAcquired);
	CSV += FString::Printf(TEXT("General,TotalItemsDiscarded,,%d\n"), LogData.TotalItemsDiscarded);
	CSV += FString::Printf(TEXT("General,InventoryFullOccurrenceCount,,%d\n"), LogData.InventoryFullOccurrenceCount);
	CSV += FString::Printf(TEXT("General,GuardUsageCount,,%d\n"), LogData.GuardUsageCount);
	CSV += FString::Printf(TEXT("General,TotalDamageMitigatedByGuard,,%.2f\n"), LogData.TotalDamageMitigatedByGuard);
	CSV += FString::Printf(TEXT("General,TotalCropGrowthDelayDueToWeeds,,%.2f\n"), LogData.TotalCropGrowthDelayDueToWeeds);

	CSV += FString::Printf(TEXT("Tutorial,FullSkipCount,,%d\n"), LogData.TutorialFullSkipCount);
	CSV += FString::Printf(TEXT("Tutorial,DialogueLineSkipCount,,%d\n"), LogData.DialogueLineSkipCount);

	CSV += FString::Printf(TEXT("UI,OptionChangeCount,,%d\n"), LogData.OptionChangeCount);
	CSV += FString::Printf(TEXT("UI,DialogueLogRecheckCount,,%d\n"), LogData.DialogueLogRecheckCount);

	CSV += FString::Printf(TEXT("Reward,TributeAltarUsageCount,,%d\n"), LogData.TributeAltarUsageCount);
	CSV += FString::Printf(TEXT("Reward,LatestHealthPercent,,%.2f\n"), LogData.LatestPlayerHealthPercent);

	// 2. 스테이지별 이동 및 행동 지표
	for (const auto& Pair : LogData.StagePlayTimes)
	{
		CSV += FString::Printf(TEXT("StagePlayTime,PlayTime,%s,%.2f\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageClearPortalTimes)
	{
		CSV += FString::Printf(TEXT("StageClearPortalTime,PortalTime,%s,%.2f\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageMovementDistances)
	{
		CSV += FString::Printf(TEXT("StageMovementDistance,DistanceMeters,%s,%.2f\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageInteractionCounts)
	{
		CSV += FString::Printf(TEXT("StageInteractionCount,Interactions,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageFallRespawnCounts)
	{
		CSV += FString::Printf(TEXT("StageFallRespawnCount,FallRespawns,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageJumpCounts)
	{
		CSV += FString::Printf(TEXT("StageJumpCount,Jumps,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDashCounts)
	{
		CSV += FString::Printf(TEXT("StageDashCount,Dashes,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageGimmickClearCounts)
	{
		CSV += FString::Printf(TEXT("StageGimmickClearCount,StageClears,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.GimmickClearCounts)
	{
		CSV += FString::Printf(TEXT("GimmickClearCount,GimmickClears,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossPatternDodgeCounts)
	{
		CSV += FString::Printf(TEXT("BossPatternDodgeCount,PatternDodges,%s,%d\n"), *Pair.Key, Pair.Value);
	}

	// 3. 전투 및 보스전 지표
	for (const auto& Pair : LogData.BossEnterCounts)
	{
		CSV += FString::Printf(TEXT("BossEnterCount,BossEnters,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossDeathCounts)
	{
		CSV += FString::Printf(TEXT("BossDeathCount,BossDeaths,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossClearCounts)
	{
		CSV += FString::Printf(TEXT("BossClearCount,BossClears,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossBattleTimes)
	{
		CSV += FString::Printf(TEXT("BossBattleTime,BattleSeconds,%s,%.2f\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.BossLootAcquiredCounts)
	{
		CSV += FString::Printf(TEXT("BossLootCount,LootAmount,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.PotionUsagePerMap)
	{
		CSV += FString::Printf(TEXT("PotionUsagePerMap,PotionCount,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.PotionUsageByType)
	{
		CSV += FString::Printf(TEXT("PotionUsageByType,PotionCount,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDamageDealt)
	{
		CSV += FString::Printf(TEXT("StageDamageDealt,DamageDealt,%s,%.2f\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageDamageTaken)
	{
		CSV += FString::Printf(TEXT("StageDamageTaken,DamageTaken,%s,%.2f\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.DeathReasonCounts)
	{
		CSV += FString::Printf(TEXT("DeathReason,Deaths,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.MonsterKillCounts)
	{
		CSV += FString::Printf(TEXT("MonsterKill,Kills,%s,%d\n"), *Pair.Key, Pair.Value);
	}

	// 4. 거점/파밍/상호작용/콤보 지표
	for (const auto& Pair : LogData.CropHarvestCounts)
	{
		CSV += FString::Printf(TEXT("CropHarvest,HarvestAmount,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.ElixirUsagePerCrop)
	{
		CSV += FString::Printf(TEXT("ElixirUsage,ElixirCount,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.UsedComboCounts)
	{
		CSV += FString::Printf(TEXT("UsedCombo,ComboUsed,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.CompletedComboCounts)
	{
		CSV += FString::Printf(TEXT("CompletedCombo,ComboCompleted,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.NPCDialogueCounts)
	{
		CSV += FString::Printf(TEXT("NPCDialogue,Dialogues,%s,%d\n"), *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.MapClearTimes)
	{
		CSV += FString::Printf(TEXT("MapClearTime,ClearSeconds,%s,%.2f\n"), *Pair.Key, Pair.Value);
	}

	// 5. 공간 히트맵 좌표 데이터
	for (const auto& Pair : LogData.PlayerPositionsAtDateChange)
	{
		CSV += FString::Printf(TEXT("PlayerPositionAtDateChange,Day_%d,%s,1\n"), Pair.Key, *Pair.Value.ToString());
	}
	for (int32 i = 0; i < LogData.DeathPositions.Num(); ++i)
	{
		CSV += FString::Printf(TEXT("DeathPosition,DeathIndex_%d,%s,1\n"), i + 1, *LogData.DeathPositions[i].ToString());
	}
	for (int32 i = 0; i < LogData.FallRespawnPositions.Num(); ++i)
	{
		CSV += FString::Printf(TEXT("FallRespawnPosition,FallIndex_%d,%s,1\n"), i + 1, *LogData.FallRespawnPositions[i].ToString());
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
	return FFileHelper::SaveStringToFile(CSVContent, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
