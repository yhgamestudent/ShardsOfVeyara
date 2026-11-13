// Fill out your copyright notice in the Description page of Project Settings.

#include "Alchemy/UI/AlchemyRecipeEntryWidget.h"
#include "Alchemy/UI/AlchemyUI.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Struct_ItemData.h"
#include "Engine/DataTable.h"

void UAlchemyRecipeEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BTN_Craft)
	{
		BTN_Craft->OnClicked.RemoveDynamic(this, &UAlchemyRecipeEntryWidget::OnCraftButtonClicked);
		BTN_Craft->OnClicked.AddDynamic(this, &UAlchemyRecipeEntryWidget::OnCraftButtonClicked);
	}
}

void UAlchemyRecipeEntryWidget::SetupEntry(const FPotionData& InRecipeData, bool bInDiscovered, bool bInCanCraft, int32 InCountA, int32 InCountB, int32 InCrystalCount)
{
	RecipeData = InRecipeData;
	bIsDiscovered = bInDiscovered;
	bCanCraft = bInCanCraft;

	UAlchemyUI* ParentUI = GetParentAlchemyUI();
	UDataTable* ItemDataTable = ParentUI ? ParentUI->GetItemDataTable() : nullptr;

	// 1. 완성 포션 정보 표시
	if (bIsDiscovered)
	{
		if (ItemDataTable)
		{
			FStruct_ItemData* PotionItemData = ItemDataTable->FindRow<FStruct_ItemData>(FName(*RecipeData.ItemID), TEXT("SetupRecipeEntry_Potion"));
			if (PotionItemData)
			{
				if (IMG_PotionIcon)
				{
					if (PotionItemData->ItemIcon)
					{
						IMG_PotionIcon->SetBrushFromTexture(PotionItemData->ItemIcon);
						IMG_PotionIcon->SetColorAndOpacity(FLinearColor::White);
					}
				}
				if (TXT_PotionName)
				{
					TXT_PotionName->SetText(PotionItemData->ItemName);
				}
				if (TXT_PotionDescription)
				{
					TXT_PotionDescription->SetText(PotionItemData->ItemDescription);
				}
			}
		}

		// 재료 A 정보
		if (ItemDataTable)
		{
			FStruct_ItemData* ItemA = ItemDataTable->FindRow<FStruct_ItemData>(FName(*RecipeData.IngredientA), TEXT("SetupRecipeEntry_IngredientA"));
			if (ItemA && IMG_IngredientA && ItemA->ItemIcon)
			{
				IMG_IngredientA->SetBrushFromTexture(ItemA->ItemIcon);
				IMG_IngredientA->SetColorAndOpacity(FLinearColor::White);
			}
		}
		if (TXT_IngredientACount)
		{
			// "1 / 보유수량" 형식 표시
			FString CountStr = FString::Printf(TEXT("1 / %d"), InCountA);
			TXT_IngredientACount->SetText(FText::FromString(CountStr));
			TXT_IngredientACount->SetColorAndOpacity(InCountA >= 1 ? NormalTextColor : InsufficientTextColor);
		}

		// 재료 B 정보
		if (ItemDataTable)
		{
			FStruct_ItemData* ItemB = ItemDataTable->FindRow<FStruct_ItemData>(FName(*RecipeData.IngredientB), TEXT("SetupRecipeEntry_IngredientB"));
			if (ItemB && IMG_IngredientB && ItemB->ItemIcon)
			{
				IMG_IngredientB->SetBrushFromTexture(ItemB->ItemIcon);
				IMG_IngredientB->SetColorAndOpacity(FLinearColor::White);
			}
		}
		if (TXT_IngredientBCount)
		{
			FString CountStr = FString::Printf(TEXT("1 / %d"), InCountB);
			TXT_IngredientBCount->SetText(FText::FromString(CountStr));
			TXT_IngredientBCount->SetColorAndOpacity(InCountB >= 1 ? NormalTextColor : InsufficientTextColor);
		}

		// 차원 결정 정보
		if (ItemDataTable)
		{
			FStruct_ItemData* Crystal = ItemDataTable->FindRow<FStruct_ItemData>(FName(TEXT("DimensionCrystal")), TEXT("SetupRecipeEntry_Crystal"));
			if (Crystal && IMG_Crystal && Crystal->ItemIcon)
			{
				IMG_Crystal->SetBrushFromTexture(Crystal->ItemIcon);
				IMG_Crystal->SetColorAndOpacity(FLinearColor::White);
			}
		}
		if (TXT_CrystalCount)
		{
			FString CountStr = FString::Printf(TEXT("1 / %d"), InCrystalCount);
			TXT_CrystalCount->SetText(FText::FromString(CountStr));
			TXT_CrystalCount->SetColorAndOpacity(InCrystalCount >= 1 ? NormalTextColor : InsufficientTextColor);
		}

		// 제작 버튼 상태 설정
		if (BTN_Craft)
		{
			BTN_Craft->SetIsEnabled(bCanCraft);
		}
		if (TXT_CraftButtonLabel)
		{
			TXT_CraftButtonLabel->SetText(FText::FromString(bCanCraft ? TEXT("제작하기") : TEXT("재료부족")));
		}
	}
	else
	{
		// ── 미발견 레시피인 경우 마스킹 처리 ──

		// 포션 아이콘: 대체 텍스처가 있으면 적용, 없으면 어두운 실루엣 처리
		if (IMG_PotionIcon)
		{
			if (UnknownPotionTexture)
			{
				IMG_PotionIcon->SetBrushFromTexture(UnknownPotionTexture);
				IMG_PotionIcon->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				IMG_PotionIcon->SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f));
			}
		}

		// 재료 A, B 아이콘: 대체 텍스처가 있으면 적용, 없으면 어두운 실루엣 처리
		if (IMG_IngredientA)
		{
			if (UnknownIngredientTexture)
			{
				IMG_IngredientA->SetBrushFromTexture(UnknownIngredientTexture);
				IMG_IngredientA->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				IMG_IngredientA->SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f, 0.4f));
			}
		}

		if (IMG_IngredientB)
		{
			if (UnknownIngredientTexture)
			{
				IMG_IngredientB->SetBrushFromTexture(UnknownIngredientTexture);
				IMG_IngredientB->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				IMG_IngredientB->SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f, 0.4f));
			}
		}

		// 차원 결정은 공통 촉매이므로 아이콘 유지
		if (ItemDataTable && IMG_Crystal)
		{
			FStruct_ItemData* Crystal = ItemDataTable->FindRow<FStruct_ItemData>(FName(TEXT("DimensionCrystal")), TEXT("SetupRecipeEntry_Crystal"));
			if (Crystal && Crystal->ItemIcon)
			{
				IMG_Crystal->SetBrushFromTexture(Crystal->ItemIcon);
				IMG_Crystal->SetColorAndOpacity(FLinearColor::White);
			}
		}

		if (TXT_PotionName)
		{
			TXT_PotionName->SetText(FText::FromString(TEXT("???")));
		}
		if (TXT_PotionDescription)
		{
			TXT_PotionDescription->SetText(FText::FromString(TEXT("아직 제조해보지 않은 미지의 물약입니다.")));
		}
		if (TXT_IngredientACount)
		{
			TXT_IngredientACount->SetText(FText::FromString(TEXT("?")));
			TXT_IngredientACount->SetColorAndOpacity(NormalTextColor);
		}
		if (TXT_IngredientBCount)
		{
			TXT_IngredientBCount->SetText(FText::FromString(TEXT("?")));
			TXT_IngredientBCount->SetColorAndOpacity(NormalTextColor);
		}
		if (TXT_CrystalCount)
		{
			TXT_CrystalCount->SetText(FText::FromString(TEXT("?")));
			TXT_CrystalCount->SetColorAndOpacity(NormalTextColor);
		}
		if (BTN_Craft)
		{
			BTN_Craft->SetIsEnabled(false);
		}
		if (TXT_CraftButtonLabel)
		{
			TXT_CraftButtonLabel->SetText(FText::FromString(TEXT("미발견")));
		}
	}
}

void UAlchemyRecipeEntryWidget::OnCraftButtonClicked()
{
	if (!bCanCraft || !bIsDiscovered) return;

	if (UAlchemyUI* ParentUI = GetParentAlchemyUI())
	{
		ParentUI->ExecuteQuickBrew(RecipeData);
	}
}

UAlchemyUI* UAlchemyRecipeEntryWidget::GetParentAlchemyUI() const
{
	UUserWidget* Current = const_cast<UAlchemyRecipeEntryWidget*>(this);
	while (Current)
	{
		if (UAlchemyUI* AlchemyUI = Cast<UAlchemyUI>(Current))
		{
			return AlchemyUI;
		}
		Current = Current->GetTypedOuter<UUserWidget>();
	}
	return nullptr;
}
