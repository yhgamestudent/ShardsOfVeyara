// Fill out your copyright notice in the Description page of Project Settings.


#include "SOVGameInstance.h"
#include "SOVSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Struct_ItemData.h"
#include "Engine/DataTable.h"
#include "GameplayLogSubsystem.h"
#include "Tutorial/TutorialSubsystem.h"

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
	bCombatStatsInitialized = false;
	bHasPet = DefaultInstance->bHasPet;
	CurrentSelectedHotbar = DefaultInstance->CurrentSelectedHotbar;
	TributeLevel = DefaultInstance->TributeLevel;
	MouseSensitivity = DefaultInstance->MouseSensitivity;
	bIsEnding = DefaultInstance->bIsEnding;
	ForestShortCut = DefaultInstance->ForestShortCut;
	DungeonShortCut = DefaultInstance->DungeonShortCut;
	CaveShortCut = DefaultInstance->CaveShortCut;
	CityShortCut = DefaultInstance->CityShortCut;

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
	SaveData.PlayerSessionID = PlayerSessionID;
	SaveData.NoRegenItem = NoRegenItem;

	// 현재 활성 월드가 유효한 게임플레이 맵인 경우 최신 Level 정보 갱신
	if (UWorld* CurrentWorld = GetWorld())
	{
		if ((CurrentWorld->IsGameWorld() || CurrentWorld->IsPlayInEditor()) && IsGameplayMap(CurrentWorld))
		{
			Level = TSoftObjectPtr<UWorld>(CurrentWorld);
		}
	}
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
	SaveData.ForestShortCut = ForestShortCut;
	SaveData.DungeonShortCut = DungeonShortCut;
	SaveData.CaveShortCut = CaveShortCut;
	SaveData.CityShortCut = CityShortCut;
	
	if (UGameplayLogSubsystem* LogSubsystem = GetSubsystem<UGameplayLogSubsystem>())
	{
		SaveData.LogData = LogSubsystem->GetLogData();
	}

	// 튜토리얼 완료 상태 및 해제된 게이트 태그 영구 저장 (중간 진행도는 저장하지 않고 맵 도착 시점으로 리셋)
	if (UTutorialSubsystem* TutSub = GetSubsystem<UTutorialSubsystem>())
	{
		SaveData.ActiveTutorialSequence = NAME_None;
		SaveData.ActiveTutorialStepIndex = 0;
		SaveData.ActiveTutorialObjectiveCounts.Empty();
		SaveData.CompletedTutorialSequences = TutSub->GetCompletedSequences();
		SaveData.OpenedGateTags = TutSub->GetOpenedGateTags();
		SaveData.bCompletedHubTutorial = TutSub->IsHubTutorialCompleted();
	}
	
	return SaveData; 
}

void USOVGameInstance::SetSaveData(FSaveData SaveData)
{
	PlayerSessionID = SaveData.PlayerSessionID;
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
	ForestShortCut = SaveData.ForestShortCut;
	DungeonShortCut = SaveData.DungeonShortCut;
	CaveShortCut = SaveData.CaveShortCut;
	CityShortCut = SaveData.CityShortCut;

	if (UGameplayLogSubsystem* LogSubsystem = GetSubsystem<UGameplayLogSubsystem>())
	{
		LogSubsystem->SetLogData(SaveData.LogData);
	}

	// 튜토리얼 완료 상태 및 해제된 게이트 태그 복원
	if (UTutorialSubsystem* TutSub = GetSubsystem<UTutorialSubsystem>())
	{
		TutSub->SetCompletedSequences(SaveData.CompletedTutorialSequences);
		TutSub->SetOpenedGateTags(SaveData.OpenedGateTags);
		TutSub->SetHubTutorialCompleted(SaveData.bCompletedHubTutorial);
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

		// CSV 로그 파일 자동 동기화 내보내기
		if (UGameplayLogSubsystem* LogSubsystem = GetSubsystem<UGameplayLogSubsystem>())
		{
			LogSubsystem->ExportLogsToCSVFile(TEXT(""));
		}
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
			// 불러온 데이터를 인스턴스 변수에 적용 (기존 PlayerSessionID 유지)
			SetSaveData(LoadedInstance->SaveData);
		}
		else
		{
			// 세이브 파일이 없으면 새 게임 시작 — 새 PlayerSessionID 생성
			if (UGameplayLogSubsystem* LogSubsystem = GetSubsystem<UGameplayLogSubsystem>())
			{
				LogSubsystem->InitNewSession();
			}
		}
	}
}

void USOVGameInstance::Init()
{
	Super::Init();

	// 월드 초기화가 완료된 후 호출되는 델리게이트에 바인딩 (맵이 열릴 때)
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &USOVGameInstance::HandleWorldInitialized);
}

void USOVGameInstance::Shutdown()
{
	// 튜토리얼 도중 종료 시 중간 상태를 덮어쓰지 않고 맵 도착 시점의 세이브가 보존되도록 처리
	Super::Shutdown();
}

void USOVGameInstance::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
}

bool USOVGameInstance::IsGameplayMap(const UWorld* World)
{
	if (!World)
	{
		return false;
	}

	FString CleanMapName = World->GetMapName();
	World->RemovePIEPrefix(CleanMapName);
	CleanMapName = FPaths::GetBaseFilename(CleanMapName);

	// 메인 메뉴, 타이틀, 엔트리 등 비플레이 레벨 필터링
	if (CleanMapName.IsEmpty() ||
		CleanMapName.Contains(TEXT("MainMenu"), ESearchCase::IgnoreCase) ||
		CleanMapName.Contains(TEXT("Title"), ESearchCase::IgnoreCase) ||
		CleanMapName.Contains(TEXT("Entry"), ESearchCase::IgnoreCase))
	{
		return false;
	}

	return true;
}

void USOVGameInstance::HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (!World || (!World->IsGameWorld() && !World->IsPlayInEditor()))
	{
		return;
	}

	// 메인 메뉴나 타이틀 맵인 경우 세이브 오염 방지를 위해 갱신 및 저장 차단
	if (!IsGameplayMap(World))
	{
		UE_LOG(LogTemp, Log, TEXT("[USOVGameInstance] Skipped auto-save on non-gameplay map: %s"), *World->GetMapName());
		return;
	}

	// 활성 세이브 슬롯이 지정되어 있지 않은 경우 저장 차단
	if (SaveGameSlot.IsEmpty())
	{
		return;
	}

	// 1. 현재 로드된 레벨 정보 갱신
	Level = TSoftObjectPtr<UWorld>(World);

	// 2. 맵 도착 시점 체크포인트 자동 저장
	SaveGame();
	UE_LOG(LogTemp, Log, TEXT("[USOVGameInstance] Checkpoint auto-saved on map arrival: %s (Slot: %s)"),
		*World->GetMapName(), *SaveGameSlot);
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

FString USOVGameInstance::GetActorUniqueKey(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FString();
	}

	FString MapName = TEXT("");
	if (const UWorld* World = TargetActor->GetWorld())
	{
		MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	}

	return MapName + TEXT("_") + TargetActor->GetName();
}

bool USOVGameInstance::IsActorRegisteredDead(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const FString UniqueKey = GetActorUniqueKey(TargetActor);
	return NoRegenItem.Contains(UniqueKey) || NoRegenItem.Contains(TargetActor->GetName());
}

bool USOVGameInstance::CheckAndDestroyIfDead(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	if (IsActorRegisteredDead(TargetActor))
	{
		UE_LOG(LogTemp, Log, TEXT("CheckAndDestroyIfDead - Actor [%s] is already registered in NoRegenItem. Destroying..."), *TargetActor->GetName());
		TargetActor->Destroy();
		return true;
	}

	return false;
}

bool USOVGameInstance::RegisterDeadActor(AActor* TargetActor, bool bDestroyActor)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const FString UniqueKey = GetActorUniqueKey(TargetActor);
	NoRegenItem.AddUnique(UniqueKey);

	UE_LOG(LogTemp, Log, TEXT("RegisterDeadActor - Registered Actor [%s] with Key [%s] to NoRegenItem."), *TargetActor->GetName(), *UniqueKey);

	if (bDestroyActor)
	{
		TargetActor->Destroy();
	}

	return true;
}

void USOVGameInstance::SetForestShortcut(bool bOpened, bool bAutoSave)
{
	ForestShortCut = bOpened;
	OnShortcutStateChanged.Broadcast(FName("Forest"), bOpened);
	UE_LOG(LogTemp, Log, TEXT("[USOVGameInstance] Forest shortcut state changed to: %s"), bOpened ? TEXT("Opened") : TEXT("Closed"));

	if (bAutoSave)
	{
		SaveGame();
	}
}

bool USOVGameInstance::IsForestShortcutOpened() const
{
	return ForestShortCut;
}

void USOVGameInstance::SetDungeonShortcut(bool bOpened, bool bAutoSave)
{
	DungeonShortCut = bOpened;
	OnShortcutStateChanged.Broadcast(FName("Dungeon"), bOpened);
	UE_LOG(LogTemp, Log, TEXT("[USOVGameInstance] Dungeon shortcut state changed to: %s"), bOpened ? TEXT("Opened") : TEXT("Closed"));

	if (bAutoSave)
	{
		SaveGame();
	}
}

bool USOVGameInstance::IsDungeonShortcutOpened() const
{
	return DungeonShortCut;
}

void USOVGameInstance::SetCaveShortcut(bool bOpened, bool bAutoSave)
{
	CaveShortCut = bOpened;
	OnShortcutStateChanged.Broadcast(FName("Cave"), bOpened);
	UE_LOG(LogTemp, Log, TEXT("[USOVGameInstance] Cave shortcut state changed to: %s"), bOpened ? TEXT("Opened") : TEXT("Closed"));

	if (bAutoSave)
	{
		SaveGame();
	}
}

bool USOVGameInstance::IsCaveShortcutOpened() const
{
	return CaveShortCut;
}

void USOVGameInstance::SetCityShortcut(bool bOpened, bool bAutoSave)
{
	CityShortCut = bOpened;
	OnShortcutStateChanged.Broadcast(FName("City"), bOpened);
	UE_LOG(LogTemp, Log, TEXT("[USOVGameInstance] City shortcut state changed to: %s"), bOpened ? TEXT("Opened") : TEXT("Closed"));

	if (bAutoSave)
	{
		SaveGame();
	}
}

bool USOVGameInstance::IsCityShortcutOpened() const
{
	return CityShortCut;
}

void USOVGameInstance::SetShortcutState(FName ShortcutTag, bool bOpened, bool bAutoSave)
{
	const FString TagStr = ShortcutTag.ToString();
	if (TagStr.Equals(TEXT("Forest"), ESearchCase::IgnoreCase))
	{
		SetForestShortcut(bOpened, bAutoSave);
	}
	else if (TagStr.Equals(TEXT("Dungeon"), ESearchCase::IgnoreCase))
	{
		SetDungeonShortcut(bOpened, bAutoSave);
	}
	else if (TagStr.Equals(TEXT("Cave"), ESearchCase::IgnoreCase))
	{
		SetCaveShortcut(bOpened, bAutoSave);
	}
	else if (TagStr.Equals(TEXT("City"), ESearchCase::IgnoreCase))
	{
		SetCityShortcut(bOpened, bAutoSave);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[USOVGameInstance] Unknown shortcut tag: %s"), *TagStr);
	}
}

bool USOVGameInstance::IsShortcutOpened(FName ShortcutTag) const
{
	const FString TagStr = ShortcutTag.ToString();
	if (TagStr.Equals(TEXT("Forest"), ESearchCase::IgnoreCase))
	{
		return ForestShortCut;
	}
	else if (TagStr.Equals(TEXT("Dungeon"), ESearchCase::IgnoreCase))
	{
		return DungeonShortCut;
	}
	else if (TagStr.Equals(TEXT("Cave"), ESearchCase::IgnoreCase))
	{
		return CaveShortCut;
	}
	else if (TagStr.Equals(TEXT("City"), ESearchCase::IgnoreCase))
	{
		return CityShortCut;
	}

	UE_LOG(LogTemp, Warning, TEXT("[USOVGameInstance] Unknown shortcut tag: %s"), *TagStr);
	return false;
}

