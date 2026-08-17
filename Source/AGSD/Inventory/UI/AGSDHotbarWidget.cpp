// AGSDHotbarWidget.cpp - 핫바 컨테이너 위젯 구현

#include "AGSDHotbarWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "AGSDHotbarSlotWidget.h"

void UAGSDHotbarWidget::InitializeHotbar(UAGSDInventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent) return;

	InventoryComponent = InInventoryComponent;

	// 델리게이트 바인딩 (중복 등록 방지를 위해 먼저 해제)
	InventoryComponent->OnHotbarSelectionChanged.RemoveAll(this);
	InventoryComponent->OnHotbarSelectionChanged.AddDynamic(this, &UAGSDHotbarWidget::OnHotbarSelected);

	InventoryComponent->OnInventorySlotUpdated.RemoveAll(this);
	InventoryComponent->OnInventorySlotUpdated.AddDynamic(this, &UAGSDHotbarWidget::OnInventorySlotUpdated);

	// 핫바 UI 요소 정리
	if (HB_Slotbar)
	{
		HB_Slotbar->ClearChildren();
	}
	HotbarSlots.Empty();

	// 10개의 핫바 슬롯 생성 및 배치
	if (HotbarSlotClass)
	{
		for (int32 i = 0; i < UAGSDInventoryComponent::HOTBAR_SLOT_COUNT; ++i)
		{
			UAGSDHotbarSlotWidget* NewSlot = CreateWidget<UAGSDHotbarSlotWidget>(this, HotbarSlotClass);
			if (NewSlot)
			{
				NewSlot->SlotIndex = i;
				NewSlot->OwningInventoryComponent = InventoryComponent;
				
				// 백엔드 데이터에서 데이터 가져와 설정
				FStruct_InventorySlotData SlotData = InventoryComponent->GetSlotData(i);
				NewSlot->SetHotbarItem(SlotData.ItemData, SlotData.IsEmpty);

				HB_Slotbar->AddChildToHorizontalBox(NewSlot);
				HotbarSlots.Add(NewSlot);
			}
		}
	}

	// 초기 활성화 핫바 슬롯 동기화
	const int32 CurrentActiveIndex = InventoryComponent->GetCurrentHotbarIndex();
	OnHotbarSelected(-1, CurrentActiveIndex);
}

void UAGSDHotbarWidget::OnHotbarSelected(int32 PreviousIndex, int32 NewIndex)
{
	// 이전 활성화 슬롯 선택 해제
	if (HotbarSlots.IsValidIndex(PreviousIndex))
	{
		HotbarSlots[PreviousIndex]->ToggleSelectedHotbar(false);
	}

	// 새 활성화 슬롯 선택
	if (HotbarSlots.IsValidIndex(NewIndex))
	{
		UAGSDHotbarSlotWidget* NewActiveSlot = HotbarSlots[NewIndex];
		NewActiveSlot->ToggleSelectedHotbar(true);
		
		// 핫바 상태 바 갱신
		SetHotbarStatus(NewActiveSlot);
	}
}

void UAGSDHotbarWidget::OnInventorySlotUpdated(int32 SlotIndex)
{
	// 핫바 영역(0~9)의 슬롯 데이터가 업데이트되었을 때만 처리
	if (InventoryComponent->IsHotbarSlot(SlotIndex) && HotbarSlots.IsValidIndex(SlotIndex))
	{
		FStruct_InventorySlotData SlotData = InventoryComponent->GetSlotData(SlotIndex);
		HotbarSlots[SlotIndex]->SetHotbarItem(SlotData.ItemData, SlotData.IsEmpty);

		// 현재 활성화된 슬롯의 데이터가 변경된 것이라면 상태 정보도 업데이트
		if (SlotIndex == InventoryComponent->GetCurrentHotbarIndex())
		{
			SetHotbarStatus(HotbarSlots[SlotIndex]);
		}
	}
}

void UAGSDHotbarWidget::SetHotbarStatus(UAGSDHotbarSlotWidget* SelectedSlot)
{
	if (!SelectedSlot) return;

	FStruct_InventorySlotData SlotData = SelectedSlot->GetHotbarItem();

	if (SlotData.IsEmpty || SlotData.ItemData.ItemID.IsEmpty())
	{
		// 비어있으면 가시성 제거
		if (SB_HotbarStatus)
		{
			SB_HotbarStatus->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		// 아이템이 존재하면 가시성 노출 및 이름 설정
		if (SB_HotbarStatus)
		{
			SB_HotbarStatus->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		if (TXT_HotbarStatus)
		{
			TXT_HotbarStatus->SetText(SlotData.ItemData.ItemName);
		}
	}
}
