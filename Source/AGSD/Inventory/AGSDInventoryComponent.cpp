// AGSDInventoryComponent.cpp - 인벤토리 컴포넌트 구현
// 기존 블루프린트 BPAC_InventoryComponent의 모든 로직을 C++로 재구현합니다.

#include "AGSDInventoryComponent.h"
#include "AGSDCharacter.h"
#include "PickUpItem.h"
#include "SOVGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"

UAGSDInventoryComponent::UAGSDInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ═══════════════════════════════════════════════════
// 라이프사이클
// ═══════════════════════════════════════════════════

void UAGSDInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeSlots();
}

void UAGSDInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SaveToGameInstance();
	Super::EndPlay(EndPlayReason);
}

// ═══════════════════════════════════════════════════
// 초기화
// ═══════════════════════════════════════════════════

void UAGSDInventoryComponent::InitializeSlots()
{
	// GameInstance에서 데이터 복원 시도
	LoadFromGameInstance();

	// 복원된 데이터가 없거나 크기가 맞지 않으면 새로 초기화
	if (InventorySlots.Num() != MaxSlots)
	{
		InventorySlots.Empty();
		InventorySlots.SetNum(MaxSlots);

		for (int32 i = 0; i < MaxSlots; ++i)
		{
			InventorySlots[i] = FStruct_InventorySlotData();
			InventorySlots[i].SlotIndex = i;
			InventorySlots[i].IsEmpty = true;
		}

		// 빈 인벤토리로 최초 초기화되었으므로 에디터 지정 디폴트 아이템들을 슬롯에 지급
		for (const FStruct_DefaultInventoryItem& DefaultItem : DefaultItems)
		{
			if (!DefaultItem.ItemID.IsEmpty() && DefaultItem.Amount > 0)
			{
				int32 RemainingQty = DefaultItem.Amount;
				FStruct_ItemData OutItemData;
				AddItemByID(DefaultItem.ItemID, DefaultItem.Amount, RemainingQty, OutItemData);
			}
		}
	}

	AActor* Owner = GetOwner();
	bool bIsPlayer = Owner && Owner->IsA(AAGSDCharacter::StaticClass());

	// 핫바 선택 인덱스도 GameInstance에서 복원 (플레이어 캐릭터인 경우에만)
	if (bIsPlayer)
	{
		if (USOVGameInstance* GI = GetGameInstance())
		{
			CurrentSelectedHotbar = FMath::Clamp(GI->CurrentSelectedHotbar, HOTBAR_START_INDEX, HOTBAR_END_INDEX);
		}
	}

	OnInventoryFullyUpdated.Broadcast();
	
	// 초기 핫바 선택 상태 알림 (UI 동기화용, 플레이어 캐릭터인 경우에만)
	if (bIsPlayer)
	{
		OnHotbarSelectionChanged.Broadcast(-1, CurrentSelectedHotbar);
	}
}

void UAGSDInventoryComponent::ClearAllSlots()
{
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		InventorySlots[i].IsEmpty = true;
		InventorySlots[i].ItemData = FStruct_ItemData();
		InventorySlots[i].SlotIndex = i;
	}

	// UI 갱신 이벤트 전달
	if (OnInventoryFullyUpdated.IsBound())
	{
		OnInventoryFullyUpdated.Broadcast();
	}
}

// ═══════════════════════════════════════════════════
// 아이템 추가
// ═══════════════════════════════════════════════════

bool UAGSDInventoryComponent::AddItem(FStruct_ItemData ItemData, int32& OutRemainingQty)
{
	OutRemainingQty = ItemData.CurrentQuantity;

	if (OutRemainingQty <= 0)
	{
		return false;
	}

	bool bAddedAny = false;

	// 1단계: 기존 스택에 중첩 시도
	for (int32 i = 0; i < MaxSlots && OutRemainingQty > 0; ++i)
	{
		if (!InventorySlots[i].IsEmpty &&
			InventorySlots[i].ItemData.ItemID == ItemData.ItemID &&
			InventorySlots[i].ItemData.CurrentQuantity < InventorySlots[i].ItemData.MaxQuantity)
		{
			const int32 SpaceAvailable = InventorySlots[i].ItemData.MaxQuantity - InventorySlots[i].ItemData.CurrentQuantity;
			const int32 AmountToAdd = FMath::Min(SpaceAvailable, OutRemainingQty);

			InventorySlots[i].ItemData.CurrentQuantity += AmountToAdd;
			OutRemainingQty -= AmountToAdd;
			bAddedAny = true;

			OnInventorySlotUpdated.Broadcast(i);
			OnItemAdded.Broadcast(i, InventorySlots[i].ItemData);
		}
	}

	// 2단계: 빈 슬롯에 배치
	while (OutRemainingQty > 0)
	{
		const int32 EmptySlot = FindEmptySlot();
		if (EmptySlot == -1)
		{
			// 인벤토리 가득 참
			break;
		}

		const int32 AmountToPlace = FMath::Min(ItemData.MaxQuantity, OutRemainingQty);

		InventorySlots[EmptySlot].IsEmpty = false;
		InventorySlots[EmptySlot].ItemData = ItemData;
		InventorySlots[EmptySlot].ItemData.CurrentQuantity = AmountToPlace;
		OutRemainingQty -= AmountToPlace;
		bAddedAny = true;

		OnInventorySlotUpdated.Broadcast(EmptySlot);
		OnItemAdded.Broadcast(EmptySlot, InventorySlots[EmptySlot].ItemData);
	}

	return bAddedAny;
}

bool UAGSDInventoryComponent::AddItemByID(const FString& ItemID, int32 Amount, int32& OutRemainingQty, FStruct_ItemData& OutItemData)
{
	OutRemainingQty = Amount;

	if (OutRemainingQty <= 0)
	{
		return false;
	}

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAGSDInventoryComponent::AddItemByID - ItemDataTable is null."));
		return false;
	}

	FStruct_ItemData* RowData = ItemDataTable->FindRow<FStruct_ItemData>(FName(*ItemID), TEXT("AddItemByID"));
	if (!RowData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAGSDInventoryComponent::AddItemByID - Failed to find Item ID [%s] in DataTable."), *ItemID);
		return false;
	}

	OutItemData = *RowData;
	OutItemData.CurrentQuantity = Amount;

	return AddItem(OutItemData, OutRemainingQty);
}

// ═══════════════════════════════════════════════════
// 아이템 제거
// ═══════════════════════════════════════════════════

bool UAGSDInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
	if (!ValidateSlotIndex(SlotIndex, TEXT("RemoveItem"))) return false;
	if (InventorySlots[SlotIndex].IsEmpty) return false;

	const FStruct_ItemData RemovedItemData = InventorySlots[SlotIndex].ItemData;

	if (Quantity <= 0 || Quantity >= InventorySlots[SlotIndex].ItemData.CurrentQuantity)
	{
		// 전체 제거
		ClearSlot(SlotIndex);
	}
	else
	{
		// 일부 제거
		InventorySlots[SlotIndex].ItemData.CurrentQuantity -= Quantity;
		OnInventorySlotUpdated.Broadcast(SlotIndex);
	}

	OnItemRemoved.Broadcast(SlotIndex, RemovedItemData);
	return true;
}

// ═══════════════════════════════════════════════════
// 슬롯 교환
// ═══════════════════════════════════════════════════

void UAGSDInventoryComponent::SwapSlots(int32 FromIndex, int32 ToIndex)
{
	if (!ValidateSlotIndex(FromIndex, TEXT("SwapSlots (From)"))) return;
	if (!ValidateSlotIndex(ToIndex, TEXT("SwapSlots (To)"))) return;
	if (FromIndex == ToIndex) return;

	// 같은 아이템이면 합치기 시도
	if (!InventorySlots[FromIndex].IsEmpty && !InventorySlots[ToIndex].IsEmpty)
	{
		if (InventorySlots[FromIndex].ItemData.ItemID == InventorySlots[ToIndex].ItemData.ItemID)
		{
			const int32 SpaceAvailable = InventorySlots[ToIndex].ItemData.MaxQuantity - InventorySlots[ToIndex].ItemData.CurrentQuantity;
			
			if (SpaceAvailable > 0)
			{
				const int32 AmountToMerge = FMath::Min(SpaceAvailable, InventorySlots[FromIndex].ItemData.CurrentQuantity);
				InventorySlots[ToIndex].ItemData.CurrentQuantity += AmountToMerge;
				InventorySlots[FromIndex].ItemData.CurrentQuantity -= AmountToMerge;

				if (InventorySlots[FromIndex].ItemData.CurrentQuantity <= 0)
				{
					InventorySlots[FromIndex].IsEmpty = true;
					InventorySlots[FromIndex].ItemData = FStruct_ItemData();
				}

				OnInventorySlotUpdated.Broadcast(FromIndex);
				OnInventorySlotUpdated.Broadcast(ToIndex);
				return;
			}
		}
	}

	// 다른 아이템이거나 합치기 불가 → 위치 교환
	FStruct_InventorySlotData TempSlot = InventorySlots[FromIndex];
	
	InventorySlots[FromIndex] = InventorySlots[ToIndex];
	InventorySlots[FromIndex].SlotIndex = FromIndex;
	
	InventorySlots[ToIndex] = TempSlot;
	InventorySlots[ToIndex].SlotIndex = ToIndex;

	OnInventorySlotUpdated.Broadcast(FromIndex);
	OnInventorySlotUpdated.Broadcast(ToIndex);
}

// ═══════════════════════════════════════════════════
// 슬롯 비우기
// ═══════════════════════════════════════════════════

void UAGSDInventoryComponent::ClearSlot(int32 SlotIndex)
{
	if (!ValidateSlotIndex(SlotIndex, TEXT("ClearSlot"))) return;

	InventorySlots[SlotIndex].IsEmpty = true;
	InventorySlots[SlotIndex].ItemData = FStruct_ItemData();
	InventorySlots[SlotIndex].SlotIndex = SlotIndex;

	OnInventorySlotUpdated.Broadcast(SlotIndex);
}

// ═══════════════════════════════════════════════════
// 빈 슬롯 탐색
// ═══════════════════════════════════════════════════

int32 UAGSDInventoryComponent::FindEmptySlot() const
{
	// 0번부터 MaxSlots-1까지 순차적으로 빈 슬롯을 탐색합니다.
	// (인덱스 구조상 핫바(0~9) 영역이 가방(10~) 영역보다 우선 탐색됩니다.)
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (InventorySlots[i].IsEmpty)
		{
			return i;
		}
	}

	return -1; // 인벤토리 가득 참
}

// ═══════════════════════════════════════════════════
// 중첩 가능 슬롯 탐색
// ═══════════════════════════════════════════════════

int32 UAGSDInventoryComponent::FindStackableSlot(const FString& ItemID) const
{
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (!InventorySlots[i].IsEmpty &&
			InventorySlots[i].ItemData.ItemID == ItemID &&
			InventorySlots[i].ItemData.CurrentQuantity < InventorySlots[i].ItemData.MaxQuantity)
		{
			return i;
		}
	}

	return -1;
}

// ═══════════════════════════════════════════════════
// 아이템 드롭
// ═══════════════════════════════════════════════════

bool UAGSDInventoryComponent::DropItem(int32 SlotIndex)
{
	if (!ValidateSlotIndex(SlotIndex, TEXT("DropItem"))) return false;
	if (InventorySlots[SlotIndex].IsEmpty) return false;

	const FStruct_ItemData& ItemData = InventorySlots[SlotIndex].ItemData;

	// ItemBPClass가 유효한 경우에만 월드에 스폰
	if (ItemData.ItemBPClass)
	{
		UWorld* World = GetWorld();
		AActor* Owner = GetOwner();
		if (World && Owner)
		{
			// 캐릭터 전방 1.5m 위치를 기준으로 설정
			const FVector StartLocation = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 150.0f;
			// 바닥 높이 검출을 위해 아래로 20m (2000 유닛) 레이캐스트
			const FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, 2000.0f);

			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Owner); // 드롭한 액터(플레이어) 무시

			FVector DropLocation = StartLocation;

			// Visibility 채널을 통해 지형 바닥 탐색
			if (World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, QueryParams))
			{
				DropLocation = HitResult.Location;
			}

			// 겹침 방지 오프셋 적용: 바닥에서 약간 띄우고(Z+30), X/Y로 미세한 랜덤 오프셋을 부여해 튕겨나감 현상을 방지
			FVector RandomOffset = FVector(FMath::RandRange(-15.0f, 15.0f), FMath::RandRange(-15.0f, 15.0f), 30.0f);
			DropLocation += RandomOffset;

			const FRotator DropRotation = FRotator::ZeroRotator;
			
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AActor* SpawnedActor = World->SpawnActor<AActor>(ItemData.ItemBPClass, DropLocation, DropRotation, SpawnParams);
			if (SpawnedActor)
			{
				if (APickUpItem* PickUpItem = Cast<APickUpItem>(SpawnedActor))
				{
					PickUpItem->SetItemID(ItemData.ItemID);
					PickUpItem->SetAmount(ItemData.CurrentQuantity);
				}
			}
		}
	}

	// 슬롯 비우기
	const FStruct_ItemData RemovedData = ItemData;
	ClearSlot(SlotIndex);
	OnItemRemoved.Broadcast(SlotIndex, RemovedData);

	return true;
}

// ═══════════════════════════════════════════════════
// 인벤토리 정렬
// ═══════════════════════════════════════════════════

void UAGSDInventoryComponent::SortInventory()
{
	// 가방 영역만 정렬 (핫바 0~9는 유지)
	// 비어 있지 않은 슬롯들을 임시 배열에 수집
	TArray<FStruct_InventorySlotData> FilledSlots;

	for (int32 i = BAG_START_INDEX; i < MaxSlots; ++i)
	{
		if (!InventorySlots[i].IsEmpty)
		{
			FilledSlots.Add(InventorySlots[i]);
		}
	}

	// 아이템 ID 기준으로 정렬
	FilledSlots.Sort([](const FStruct_InventorySlotData& A, const FStruct_InventorySlotData& B)
	{
		return A.ItemData.ItemID < B.ItemData.ItemID;
	});

	// 가방 영역 재배치
	int32 FillIndex = 0;
	for (int32 i = BAG_START_INDEX; i < MaxSlots; ++i)
	{
		if (FillIndex < FilledSlots.Num())
		{
			InventorySlots[i] = FilledSlots[FillIndex];
			InventorySlots[i].SlotIndex = i;
			++FillIndex;
		}
		else
		{
			InventorySlots[i] = FStruct_InventorySlotData();
			InventorySlots[i].SlotIndex = i;
			InventorySlots[i].IsEmpty = true;
		}
	}

	OnInventoryFullyUpdated.Broadcast();
}

// ═══════════════════════════════════════════════════
// 핫바 선택
// ═══════════════════════════════════════════════════

void UAGSDInventoryComponent::SelectHotbar(int32 NewIndex)
{
	if (NewIndex < HOTBAR_START_INDEX || NewIndex > HOTBAR_END_INDEX)
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectHotbar: 유효하지 않은 핫바 인덱스 %d"), NewIndex);
		return;
	}

	const int32 PreviousIndex = CurrentSelectedHotbar;
	CurrentSelectedHotbar = NewIndex;

	if (PreviousIndex != NewIndex)
	{
		OnHotbarSelectionChanged.Broadcast(PreviousIndex, NewIndex);
	}
}

void UAGSDInventoryComponent::CycleHotbar(bool bForward)
{
	int32 NewIndex = CurrentSelectedHotbar + (bForward ? 1 : -1);

	// 순환 로직
	if (NewIndex > HOTBAR_END_INDEX)
	{
		NewIndex = HOTBAR_START_INDEX;
	}
	else if (NewIndex < HOTBAR_START_INDEX)
	{
		NewIndex = HOTBAR_END_INDEX;
	}

	SelectHotbar(NewIndex);
}

// ═══════════════════════════════════════════════════
// Getter
// ═══════════════════════════════════════════════════

FStruct_InventorySlotData UAGSDInventoryComponent::GetSlotData(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < InventorySlots.Num())
	{
		return InventorySlots[SlotIndex];
	}

	UE_LOG(LogTemp, Warning, TEXT("GetSlotData: 유효하지 않은 슬롯 인덱스 %d"), SlotIndex);
	return FStruct_InventorySlotData();
}

FStruct_ItemData UAGSDInventoryComponent::GetCurrentHotbarItemData() const
{
	if (CurrentSelectedHotbar >= 0 && CurrentSelectedHotbar < InventorySlots.Num())
	{
		return InventorySlots[CurrentSelectedHotbar].ItemData;
	}

	return FStruct_ItemData();
}

bool UAGSDInventoryComponent::IsValidSlotIndex(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < InventorySlots.Num();
}

bool UAGSDInventoryComponent::IsHotbarSlot(int32 SlotIndex) const
{
	return SlotIndex >= HOTBAR_START_INDEX && SlotIndex <= HOTBAR_END_INDEX;
}

void UAGSDInventoryComponent::CrossInventorySwap(UAGSDInventoryComponent* SourceInv, int32 SourceIndex,
                                                  UAGSDInventoryComponent* TargetInv, int32 TargetIndex)
{
	if (!SourceInv || !TargetInv) return;
	if (!SourceInv->IsValidSlotIndex(SourceIndex) || !TargetInv->IsValidSlotIndex(TargetIndex)) return;

	FStruct_InventorySlotData& SourceSlot = SourceInv->InventorySlots[SourceIndex];
	FStruct_InventorySlotData& TargetSlot = TargetInv->InventorySlots[TargetIndex];

	// 같은 아이템이면 합치기 시도
	if (!SourceSlot.IsEmpty && !TargetSlot.IsEmpty &&
		SourceSlot.ItemData.ItemID == TargetSlot.ItemData.ItemID)
	{
		const int32 SpaceAvailable = TargetSlot.ItemData.MaxQuantity - TargetSlot.ItemData.CurrentQuantity;
		if (SpaceAvailable > 0)
		{
			const int32 AmountToMerge = FMath::Min(SpaceAvailable, SourceSlot.ItemData.CurrentQuantity);
			TargetSlot.ItemData.CurrentQuantity += AmountToMerge;
			SourceSlot.ItemData.CurrentQuantity -= AmountToMerge;

			if (SourceSlot.ItemData.CurrentQuantity <= 0)
			{
				SourceSlot.IsEmpty = true;
				SourceSlot.ItemData = FStruct_ItemData();
			}

			SourceInv->OnInventorySlotUpdated.Broadcast(SourceIndex);
			TargetInv->OnInventorySlotUpdated.Broadcast(TargetIndex);
			return;
		}
	}

	// 다른 아이템이거나 합치기 불가 → 슬롯 데이터 교환
	FStruct_InventorySlotData TempSource = SourceSlot;
	FStruct_InventorySlotData TempTarget = TargetSlot;

	SourceInv->InventorySlots[SourceIndex] = TempTarget;
	SourceInv->InventorySlots[SourceIndex].SlotIndex = SourceIndex;

	TargetInv->InventorySlots[TargetIndex] = TempSource;
	TargetInv->InventorySlots[TargetIndex].SlotIndex = TargetIndex;

	SourceInv->OnInventorySlotUpdated.Broadcast(SourceIndex);
	TargetInv->OnInventorySlotUpdated.Broadcast(TargetIndex);
}

bool UAGSDInventoryComponent::CrossInventoryTransfer(UAGSDInventoryComponent* SourceInv, int32 SourceIndex, UAGSDInventoryComponent* TargetInv)
{
	if (!SourceInv || !TargetInv) return false;
	if (!SourceInv->IsValidSlotIndex(SourceIndex)) return false;
	if (SourceInv->InventorySlots[SourceIndex].IsEmpty) return false;

	FStruct_InventorySlotData& SourceSlot = SourceInv->InventorySlots[SourceIndex];
	int32 OutRemainingQty = SourceSlot.ItemData.CurrentQuantity;
	int32 OriginalQty = OutRemainingQty;

	FStruct_ItemData ItemToAdd = SourceSlot.ItemData;
	
	// 상대방 인벤토리에 아이템 추가
	TargetInv->AddItem(ItemToAdd, OutRemainingQty);

	int32 Transferred = OriginalQty - OutRemainingQty;
	if (Transferred > 0)
	{
		SourceInv->RemoveItem(SourceIndex, Transferred);
		return true;
	}
	return false;
}

bool UAGSDInventoryComponent::MoveHotbarToBag(int32 HotbarIndex)
{
	if (!IsHotbarSlot(HotbarIndex)) return false;
	if (InventorySlots[HotbarIndex].IsEmpty) return false;

	FStruct_InventorySlotData& SourceSlot = InventorySlots[HotbarIndex];
	int32 OutRemainingQty = SourceSlot.ItemData.CurrentQuantity;
	int32 OriginalQty = OutRemainingQty;

	// 1단계: 가방 영역(10 ~ MaxSlots-1) 기존 스택 중첩
	for (int32 i = BAG_START_INDEX; i < MaxSlots && OutRemainingQty > 0; ++i)
	{
		if (!InventorySlots[i].IsEmpty &&
			InventorySlots[i].ItemData.ItemID == SourceSlot.ItemData.ItemID &&
			InventorySlots[i].ItemData.CurrentQuantity < InventorySlots[i].ItemData.MaxQuantity)
		{
			const int32 SpaceAvailable = InventorySlots[i].ItemData.MaxQuantity - InventorySlots[i].ItemData.CurrentQuantity;
			const int32 AmountToAdd = FMath::Min(SpaceAvailable, OutRemainingQty);

			InventorySlots[i].ItemData.CurrentQuantity += AmountToAdd;
			OutRemainingQty -= AmountToAdd;

			OnInventorySlotUpdated.Broadcast(i);
			OnItemAdded.Broadcast(i, InventorySlots[i].ItemData);
		}
	}

	// 2단계: 가방 영역(10 ~ MaxSlots-1) 빈 슬롯 배치
	for (int32 i = BAG_START_INDEX; i < MaxSlots && OutRemainingQty > 0; ++i)
	{
		if (InventorySlots[i].IsEmpty)
		{
			const int32 AmountToPlace = FMath::Min(SourceSlot.ItemData.MaxQuantity, OutRemainingQty);

			InventorySlots[i].IsEmpty = false;
			InventorySlots[i].ItemData = SourceSlot.ItemData;
			InventorySlots[i].ItemData.CurrentQuantity = AmountToPlace;
			OutRemainingQty -= AmountToPlace;

			OnInventorySlotUpdated.Broadcast(i);
			OnItemAdded.Broadcast(i, InventorySlots[i].ItemData);
		}
	}

	int32 Transferred = OriginalQty - OutRemainingQty;
	if (Transferred > 0)
	{
		RemoveItem(HotbarIndex, Transferred);
		return true;
	}

	return false;
}

bool UAGSDInventoryComponent::MoveBagToHotbar(int32 BagIndex)
{
	if (IsHotbarSlot(BagIndex)) return false;
	if (InventorySlots[BagIndex].IsEmpty) return false;

	FStruct_InventorySlotData& SourceSlot = InventorySlots[BagIndex];
	int32 OutRemainingQty = SourceSlot.ItemData.CurrentQuantity;
	int32 OriginalQty = OutRemainingQty;

	// 1단계: 핫바 영역(0 ~ 9) 기존 스택 중첩
	for (int32 i = HOTBAR_START_INDEX; i <= HOTBAR_END_INDEX && OutRemainingQty > 0; ++i)
	{
		if (!InventorySlots[i].IsEmpty &&
			InventorySlots[i].ItemData.ItemID == SourceSlot.ItemData.ItemID &&
			InventorySlots[i].ItemData.CurrentQuantity < InventorySlots[i].ItemData.MaxQuantity)
		{
			const int32 SpaceAvailable = InventorySlots[i].ItemData.MaxQuantity - InventorySlots[i].ItemData.CurrentQuantity;
			const int32 AmountToAdd = FMath::Min(SpaceAvailable, OutRemainingQty);

			InventorySlots[i].ItemData.CurrentQuantity += AmountToAdd;
			OutRemainingQty -= AmountToAdd;

			OnInventorySlotUpdated.Broadcast(i);
			OnItemAdded.Broadcast(i, InventorySlots[i].ItemData);
		}
	}

	// 2단계: 핫바 영역(0 ~ 9) 빈 슬롯 배치
	for (int32 i = HOTBAR_START_INDEX; i <= HOTBAR_END_INDEX && OutRemainingQty > 0; ++i)
	{
		if (InventorySlots[i].IsEmpty)
		{
			const int32 AmountToPlace = FMath::Min(SourceSlot.ItemData.MaxQuantity, OutRemainingQty);

			InventorySlots[i].IsEmpty = false;
			InventorySlots[i].ItemData = SourceSlot.ItemData;
			InventorySlots[i].ItemData.CurrentQuantity = AmountToPlace;
			OutRemainingQty -= AmountToPlace;

			OnInventorySlotUpdated.Broadcast(i);
			OnItemAdded.Broadcast(i, InventorySlots[i].ItemData);
		}
	}

	int32 Transferred = OriginalQty - OutRemainingQty;
	if (Transferred > 0)
	{
		RemoveItem(BagIndex, Transferred);
		return true;
	}

	return false;
}

// ═══════════════════════════════════════════════════
// GameInstance 연동 (영속성)
// ═══════════════════════════════════════════════════

void UAGSDInventoryComponent::SaveToGameInstance()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->IsA(AAGSDCharacter::StaticClass()))
	{
		return;
	}

	USOVGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// 전체 인벤토리 slots 데이터를 통합 백업 (0~MaxSlots-1)
	GI->TempInventory = InventorySlots;

	// 핫바 선택 인덱스 백업
	GI->CurrentSelectedHotbar = CurrentSelectedHotbar;
}

void UAGSDInventoryComponent::LoadFromGameInstance()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->IsA(AAGSDCharacter::StaticClass()))
	{
		return;
	}

	USOVGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// 통합 백업 데이터가 존재하는 경우 복원
	if (GI->TempInventory.Num() > 0)
	{
		InventorySlots = GI->TempInventory;

		// 크기가 맞지 않는 경우 안전하게 예외처리 및 패딩
		if (InventorySlots.Num() != MaxSlots)
		{
			InventorySlots.SetNum(MaxSlots);
			for (int32 i = 0; i < MaxSlots; ++i)
			{
				if (InventorySlots[i].SlotIndex != i)
				{
					InventorySlots[i] = FStruct_InventorySlotData();
					InventorySlots[i].SlotIndex = i;
					InventorySlots[i].IsEmpty = true;
				}
			}
		}
		else
		{
			// 슬롯 인덱스 강제 동기화
			for (int32 i = 0; i < MaxSlots; ++i)
			{
				InventorySlots[i].SlotIndex = i;
			}
		}
	}

	CurrentSelectedHotbar = FMath::Clamp(GI->CurrentSelectedHotbar, HOTBAR_START_INDEX, HOTBAR_END_INDEX);
}

// ═══════════════════════════════════════════════════
// 내부 헬퍼
// ═══════════════════════════════════════════════════

USOVGameInstance* UAGSDInventoryComponent::GetGameInstance()
{
	if (!CachedGameInstance)
	{
		CachedGameInstance = Cast<USOVGameInstance>(UGameplayStatics::GetGameInstance(this));
	}
	return CachedGameInstance;
}

bool UAGSDInventoryComponent::ValidateSlotIndex(int32 SlotIndex, const FString& CallerName) const
{
	if (SlotIndex < 0 || SlotIndex >= InventorySlots.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 유효하지 않은 슬롯 인덱스 %d (범위: 0~%d)"), *CallerName, SlotIndex, InventorySlots.Num() - 1);
		return false;
	}
	return true;
}

void UAGSDInventoryComponent::SetAllSlots(const TArray<FStruct_InventorySlotData>& NewSlots)
{
	InventorySlots = NewSlots;
	// 전체 갱신 델리게이트 알림
	if (OnInventoryFullyUpdated.IsBound())
	{
		OnInventoryFullyUpdated.Broadcast();
	}
}
