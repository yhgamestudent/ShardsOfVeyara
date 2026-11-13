// AGSDDragVisualWidget.cpp - 드래그 비주얼 위젯 C++ 구현 (WBP_SlotDragnDrop 이관)

#include "AGSDDragVisualWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UAGSDDragVisualWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshVisual();
}

void UAGSDDragVisualWidget::SetDragItemData(const FStruct_ItemData& InItemData)
{
	ItemData = InItemData;
	RefreshVisual();
}

void UAGSDDragVisualWidget::RefreshVisual()
{
	// ── 아이콘 설정 ──
	if (IMG_ItemIcon)
	{
		if (ItemData.ItemIcon)
		{
			IMG_ItemIcon->SetBrushFromTexture(ItemData.ItemIcon);
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ── 수량 텍스트 설정 ──
	// MaxQuantity가 1이면 단일 아이템이므로 수량 텍스트를 숨기고,
	// 그 외에는 CurrentQuantity를 표시합니다. (블루프린트 로직 동일하게 이관)
	if (TXT_ItemAmount)
	{
		if (ItemData.MaxQuantity <= 1)
		{
			TXT_ItemAmount->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TXT_ItemAmount->SetText(FText::AsNumber(ItemData.CurrentQuantity));
			TXT_ItemAmount->SetVisibility(ESlateVisibility::Visible);
		}
	}
}
