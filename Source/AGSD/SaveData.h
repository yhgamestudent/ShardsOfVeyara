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
	FString PlayerSessionID;
	
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

	/** 완료된 튜토리얼/퀘스트 시퀀스 이름 목록 */
	UPROPERTY()
	TArray<FName> CompletedTutorialSequences;

	/** 퀘스트 클리어로 인해 열린/비활성화된 게이트(블로킹 볼륨) 태그 목록 */
	UPROPERTY()
	TArray<FName> OpenedGateTags;

	/** 거점 튜토리얼 완료 여부 플래그 */
	UPROPERTY()
	bool bCompletedHubTutorial;

	/** 진행 중이던 튜토리얼 시퀀스 이름 (없으면 NAME_None) */
	UPROPERTY()
	FName ActiveTutorialSequence;

	/** 진행 중이던 튜토리얼 데이터테이블 소프트 참조 */
	UPROPERTY()
	TSoftObjectPtr<UDataTable> ActiveTutorialTable;

	/** 진행 중이던 튜토리얼 스텝 인덱스 (0-based) */
	UPROPERTY()
	int32 ActiveTutorialStepIndex;

	/** 진행 중이던 스텝의 세부 목표별 진행 카운트 목록 */
	UPROPERTY()
	TArray<int32> ActiveTutorialObjectiveCounts;

	/** 게임 종료 직전 플레이어 저장 위치 */
	UPROPERTY()
	FVector SavedPlayerLocation;

	/** 게임 종료 직전 플레이어 저장 회전 */
	UPROPERTY()
	FRotator SavedPlayerRotation;

	/** 숲 맵 숏컷 개방 여부 */
	UPROPERTY()
	bool ForestShortCut;

	/** 던전 맵 숏컷 개방 여부 */
	UPROPERTY()
	bool DungeonShortCut;

	/** 동굴 맵 숏컷 개방 여부 */
	UPROPERTY()
	bool CaveShortCut;

	/** 도시 맵 숏컷 개방 여부 */
	UPROPERTY()
	bool CityShortCut;

	FSaveData()
	{
		PlayerSessionID = TEXT("Unknown");
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
		bCompletedHubTutorial = false;
		ActiveTutorialSequence = NAME_None;
		ActiveTutorialStepIndex = 0;
		SavedPlayerLocation = FVector::ZeroVector;
		SavedPlayerRotation = FRotator::ZeroRotator;
		ForestShortCut = false;
		DungeonShortCut = false;
		CaveShortCut = false;
		CityShortCut = false;
	}
};
