// Fill out your copyright notice in the Description page of Project Settings.


#include "AlchemyInventoryUI.h"
#include "Components/UniformGridPanel.h"
#include "Engine/DataTable.h"
#include "Struct_ItemData.h"
#include "AlchemyCropSlotBase.h"
#include "AGSDCharacter.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Alchemy/Struct_SlotAddress.h"

void UAlchemyInventoryUI::InitializeSlots(const TMap<FString, FStruct_MaterialAddress>& MaterialAddress, EHoldingState Category, bool bInsertedSlot)
{
	if (!UGP_InventorySlots) return;
	if (!CropSlotClass) return;

	UGP_InventorySlots->ClearChildren();
	Slots.Empty();

	int32 Column = 0;

	// 1. 투입된 슬롯(bInsertedSlot = true)인 경우: 기존처럼 투입된 아이템만 표시
	if (bInsertedSlot)
	{
		for (const TPair<FString, FStruct_MaterialAddress>& Pair : MaterialAddress)
		{
			const FString& ItemID = Pair.Key;
			const FStruct_MaterialAddress& MatAddress = Pair.Value;

			// 데이터 테이블에서 아이템 정보 조회
			FStruct_ItemData* ItemData = nullptr;
			if (ItemDataTable)
			{
				ItemData = ItemDataTable->FindRow<FStruct_ItemData>(FName(*ItemID), TEXT("InitializeSlots"));
			}

			if (ItemData)
			{
				// EHoldingState 카테고리 필터링 비교 (하이브리드 호환)
				if (Category == EHoldingState::EHS_None || 
					ItemData->EquipHoldingState == Category || 
					ItemData->ItemCategory == Category)
				{
					// 해당 재료의 총 수량 누적 합산 (모든 슬롯의 수량 합산)
					int32 TotalAmount = 0;
					for (const FStruct_SlotAddress& SlotAddr : MatAddress.Address)
					{
						TotalAmount += SlotAddr.Amount;
					}

					// 실제로 해당 영역에 존재하는 아이템인 경우에만 생성
					if (TotalAmount > 0)
					{
						// 슬롯 위젯 동적 생성
						APlayerController* PC = GetOwningPlayer();
						UAlchemyCropSlotBase* NewSlot = PC ? CreateWidget<UAlchemyCropSlotBase>(PC, CropSlotClass) : nullptr;
						if (NewSlot)
						{
							FStruct_ItemData TempItemData = *ItemData;
							TempItemData.CurrentQuantity = TotalAmount;
							NewSlot->SetItemData(TempItemData, false);

							NewSlot->AlchemyInventoryUI = this;
							NewSlot->InitSlot(true, true); // 드래그 불가 및 가마솥 투입 슬롯 활성화

							// 가로 UniformGridPanel에 슬롯 추가 (Row = 0, Column)
							UGP_InventorySlots->AddChildToUniformGrid(NewSlot, 0, Column);
							Slots.Add(NewSlot);
							Column++;
						}
					}
				}
			}
		}

		// 만약 투입된 아이템이 없어 슬롯이 전혀 생성되지 않았다면,
		// 양피지 배경만 보이는 빈 투입구 슬롯을 강제로 1개 생성하여 노출시킵니다.
		if (Slots.Num() == 0)
		{
			APlayerController* PC = GetOwningPlayer();
			UAlchemyCropSlotBase* NewSlot = PC ? CreateWidget<UAlchemyCropSlotBase>(PC, CropSlotClass) : nullptr;
			if (NewSlot)
			{
				NewSlot->SetItemData(FStruct_ItemData(), true); // 빈 슬롯 상태 설정

				NewSlot->AlchemyInventoryUI = this;
				NewSlot->InitSlot(true, true); // 드래그 불가 및 가마솥 투입 슬롯 활성화

				UGP_InventorySlots->AddChildToUniformGrid(NewSlot, 0, 0);
				Slots.Add(NewSlot);
			}
		}
	}
	// 2. 인벤토리 영역(bInsertedSlot = false)인 경우: 테이블의 모든 관련 작물을 나열하되, 수량이 0인 것은 흐리게 비활성화
	else
	{
		if (ItemDataTable)
		{
			TArray<FStruct_ItemData*> AllRows;
			ItemDataTable->GetAllRows<FStruct_ItemData>(TEXT("InitializeSlots"), AllRows);

			for (FStruct_ItemData* ItemData : AllRows)
			{
				if (ItemData)
				{
					// EHoldingState 카테고리 필터링 비교 (하이브리드 호환)
					if (Category == EHoldingState::EHS_None || 
						ItemData->EquipHoldingState == Category || 
						ItemData->ItemCategory == Category)
					{
						FString ItemID = ItemData->ItemID;
						int32 TotalAmount = 0;
						bool bHasItem = false;

						// 인벤토리에 이 아이템이 존재하는지 확인 및 수량 합산
						const FStruct_MaterialAddress* FoundAddress = MaterialAddress.Find(ItemID);
						if (FoundAddress)
						{
							for (const FStruct_SlotAddress& SlotAddr : FoundAddress->Address)
							{
								TotalAmount += SlotAddr.Amount;
							}
							if (TotalAmount > 0)
							{
								bHasItem = true;
							}
						}

						// 슬롯 위젯 동적 생성 (수량이 0이어도 무조건 생성)
						APlayerController* PC = GetOwningPlayer();
						UAlchemyCropSlotBase* NewSlot = PC ? CreateWidget<UAlchemyCropSlotBase>(PC, CropSlotClass) : nullptr;
						if (NewSlot)
						{
							FStruct_ItemData TempItemData = *ItemData;
							TempItemData.CurrentQuantity = TotalAmount;
							NewSlot->SetItemData(TempItemData, false);

							NewSlot->AlchemyInventoryUI = this;
							
							// 인벤토리에 실질적으로 수량이 있을 때만 드래그 가능, 없으면 드래그 불가(흐려짐) 처리
							NewSlot->InitSlot(!bHasItem);

							// 가로 UniformGridPanel에 슬롯 추가 (Row = 0, Column)
							UGP_InventorySlots->AddChildToUniformGrid(NewSlot, 0, Column);
							Slots.Add(NewSlot);
							Column++;
						}
					}
				}
			}
		}
	}
}

TMap<FString, FStruct_MaterialAddress> UAlchemyInventoryUI::ScanInventoryForMaterials(AAGSDCharacter* Character)
{
	TMap<FString, FStruct_MaterialAddress> TempMaterialAddress;
	if (!Character || !Character->InventoryComponent) return TempMaterialAddress;

	const TArray<FStruct_InventorySlotData>& AllSlots = Character->InventoryComponent->GetAllSlots();

	for (int32 Index = 0; Index < AllSlots.Num(); ++Index)
	{
		const FStruct_InventorySlotData& SlotData = AllSlots[Index];
		
		if (!SlotData.IsEmpty && !SlotData.ItemData.ItemID.IsEmpty() && SlotData.ItemData.CurrentQuantity > 0)
		{
			FString ItemID = SlotData.ItemData.ItemID;

			FStruct_SlotAddress SlotAddr;
			SlotAddr.Index = Index;
			SlotAddr.Amount = SlotData.ItemData.CurrentQuantity;

			TempMaterialAddress.FindOrAdd(ItemID).Address.Add(SlotAddr);
		}
	}

	return TempMaterialAddress;
}
