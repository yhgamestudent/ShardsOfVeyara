#pragma once
#include "PlotSaveData.h"
#include "FarmingSystem/AppleTreeSaveData.h"
#include "Alchemy/AlchemySaveData.h"
#include "Struct_InventorySlotData.h"
#include "Struct_WorldChestData.h"
#include "Struct_GameplayLogData.h"
#include "SaveData.generated.h"

USTRUCT(BlueprintType)
struct FSaveData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> NoRegenItem;

	UPROPERTY()
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY()
	FName TeleportationTag = FName("None");
	
	UPROPERTY()
	int CurrentDay;

	UPROPERTY()
	float CurrentTime;

	UPROPERTY()
	TMap<FString, FStruct_WorldChestData> ChestMap;

	UPROPERTY()
	int32 ShardsAmount;
	
	UPROPERTY()
	int Coin;
	
	UPROPERTY()
	float MaxPlayerHealth;
	
	UPROPERTY()
	float PlayerHealth;

	UPROPERTY()
	bool bHasPet;

	UPROPERTY()
	TArray<FStruct_InventorySlotData> TempInventory;

	UPROPERTY()
	TArray<FStruct_InventorySlotData> TempHotbar;
	
	UPROPERTY()
	TMap<FString, FPlotSaveData> GlobalPlotDataMap;

	UPROPERTY()
	TMap<FString, FAppleTreeSaveData> GlobalAppleTreeDataMap;

	UPROPERTY()
	TMap<FString, FAlchemySaveData> AlchemyTableDataMap;

	UPROPERTY()
	FDateTime LastSaveTime;
	
	UPROPERTY()
	TArray<FString> AlreadyDroppedItems;

	UPROPERTY()
	float MouseSensitivity;
	
	UPROPERTY()
	int32 TributeLevel;

	UPROPERTY()
	TMap<FString, int32> CurrentLevelTributeItems;

	UPROPERTY()
	float Damage;

	UPROPERTY()
	FGameplayLogData LogData;

	FSaveData()
	{
		CurrentDay = 1;
		CurrentTime = 360.f;
		ShardsAmount = 0;
		Coin = 0;
		Damage = 100.0f;
		MaxPlayerHealth = 200.f;	
		PlayerHealth = MaxPlayerHealth;
		bHasPet = false;
		TempHotbar.SetNum(10);
		TributeLevel = 1;
	}
};
