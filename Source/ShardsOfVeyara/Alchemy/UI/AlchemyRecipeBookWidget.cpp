// Fill out your copyright notice in the Description page of Project Settings.

#include "Alchemy/UI/AlchemyRecipeBookWidget.h"
#include "Alchemy/UI/AlchemyRecipeEntryWidget.h"
#include "Alchemy/UI/AlchemyUI.h"
#include "Alchemy/AlchemyTable.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Button.h"
#include "GameplayLogSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UAlchemyRecipeBookWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseRecipeBookButton)
	{
		CloseRecipeBookButton->OnClicked.RemoveDynamic(this, &UAlchemyRecipeBookWidget::OnCloseButtonClicked);
		CloseRecipeBookButton->OnClicked.AddDynamic(this, &UAlchemyRecipeBookWidget::OnCloseButtonClicked);
	}
}

void UAlchemyRecipeBookWidget::OnCloseButtonClicked()
{
	if (UAlchemyUI* ParentUI = GetParentAlchemyUI())
	{
		ParentUI->OpenRecipeBook(false);
	}
}

UAlchemyUI* UAlchemyRecipeBookWidget::GetParentAlchemyUI() const
{
	UUserWidget* Current = const_cast<UAlchemyRecipeBookWidget*>(this);
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

namespace
{
	int32 GetItemQuantityFromAddressMap(const TMap<FString, FStruct_MaterialAddress>& AddressMap, const FString& ItemID)
	{
		if (const FStruct_MaterialAddress* FoundAddr = AddressMap.Find(ItemID))
		{
			int32 Total = 0;
			for (const FStruct_SlotAddress& SlotAddr : FoundAddr->Address)
			{
				Total += SlotAddr.Amount;
			}
			return Total;
		}
		return 0;
	}

	struct FRecipeSortItem
	{
		FPotionData Recipe;
		bool bDiscovered = false;
		bool bCanCraft = false;
		int32 CountA = 0;
		int32 CountB = 0;
		int32 CrystalCount = 0;
	};
}

void UAlchemyRecipeBookWidget::RefreshRecipeList()
{
	if (!SB_RecipeList || !RecipeEntryWidgetClass) return;

	SB_RecipeList->ClearChildren();

	UAlchemyUI* ParentUI = GetParentAlchemyUI();
	if (!ParentUI) return;

	AAlchemyTable* Table = Cast<AAlchemyTable>(ParentUI->GetOwnerActor());
	if (!Table || !Table->GetAlchemyDataTable()) return;

	// 1. 전체 레시피 데이터 조회
	TArray<FPotionData*> AllRecipes;
	static const FString ContextString(TEXT("RefreshRecipeList"));
	Table->GetAlchemyDataTable()->GetAllRows<FPotionData>(ContextString, AllRecipes);

	// 2. 해금 기록(Subsystem) 확인
	UGameplayLogSubsystem* LogSubsystem = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		LogSubsystem = GI->GetSubsystem<UGameplayLogSubsystem>();
	}

	// 3. 현재 플레이어 인벤토리의 재료 수량 맵
	const TMap<FString, FStruct_MaterialAddress>& MaterialAddress = ParentUI->GetMaterialAddress();
	int32 CrystalCount = GetItemQuantityFromAddressMap(MaterialAddress, TEXT("DimensionCrystal"));

	TArray<FRecipeSortItem> SortList;
	SortList.Reserve(AllRecipes.Num());

	for (FPotionData* RecipeRow : AllRecipes)
	{
		if (!RecipeRow) continue;

		// 찌꺼기(Sludge)는 일반 레시피 목록에서 제외
		if (RecipeRow->ItemID.Equals(TEXT("Sludge"), ESearchCase::IgnoreCase)) continue;

		FRecipeSortItem Item;
		Item.Recipe = *RecipeRow;

		// 해금 여부 판정 (한 번이라도 제작 기록이 있는가)
		if (LogSubsystem)
		{
			Item.bDiscovered = LogSubsystem->GetCraftedPotionCount(RecipeRow->ItemID) > 0;
		}
		else
		{
			Item.bDiscovered = false;
		}

		// 재료 수량 판정
		Item.CountA = GetItemQuantityFromAddressMap(MaterialAddress, RecipeRow->IngredientA);
		Item.CountB = GetItemQuantityFromAddressMap(MaterialAddress, RecipeRow->IngredientB);
		Item.CrystalCount = CrystalCount;

		// 제작 가능 판정 (재료 A, B가 동일 작물인 경우 2개 이상 필요)
		if (RecipeRow->IngredientA.Equals(RecipeRow->IngredientB, ESearchCase::IgnoreCase))
		{
			Item.bCanCraft = (Item.CountA >= 2) && (Item.CrystalCount >= 1);
		}
		else
		{
			Item.bCanCraft = (Item.CountA >= 1) && (Item.CountB >= 1) && (Item.CrystalCount >= 1);
		}

		SortList.Add(Item);
	}

	// 4. 스마트 정렬 (1: 제작가능, 2: 재료부족, 3: 미발견)
	SortList.Sort([](const FRecipeSortItem& Left, const FRecipeSortItem& Right)
	{
		auto GetPriority = [](const FRecipeSortItem& Target) -> int32
		{
			if (Target.bDiscovered && Target.bCanCraft) return 0; // 제작 가능 최우선
			if (Target.bDiscovered && !Target.bCanCraft) return 1; // 재료 부족
			return 2; // 미발견
		};

		int32 PriorityLeft = GetPriority(Left);
		int32 PriorityRight = GetPriority(Right);

		if (PriorityLeft != PriorityRight)
		{
			return PriorityLeft < PriorityRight;
		}

		return Left.Recipe.ItemID < Right.Recipe.ItemID;
	});

	// 5. 위젯 생성 및 ScrollBox 추가
	for (const FRecipeSortItem& Item : SortList)
	{
		UAlchemyRecipeEntryWidget* EntryWidget = CreateWidget<UAlchemyRecipeEntryWidget>(this, RecipeEntryWidgetClass);
		if (EntryWidget)
		{
			EntryWidget->SetupEntry(Item.Recipe, Item.bDiscovered, Item.bCanCraft, Item.CountA, Item.CountB, Item.CrystalCount);
			if (UPanelSlot* PanelSlot = SB_RecipeList->AddChild(EntryWidget))
			{
				if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(PanelSlot))
				{
					ScrollSlot->SetPadding(EntryPadding);
				}
			}
		}
	}
}
