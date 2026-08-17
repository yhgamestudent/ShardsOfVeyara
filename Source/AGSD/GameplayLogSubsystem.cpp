#include "GameplayLogSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"

void UGameplayLogSubsystem::InitNewSession()
{
	// 로그 데이터를 완전히 쒈초화하고 새 UUID를 세션 ID로 할당합니다
	LogData = FGameplayLogData();
	LogData.PlayerSessionID = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
}

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

	// 5. 공간 히트맵 좌표 데이터
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
