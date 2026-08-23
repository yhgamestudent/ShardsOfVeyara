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
	CSV += FString::Printf(TEXT("%s,Tutorial,DialogueLineSkipCount,,%d\n"), *SessionID, LogData.DialogueLineSkipCount);

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
	for (const auto& Pair : LogData.StageDialogueLineSkipCounts)
	{
		CSV += FString::Printf(TEXT("%s,Tutorial,StageDialogueLineSkip,%s,%d\n"), *SessionID, *Pair.Key, Pair.Value);
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
	// 6-1. 체크포인트 구간별 소요 시간 & 이동거리
	for (const auto& Pair : LogData.StageCheckpointSectionTimes)
	{
		CSV += FString::Printf(TEXT("%s,CheckpointSectionTime,SectionSeconds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}
	for (const auto& Pair : LogData.StageCheckpointSectionDistances)
	{
		CSV += FString::Printf(TEXT("%s,CheckpointSectionDistance,SectionMeters,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
	}

	// 6-2. 던전 기믹 방별 소요 시간 & 이동거리 및 복도/탐색 잔여 데이터
	float TotalDungeonGimmickTime = 0.0f;
	for (const auto& Pair : LogData.StageGimmickRoomTimes)
	{
		CSV += FString::Printf(TEXT("%s,GimmickRoomTime,RoomSeconds,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
		if (Pair.Key.StartsWith(TEXT("Dungeon_")))
		{
			TotalDungeonGimmickTime += Pair.Value;
		}
	}
	if (const float* DungeonTotalPlayTime = LogData.StagePlayTimes.Find(TEXT("Dungeon")))
	{
		float DungeonBossTime = 0.0f;
		for (const auto& Pair : LogData.BossBattleTimes)
		{
			if (Pair.Key.Contains(TEXT("Dungeon")) || Pair.Key.Contains(TEXT("Boss")))
			{
				DungeonBossTime += Pair.Value;
			}
		}
		float CorridorTime = FMath::Max(0.0f, *DungeonTotalPlayTime - (TotalDungeonGimmickTime + DungeonBossTime));
		CSV += FString::Printf(TEXT("%s,GimmickRoomTime,RoomSeconds,Dungeon_Corridor_Exploration,%.2f\n"), *SessionID, CorridorTime);
	}

	float TotalDungeonGimmickDistance = 0.0f;
	for (const auto& Pair : LogData.StageGimmickRoomDistances)
	{
		CSV += FString::Printf(TEXT("%s,GimmickRoomDistance,RoomMeters,%s,%.2f\n"), *SessionID, *Pair.Key, Pair.Value);
		if (Pair.Key.StartsWith(TEXT("Dungeon_")))
		{
			TotalDungeonGimmickDistance += Pair.Value;
		}
	}
	if (const float* DungeonTotalDistance = LogData.StageMovementDistances.Find(TEXT("Dungeon")))
	{
		float CorridorDist = FMath::Max(0.0f, *DungeonTotalDistance - TotalDungeonGimmickDistance);
		CSV += FString::Printf(TEXT("%s,GimmickRoomDistance,RoomMeters,Dungeon_Corridor_Exploration,%.2f\n"), *SessionID, CorridorDist);
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
