// Fill out your copyright notice in the Description page of Project Settings.

#include "AGSDItemLibrary.h"
#include "Engine/DataTable.h"

FStruct_ItemData UAGSDItemLibrary::GetItemDataByID(const UDataTable* ItemDataTable, const FString& ItemID, bool& bOutSuccess)
{
	bOutSuccess = false;

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAGSDItemLibrary::GetItemDataByID - ItemDataTable is null."));
		return FStruct_ItemData();
	}

	if (ItemID.IsEmpty())
	{
		return FStruct_ItemData();
	}

	FStruct_ItemData* RowData = ItemDataTable->FindRow<FStruct_ItemData>(FName(*ItemID), TEXT("GetItemDataByID"));
	if (!RowData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAGSDItemLibrary::GetItemDataByID - Failed to find Item ID [%s] in DataTable."), *ItemID);
		return FStruct_ItemData();
	}

	bOutSuccess = true;
	return *RowData;
}

FStruct_InventorySlotData UAGSDItemLibrary::CreateSlotDataByID(const UDataTable* ItemDataTable, const FString& ItemID, int32 Amount, int32 SlotIndex, bool& bOutSuccess)
{
	bOutSuccess = false;

	bool bFound = false;
	FStruct_ItemData FoundItemData = GetItemDataByID(ItemDataTable, ItemID, bFound);

	if (!bFound)
	{
		return FStruct_InventorySlotData();
	}

	FStruct_InventorySlotData NewSlot;
	NewSlot.IsEmpty = false;
	NewSlot.ItemData = FoundItemData;
	NewSlot.ItemData.CurrentQuantity = Amount;
	NewSlot.SlotIndex = SlotIndex;

	bOutSuccess = true;
	return NewSlot;
}

TArray<FStruct_InventorySlotData> UAGSDItemLibrary::CreateEmptySlots(int32 SlotCount)
{
	TArray<FStruct_InventorySlotData> Slots;
	
	if (SlotCount <= 0)
	{
		return Slots;
	}

	Slots.SetNum(SlotCount);
	for (int32 i = 0; i < SlotCount; ++i)
	{
		Slots[i] = FStruct_InventorySlotData();
		Slots[i].SlotIndex = i;
		Slots[i].IsEmpty = true;
	}

	return Slots;
}
