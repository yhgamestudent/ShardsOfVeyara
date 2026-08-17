// Fill out your copyright notice in the Description page of Project Settings.


#include "SOVGameInstance.h"
#include "SOVSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Struct_ItemData.h"
#include "Engine/DataTable.h"
#include "GameplayLogSubsystem.h"

USOVGameInstance::USOVGameInstance()
{
	InitializeVariables();
}

void USOVGameInstance::InitializeVariables()
{
	// 블루프린트 에디터에서 설정한 기본값들이 담긴 원본 객체(CDO)를 가져옵니다.
	const USOVGameInstance* DefaultInstance = GetDefault<USOVGameInstance>(GetClass());
	if (!DefaultInstance) return;

	// 에디터 설정값을 그대로 복사 (Level, 시간, 체력 등)
	NoRegenItem = DefaultInstance->NoRegenItem;
	Level = DefaultInstance->Level;
	TeleportationTag = DefaultInstance->TeleportationTag;
	CurrentDay = DefaultInstance->CurrentDay;
	CurrentTime = DefaultInstance->CurrentTime;
	ShardsAmount = DefaultInstance->ShardsAmount;
	Coin = DefaultInstance->Coin;
	Damage = DefaultInstance->Damage;
	MaxPlayerHealth = DefaultInstance->MaxPlayerHealth;
	PlayerHealth = MaxPlayerHealth;
	bHasPet = DefaultInstance->bHasPet;
	CurrentSelectedHotbar = DefaultInstance->CurrentSelectedHotbar;
	TributeLevel = DefaultInstance->TributeLevel;
	MouseSensitivity = DefaultInstance->MouseSensitivity;
	
	// 데이터 맵 및 인벤토리는 비워줌 (필요 시 CDO에서 복사 가능)
	ChestMap.Empty();
	TempInventory.Empty();
	TempHotbar.Empty();
	TempHotbar.SetNum(10);
	GlobalPlotDataMap.Empty();
	AlchemyTableDataMap.Empty();
	CurrentLevelTributeItems.Empty();
	AlreadyDroppedItems.Empty();
}

void USOVGameInstance::UpdatePlotData(FPlotSaveData NewData)
{
	// 장부에 기록 (이미 있으면 덮어쓰기, 없으면 추가)
	GlobalPlotDataMap.Add(NewData.PlotName, NewData);
}

void USOVGameInstance::UpdateAppleTreeData(FAppleTreeSaveData NewData)
{
	GlobalAppleTreeDataMap.Add(NewData.TreeName, NewData);
}

void USOVGameInstance::UpdateTableData(FAlchemySaveData NewData)
{
	// 장부에 기록 (이미 있으면 덮어쓰기, 없으면 추가)
	AlchemyTableDataMap.Add(NewData.TableName, NewData);
}

bool USOVGameInstance::GetPlotData(FString name, FPlotSaveData& OutData)
{
	if (FPlotSaveData* FoundData = GlobalPlotDataMap.Find(name))
	{
		OutData = *FoundData;
		return true;
	}
	return false;
}

bool USOVGameInstance::GetAppleTreeData(FString name, FAppleTreeSaveData& OutData)
{
	if (FAppleTreeSaveData* FoundData = GlobalAppleTreeDataMap.Find(name))
	{
		OutData = *FoundData;
		return true;
	}
	return false;
}

bool USOVGameInstance::GetAlchemyData(FString name, FAlchemySaveData& OutData)
{
	if (FAlchemySaveData* FoundData = AlchemyTableDataMap.Find(name))
	{
		OutData = *FoundData;
		return true;
	}
	return false;
}

FSaveData USOVGameInstance::GetSaveData()
{
	FSaveData SaveData;
	SaveData.NoRegenItem = NoRegenItem;
	SaveData.Level = Level;
	SaveData.TeleportationTag = TeleportationTag;
	SaveData.CurrentDay = CurrentDay;
	SaveData.CurrentTime = CurrentTime;
	SaveData.ChestMap = ChestMap;
	SaveData.ShardsAmount = ShardsAmount;
	SaveData.Coin = Coin;
	SaveData.Damage = Damage;
	SaveData.MaxPlayerHealth = MaxPlayerHealth;
	SaveData.PlayerHealth = PlayerHealth;
	SaveData.bHasPet = bHasPet;
	SaveData.TempInventory = TempInventory;
	SaveData.TempHotbar = TempHotbar;
	SaveData.GlobalPlotDataMap = GlobalPlotDataMap;
	SaveData.GlobalAppleTreeDataMap = GlobalAppleTreeDataMap;
	SaveData.AlchemyTableDataMap = AlchemyTableDataMap;
	SaveData.TributeLevel = TributeLevel;
	SaveData.CurrentLevelTributeItems = CurrentLevelTributeItems;
	SaveData.LastSaveTime = FDateTime::Now();
	SaveData.AlreadyDroppedItems = AlreadyDroppedItems;
	SaveData.MouseSensitivity = MouseSensitivity;
	
	if (UGameplayLogSubsystem* LogSubsystem = GetSubsystem<UGameplayLogSubsystem>())
	{
		SaveData.LogData = LogSubsystem->GetLogData();
	}
	
	return SaveData; 
}

void USOVGameInstance::SetSaveData(FSaveData SaveData)
{
	NoRegenItem = SaveData.NoRegenItem;
	Level = SaveData.Level;
	TeleportationTag = SaveData.TeleportationTag;
	CurrentDay = SaveData.CurrentDay;
	CurrentTime = SaveData.CurrentTime;
	ChestMap = SaveData.ChestMap;
	ShardsAmount = SaveData.ShardsAmount;
	Coin = SaveData.Coin;
	Damage = SaveData.Damage;
	MaxPlayerHealth = SaveData.MaxPlayerHealth;
	PlayerHealth = SaveData.PlayerHealth;
	bHasPet = SaveData.bHasPet;
	TempInventory = SaveData.TempInventory;
	TempHotbar = SaveData.TempHotbar;
	GlobalPlotDataMap = SaveData.GlobalPlotDataMap;
	GlobalAppleTreeDataMap = SaveData.GlobalAppleTreeDataMap;
	AlchemyTableDataMap = SaveData.AlchemyTableDataMap;
	TributeLevel = SaveData.TributeLevel;
	CurrentLevelTributeItems = SaveData.CurrentLevelTributeItems;
	AlreadyDroppedItems = SaveData.AlreadyDroppedItems;
	MouseSensitivity = SaveData.MouseSensitivity;

	if (UGameplayLogSubsystem* LogSubsystem = GetSubsystem<UGameplayLogSubsystem>())
	{
		LogSubsystem->SetLogData(SaveData.LogData);
	}
}

void USOVGameInstance::SaveGame()
{
	if (SaveGameSlot.IsEmpty()) return;

	// 세이브 객체 생성
	USOVSaveGame* SaveInstance = Cast<USOVSaveGame>(UGameplayStatics::CreateSaveGameObject(USOVSaveGame::StaticClass()));
    
	if (SaveInstance)
	{
		// 데이터 채우기
		SaveInstance->SaveData = GetSaveData();

		// 파일 쓰기 (동기식)
		UGameplayStatics::SaveGameToSlot(SaveInstance, SaveGameSlot, 0);
	}
}

void USOVGameInstance::LoadGame()
{
	if (SaveGameSlot.IsEmpty()) return;

	// 해당 슬롯에 세이브 파일이 있는지 확인
	if (UGameplayStatics::DoesSaveGameExist(SaveGameSlot, 0))
	{
		// 파일 읽기
		USOVSaveGame* LoadedInstance = Cast<USOVSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveGameSlot, 0));
        
		if (LoadedInstance)
		{
			// 불러온 데이터를 인스턴스 변수에 적용
			SetSaveData(LoadedInstance->SaveData);
		}
	}
}

void USOVGameInstance::Init()
{
	Super::Init();

	// 월드가 정리될 때 호출되는 델리게이트에 HandleWorldCleanup 함수를 연결(Bind)합니다.
	//FWorldDelegates::OnWorldCleanup.AddUObject(this, &USOVGameInstance::HandleWorldCleanup);

	// 2. 월드 초기화가 완료된 후 호출되는 델리게이트에 바인딩 (맵이 열릴 때)
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &USOVGameInstance::HandleWorldInitialized);
}

void USOVGameInstance::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	// 실제 게임 월드이거나 에디터 플레이(PIE) 중일 때만 자동 저장 실행
	if (World && (World->IsGameWorld() || World->IsPlayInEditor()))
	{
		// 현재 설정된 스트링(SaveGameSlot)으로 저장 실행
		SaveGame();
	}
}

void USOVGameInstance::HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS)
{
	// 실제 게임 월드이거나 에디터 플레이(PIE) 중일 때만 자동 저장 실행
	if (World && (World->IsGameWorld() || World->IsPlayInEditor()))
	{
		// 현재 설정된 스트링(SaveGameSlot)으로 저장 실행
		SaveGame();
	}
}

void USOVGameInstance::ClearGameInstanceInventory()
{
	// 기본 30칸으로 설정하여 비운 상태의 인벤토리 생성
	TempInventory.Empty();
	TempInventory.SetNum(30);

	for (int32 i = 0; i < 30; ++i)
	{
		TempInventory[i] = FStruct_InventorySlotData();
		TempInventory[i].SlotIndex = i;
		TempInventory[i].IsEmpty = true;
	}

	// Hotbar 선택 인덱스 초기화
	CurrentSelectedHotbar = 0;

	UE_LOG(LogTemp, Log, TEXT("ClearGameInstanceInventory - GameInstance inventory has been cleared."));
}

bool USOVGameInstance::AddGameInstanceItemByID(const FString& ItemID, int32 Amount, UDataTable* ItemDataTable)
{
	if (Amount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("USOVGameInstance::AddGameInstanceItemByID - Amount is less than or equal to 0."));
		return false;
	}

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("USOVGameInstance::AddGameInstanceItemByID - ItemDataTable is null."));
		return false;
	}

	FStruct_ItemData* RowData = ItemDataTable->FindRow<FStruct_ItemData>(FName(*ItemID), TEXT("AddGameInstanceItemByID"));
	if (!RowData)
	{
		UE_LOG(LogTemp, Warning, TEXT("USOVGameInstance::AddGameInstanceItemByID - Failed to find Item ID [%s] in DataTable."), *ItemID);
		return false;
	}

	// TempInventory가 비어있다면, 기본 크기(30칸)로 초기화
	if (TempInventory.Num() <= 0)
	{
		TempInventory.SetNum(30);
		for (int32 i = 0; i < 30; ++i)
		{
			TempInventory[i] = FStruct_InventorySlotData();
			TempInventory[i].SlotIndex = i;
			TempInventory[i].IsEmpty = true;
		}
	}

	int32 OutRemainingQty = Amount;
	bool bAddedAny = false;

	FStruct_ItemData ItemDataToAdd = *RowData;
	ItemDataToAdd.CurrentQuantity = Amount; // 추가할 수량 정보 임시 저장

	// 1단계: 기존 스택에 중첩 시도
	for (int32 i = 0; i < TempInventory.Num() && OutRemainingQty > 0; ++i)
	{
		if (!TempInventory[i].IsEmpty &&
			TempInventory[i].ItemData.ItemID == ItemID &&
			TempInventory[i].ItemData.CurrentQuantity < TempInventory[i].ItemData.MaxQuantity)
		{
			const int32 SpaceAvailable = TempInventory[i].ItemData.MaxQuantity - TempInventory[i].ItemData.CurrentQuantity;
			const int32 AmountToAdd = FMath::Min(SpaceAvailable, OutRemainingQty);

			TempInventory[i].ItemData.CurrentQuantity += AmountToAdd;
			OutRemainingQty -= AmountToAdd;
			bAddedAny = true;
		}
	}

	// 2단계: 빈 슬롯에 배치
	for (int32 i = 0; i < TempInventory.Num() && OutRemainingQty > 0; ++i)
	{
		if (TempInventory[i].IsEmpty)
		{
			const int32 AmountToPlace = FMath::Min(ItemDataToAdd.MaxQuantity, OutRemainingQty);

			TempInventory[i].IsEmpty = false;
			TempInventory[i].ItemData = ItemDataToAdd;
			TempInventory[i].ItemData.CurrentQuantity = AmountToPlace;
			TempInventory[i].SlotIndex = i;
			
			OutRemainingQty -= AmountToPlace;
			bAddedAny = true;
		}
	}

	if (bAddedAny)
	{
		UE_LOG(LogTemp, Log, TEXT("AddGameInstanceItemByID - Successfully added Item [%s], Remaining Amount to add: %d"), *ItemID, OutRemainingQty);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AddGameInstanceItemByID - Failed to add Item [%s] (Inventory might be full)."), *ItemID);
	}

	return bAddedAny;
}
