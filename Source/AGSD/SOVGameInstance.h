// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayLogSubsystem.h"
#include "PlotSaveData.h"
#include "Alchemy/AlchemySaveData.h"
#include "SaveData.h"
#include "Struct_InventorySlotData.h"
#include "Struct_WorldChestData.h"
#include "Engine/GameInstance.h"
#include "SOVGameInstance.generated.h"

/**
 * 
 */

UCLASS()
class AGSD_API USOVGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	USOVGameInstance();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame")
	FString SaveGameSlot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
	TArray<FString> NoRegenItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
	TSoftObjectPtr<UWorld> Level;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
	FName TeleportationTag = FName("None");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
	int CurrentDay = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
	float CurrentTime = 360.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
	TMap<FString, FStruct_WorldChestData> ChestMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
	int32 ShardsAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	int Coin = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerState")
	float Damage = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	float MaxPlayerHealth = 200.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	float PlayerHealth = MaxPlayerHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "PlayerState")
	bool bCombatStatsInitialized = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	bool bHasPet = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	int32 CurrentSelectedHotbar = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	TArray<FStruct_InventorySlotData> TempInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	TArray<FStruct_InventorySlotData> TempHotbar;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FarmingState")
	TMap<FString, FPlotSaveData> GlobalPlotDataMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FarmingState")
	TMap<FString, FAppleTreeSaveData> GlobalAppleTreeDataMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlchemyTableState")
	TMap<FString, FAlchemySaveData> AlchemyTableDataMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "TributeState")
	int32 TributeLevel = 1;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "TributeState")
	TMap<FString, int32> CurrentLevelTributeItems;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnemyItemDrop")
	TArray<FString> AlreadyDroppedItems;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Setting")
	float MouseSensitivity = 5.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Setting")
	FString PlayerSessionID = TEXT("Unknown");

	// 경작지로부터 데이터를 받아서 저장/갱신하는 함수
	UFUNCTION(BlueprintCallable, Category = "Farming")
	void UpdatePlotData(FPlotSaveData NewData);

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void UpdateAppleTreeData(FAppleTreeSaveData NewData);

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void UpdateTableData(FAlchemySaveData NewData);
	
	// GUID로 데이터를 꺼내주는 함수 (나중에 로드할 때 씀)
	UFUNCTION(BlueprintCallable, Category = "Farming")
	bool GetPlotData(FString name, FPlotSaveData& OutData);

	UFUNCTION(BlueprintCallable, Category = "Farming")
	bool GetAppleTreeData(FString name, FAppleTreeSaveData& OutData);

	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	bool GetAlchemyData(FString name, FAlchemySaveData& OutData);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FSaveData GetSaveData();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SetSaveData(FSaveData SaveData);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SaveGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadGame();

	UFUNCTION(BlueprintCallable, Category = "GameInstance")
	void InitializeVariables();

	/** 주어진 월드가 메인 메뉴나 타이틀이 아닌 실제 플레이용 게임플레이 맵인지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	static bool IsGameplayMap(const UWorld* World);



	/** GameInstance 메모리에 저장된 인벤토리 데이터를 완전히 비웁니다. */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void ClearGameInstanceInventory();

	/** GameInstance의 인벤토리 데이터에 ID 기반으로 아이템을 추가합니다. (기존 중첩 스택 반영 및 빈 슬롯 배치) */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool AddGameInstanceItemByID(const FString& ItemID, int32 Amount, class UDataTable* ItemDataTable);

	// 튜토리얼 마을 진입시 엔딩 플래그 트루면 엔딩 컷씬 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	bool bIsEnding = false;
	
protected:
	// 게임 인스턴스가 초기화될 때 호출되는 함수 오버라이드
	virtual void Init() override;

	// 게임 인스턴스가 종료될 때 호출되는 함수 오버라이드
	virtual void Shutdown() override;

	// 클린업 시 실행될 실제 로직 함수
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	// 새 맵이 열릴 때 호출되어 맵 도착 시점 체크포인트를 저장하는 콜백
	void HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS);
};
