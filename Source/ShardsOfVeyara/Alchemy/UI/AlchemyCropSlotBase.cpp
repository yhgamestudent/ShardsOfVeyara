// Fill out your copyright notice in the Description page of Project Settings.


#include "AlchemyCropSlotBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/UI/AGSDDragVisualWidget.h"

void UAlchemyCropSlotBase::NativeConstruct()
{
	Super::NativeConstruct();

	UTexture2D* Texture = SlotItemData.ItemData.ItemIcon;
	int32 Amount = SlotItemData.ItemData.CurrentQuantity;

	if (IMG_ItemIcon)
	{
		if (Texture)
		{
			IMG_ItemIcon->SetBrushFromTexture(Texture);
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// 텍스처가 없는 빈 슬롯 상태인 경우 (양피지 배경만 노출되도록 아이콘만 가림)
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (TXT_ItemAmount)
	{
		TXT_ItemAmount->SetText(FText::AsNumber(Amount));
	}

	// 렌더링 시점에 드래그 불가 및 가마솥 투입구 분기 처리를 일괄 적용합니다.
	InitSlot(bCantDrag, bIsInsertedSlot);
}

FString UAlchemyCropSlotBase::GetItemID() const
{
	return SlotItemData.ItemData.ItemID;
}

int32 UAlchemyCropSlotBase::GetAmount() const
{
	return SlotItemData.ItemData.CurrentQuantity;
}

UTexture2D* UAlchemyCropSlotBase::GetTexture() const
{
	return SlotItemData.ItemData.ItemIcon;
}

void UAlchemyCropSlotBase::InitSlot(bool bInCantDrag, bool bInIsInsertedSlot)
{
	bCantDrag = bInCantDrag;
	bIsInsertedSlot = bInIsInsertedSlot;
	
	if (bIsInsertedSlot)
	{
		// 가마솥에 투입된 슬롯인 경우:
		// 드래그가 불가능하게 막되, 이미지는 흐리지 않고 또렷하게(1.0) 노출시킵니다.
		bCantDrag = true;
		if (IMG_ItemIcon)
		{
			IMG_ItemIcon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
		}

		// 투입구 슬롯은 재료 수량 숫자를 출력하지 않습니다.
		if (TXT_ItemAmount)
		{
			TXT_ItemAmount->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		// 캐릭터 인벤토리 영역인 경우:
		if (TXT_ItemAmount)
		{
			TXT_ItemAmount->SetVisibility(ESlateVisibility::Visible);
		}

		if (IMG_ItemIcon)
		{
			if (bCantDrag)
			{
				// 재료가 없어 비활성화된 상태: 흐릿하게(0.35)
				IMG_ItemIcon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.35f));
			}
			else
			{
				// 보유 중인 활성화 상태: 또렷하게(1.0)
				IMG_ItemIcon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
			}
		}
	}
}

void UAlchemyCropSlotBase::UpdateSlot(UTexture2D* InTexture, int32 InAmount)
{
	SlotItemData.ItemData.ItemIcon = InTexture;
	SlotItemData.ItemData.CurrentQuantity = InAmount;
	SlotItemData.IsEmpty = (InTexture == nullptr);

	if (IMG_ItemIcon)
	{
		if (InTexture)
		{
			IMG_ItemIcon->SetBrushFromTexture(InTexture);
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (TXT_ItemAmount)
	{
		TXT_ItemAmount->SetText(FText::AsNumber(InAmount));
	}
}

FReply UAlchemyCropSlotBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bCantDrag)
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 왼쪽 클릭이고 드래그가 가능한 상태라면 드래그 탐지 요청
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return FReply::Unhandled();
}

void UAlchemyCropSlotBase::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (bCantDrag) return;

	UDragDropOperation* DragOp = NewObject<UDragDropOperation>(this);
	if (!DragOp) return;

	// Payload로 자기 자신을 설정 (블루프린트 로직과 호환)
	DragOp->Payload = this;
	DragOp->Pivot = EDragPivot::MouseDown;

	// 드래그 비주얼 생성
	if (DragVisualClass)
	{
		UAGSDDragVisualWidget* DragVisual = CreateWidget<UAGSDDragVisualWidget>(GetWorld(), DragVisualClass);
		if (DragVisual)
		{
			// 부모의 데이터 모델에서 데이터 세팅
			DragVisual->SetDragItemData(SlotItemData.ItemData);
			DragOp->DefaultDragVisual = DragVisual;
		}
	}

	OutOperation = DragOp;
}
