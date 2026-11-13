#include "Inventory/UI/AGSDItemTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UAGSDItemTooltipWidget::SetTooltipData(const FStruct_ItemData& InItemData)
{
	// 1. 아이템 이름 설정
	if (TXT_ItemName)
	{
		TXT_ItemName->SetText(InItemData.ItemName);
	}

	// 2. 아이템 설명 설정
	if (TXT_ItemDescription)
	{
		FString CleanedDesc = InItemData.ItemDescription.ToString();
		CleanedDesc = CleanedDesc.Replace(TEXT("\\n"), TEXT("\n"));
		TXT_ItemDescription->SetText(FText::FromString(CleanedDesc));
	}

	// 3. 아이템 분류 타입 한국어 변환 및 설정
	if (TXT_ItemType)
	{
		FText TypeText = FText::FromString(TEXT("일반"));
		switch (InItemData.ItemType)
		{
		case EItemType::EIT_Equipment:
			TypeText = FText::FromString(TEXT("장비"));
			break;
		case EItemType::EIT_Consumable:
			TypeText = FText::FromString(TEXT("소모품"));
			break;
		case EItemType::EIT_Material:
			TypeText = FText::FromString(TEXT("재료"));
			break;
		case EItemType::EIT_Quest:
			TypeText = FText::FromString(TEXT("퀘스트"));
			break;
		default:
			break;
		}
		TXT_ItemType->SetText(TypeText);
	}

	// 4. 아이템 아이콘 이미지 설정
	if (IMG_ItemIcon)
	{
		if (InItemData.ItemIcon)
		{
			IMG_ItemIcon->SetBrushFromTexture(InItemData.ItemIcon);
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
