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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShortcutStateChanged, FName, ShortcutTag, bool, bOpened);

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

	/** 액터의 현재 맵 이름과 액터 이름을 조합하여 NoRegenItem 장부용 고유 키를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "WorldState|SaveGame")
	FString GetActorUniqueKey(const AActor* TargetActor) const;

	/** 액터가 이미 영구 삭제 목록(NoRegenItem)에 등록되어 있는지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "WorldState|SaveGame")
	bool IsActorRegisteredDead(const AActor* TargetActor) const;

	/** 액터가 영구 삭제 목록(NoRegenItem)에 등록되어 있다면 월드에서 즉시 Destroy하고 true를 반환합니다. (BeginPlay 전용) */
	UFUNCTION(BlueprintCallable, Category = "WorldState|SaveGame")
	bool CheckAndDestroyIfDead(AActor* TargetActor);

	/** 액터를 영구 삭제 목록(NoRegenItem)에 등록하고, bDestroyActor가 true이면 즉시 액터를 Destroy합니다. */
	UFUNCTION(BlueprintCallable, Category = "WorldState|SaveGame")
	bool RegisterDeadActor(AActor* TargetActor, bool bDestroyActor = true);

	// 숲 맵 숏컷 개방 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState|SaveGame")
	bool ForestShortCut = false;
	
	// 던전 맵 숏컷 개방 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState|SaveGame")
	bool DungeonShortCut = false;
	
	// 동굴 맵 숏컷 개방 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState|SaveGame")
	bool CaveShortCut = false;
	
	// 도시 맵 숏컷 개방 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState|SaveGame")
	bool CityShortCut = false;

	/** 숏컷 상태가 변경되었을 때 브로드캐스트되는 이벤트 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "WorldState|Shortcut")
	FOnShortcutStateChanged OnShortcutStateChanged;

	/** 숲 맵 숏컷 상태를 설정합니다. bAutoSave가 true이면 세이브파일에 즉시 저장합니다. */
	UFUNCTION(BlueprintCallable, Category = "WorldState|Shortcut")
	void SetForestShortcut(bool bOpened, bool bAutoSave = true);

	/** 숲 맵 숏컷이 열려있는지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "WorldState|Shortcut")
	bool IsForestShortcutOpened() const;

	/** 던전 맵 숏컷 상태를 설정합니다. bAutoSave가 true이면 세이브파일에 즉시 저장합니다. */
	UFUNCTION(BlueprintCallable, Category = "WorldState|Shortcut")
	void SetDungeonShortcut(bool bOpened, bool bAutoSave = true);

	/** 던전 맵 숏컷이 열려있는지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "WorldState|Shortcut")
	bool IsDungeonShortcutOpened() const;

	/** 동굴 맵 숏컷 상태를 설정합니다. bAutoSave가 true이면 세이브파일에 즉시 저장합니다. */
	UFUNCTION(BlueprintCallable, Category = "WorldState|Shortcut")
	void SetCaveShortcut(bool bOpened, bool bAutoSave = true);

	/** 동굴 맵 숏컷이 열려있는지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "WorldState|Shortcut")
	bool IsCaveShortcutOpened() const;

	/** 도시 맵 숏컷 상태를 설정합니다. bAutoSave가 true이면 세이브파일에 즉시 저장합니다. */
	UFUNCTION(BlueprintCallable, Category = "WorldState|Shortcut")
	void SetCityShortcut(bool bOpened, bool bAutoSave = true);

	/** 도시 맵 숏컷이 열려있는지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "WorldState|Shortcut")
	bool IsCityShortcutOpened() const;

	/** 태그명("Forest", "Dungeon", "Cave", "City")으로 숏컷 상태를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "WorldState|Shortcut")
	void SetShortcutState(FName ShortcutTag, bool bOpened, bool bAutoSave = true);

	/** 태그명으로 숏컷 상태를 조회합니다. */
	UFUNCTION(BlueprintPure, Category = "WorldState|Shortcut")
	bool IsShortcutOpened(FName ShortcutTag) const;
	
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
