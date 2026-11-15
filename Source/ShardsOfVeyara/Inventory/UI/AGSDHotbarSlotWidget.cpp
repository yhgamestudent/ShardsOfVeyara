// AGSDHotbarSlotWidget.cpp - 핫바 개별 슬롯 위젯 구현

#include "AGSDHotbarSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "SOVGameInstance.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "AGSDSlotDragDropOperation.h"
#include "Engine/World.h"

void UAGSDHotbarSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 단축키 표시 설정 (1~9, 0)
	if (TXT_SlotIndex)
	{
		if (SlotIndex == 9)
		{
			TXT_SlotIndex->SetText(FText::FromString(TEXT("0")));
		}
		else
		{
			TXT_SlotIndex->SetText(FText::AsNumber(SlotIndex + 1));
		}
	}

	// 드래그 플래그 초기화
	bIsSlotDragging = false;
	bDragLeavingFromHotbar = false;
	bDragLeaving = false;
	bDragToHotbar = false;
	SelectedHotbar = nullptr;

	// 하이라이트 테두리 초기화
	ToggleSelectedHotbar(false);
	if (IMG_HotbarSelectedYellow)
	{
		IMG_HotbarSelectedYellow->SetVisibility(ESlateVisibility::Collapsed);
	}
}


void UAGSDHotbarSlotWidget::ToggleSelectedHotbar(bool bSelected)
{
	if (IMG_HotbarSelectedWhite)
	{
		IMG_HotbarSelectedWhite->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UAGSDHotbarSlotWidget::NativeOnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDragEnter(MyGeometry, DragDropEvent, Operation);

	if (Cast<UAGSDSlotDragDropOperation>(Operation))
	{
		bDragToHotbar = true;
		SelectedHotbar = this;

		if (IMG_HotbarSelectedYellow)
		{
			IMG_HotbarSelectedYellow->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void UAGSDHotbarSlotWidget::NativeOnDragLeave(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDragLeave(DragDropEvent, Operation);

	bDragToHotbar = false;
	SelectedHotbar = nullptr;

	if (IMG_HotbarSelectedYellow)
	{
		IMG_HotbarSelectedYellow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAGSDHotbarSlotWidget::NativeOnDragCancelled(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDragCancelled(DragDropEvent, Operation);

	bIsSlotDragging = false;

	if (UAGSDSlotDragDropOperation* SlotOp = Cast<UAGSDSlotDragDropOperation>(Operation))
	{
		// 조건 B (핫바 타겟 취소 시) 복원
		if (bDragToHotbar && SelectedHotbar)
		{
			if (SelectedHotbar->GetItemData().IsEmpty)
			{
				SelectedHotbar->SetItemData(SlotOp->SourceSlotData.ItemData, false);
			}
		}
	}

	// 취소 시에도 노란 테두리 제거
	if (IMG_HotbarSelectedYellow)
	{
		IMG_HotbarSelectedYellow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UAGSDHotbarSlotWidget::NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	bool bSuccess = Super::NativeOnDrop(MyGeometry, DragDropEvent, Operation);

	// 드롭 성공 시에도 노란 테두리 제거
	if (IMG_HotbarSelectedYellow)
	{
		IMG_HotbarSelectedYellow->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 상태 플래그 초기화
	bDragToHotbar = false;
	SelectedHotbar = nullptr;

	return bSuccess;
}
