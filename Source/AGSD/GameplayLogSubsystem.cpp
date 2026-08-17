#include "GameplayLogSubsystem.h"

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
