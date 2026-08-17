// Fill out your copyright notice in the Description page of Project Settings.

#include "Tribute/TributeUI.h"
#include "Tribute/Tribute.h"
#include "AGSDCharacter.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Alchemy/UI/AlchemyInventoryUI.h"
#include "Alchemy/UI/AlchemyCropSlotBase.h"
#include "Tribute/TributeTextUI.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/DragDropOperation.h"
#include "Alchemy/UI/DragEnterWidget.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void UTributeUI::NativeConstruct()
{
	Super::NativeConstruct();

	// 필수 에셋 할당 확인 (누락 시 런타임 경고 콜스택 방출)
	ensureMsgf(ItemDataTable != nullptr, TEXT("TributeUI: ItemDataTable이 블루프린트에서 누락되었습니다!"));
	ensureMsgf(TributeItem != nullptr, TEXT("TributeUI: TributeItem 클래스가 블루프린트에서 누락되었습니다!"));

	SetIsFocusable(true);

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UTributeUI::OnCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UTributeUI::OnCloseButtonClicked);
	}

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		FarmerChar = Cast<AAGSDCharacter>(PC->GetPawn());
	}
}

void UTributeUI::NativeDestruct()
{
	if (FarmerChar)
	{
		FarmerChar->UnregisterCloseableUI(this);
	}
	Super::NativeDestruct();
}

void UTributeUI::PlayFadeIn()
{
	if (FadeinAnim)
	{
		PlayAnimation(FadeinAnim);
	}
}

void UTributeUI::OnCloseButtonClicked()
{
	if (GetClass()->ImplementsInterface(UUIClosable::StaticClass()))
	{
		IUIClosable::Execute_CloseUI(this);
	}
	else
	{
		RemoveFromParent();
	}
}

void UTributeUI::setOwnerActor(AActor* owner)
{
	OwnerActor = owner;
	TributeActor = Cast<ATribute>(owner);

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		FarmerChar = Cast<AAGSDCharacter>(PC->GetPawn());
	}

	if (FarmerChar)
	{
		FarmerChar->RegisterCloseableUI(this);
	}

	RebuildMaterialAddresses();

	if (WBP_TributeTextUI && TributeActor)
	{
		WBP_TributeTextUI->SetNextTributeItem(TributeActor->CurrentLevelTributeItems);
	}
}

void UTributeUI::RebuildMaterialAddresses()
{
	if (!FarmerChar) return;

	MaterialAddress = UAlchemyInventoryUI::ScanInventoryForMaterials(FarmerChar);

	if (CropSlot)
	{
		CropSlot->InitializeSlots(MaterialAddress, EHoldingState::EHS_Crop, false);
	}
}

void UTributeUI::InsertMaterial(UAlchemyCropSlotBase* InsertedSlot)
{
	if (!InsertedSlot || !FarmerChar || !FarmerChar->InventoryComponent || !TributeActor) return;

	FString ItemID = InsertedSlot->GetItemID();
	int32 Amount = InsertedSlot->GetAmount();

	int32* RequiredAmountPtr = TributeActor->CurrentLevelTributeItems.Find(ItemID);
	if (!RequiredAmountPtr) return; // 필요한 제물이 아닌 경우 무시

	int32 RequiredAmount = *RequiredAmountPtr;
	int32 AmountToRemove = FMath::Min(Amount, RequiredAmount);
	if (AmountToRemove <= 0) return;

	int32 OriginalAmountToRemove = AmountToRemove;

	FStruct_MaterialAddress* FoundAddr = MaterialAddress.Find(ItemID);
	if (FoundAddr)
	{
		// 여러 슬롯에 나뉘어 있을 수 있으므로 순차적으로 차감
		for (int32 i = 0; i < FoundAddr->Address.Num(); ++i)
		{
			if (AmountToRemove <= 0) break;

			const FStruct_SlotAddress& SlotAddr = FoundAddr->Address[i];
			int32 QtyInSlot = SlotAddr.Amount;
			int32 QtyToSub = FMath::Min(AmountToRemove, QtyInSlot);

			if (QtyToSub > 0)
			{
				int32 AbsoluteIndex = SlotAddr.Index;

				if (AbsoluteIndex != -1)
				{
					// 인벤토리 컴포넌트에서 수량 차감
					FarmerChar->InventoryComponent->RemoveItem(AbsoluteIndex, QtyToSub);
				}

				AmountToRemove -= QtyToSub;
			}
		}
	}

	int32 SubtedAmount = OriginalAmountToRemove - AmountToRemove;
	if (SubtedAmount > 0)
	{
		// 봉헌 액터에 성공적으로 투입되었음을 전달
		TributeActor->SuccessInsert(ItemID, SubtedAmount);
	}

	// 차감 후 인벤토리 주소 맵 재빌드 및 UI 갱신
	RebuildMaterialAddresses();

	if (WBP_TributeTextUI)
	{
		WBP_TributeTextUI->SetNextTributeItem(TributeActor->CurrentLevelTributeItems);
	}
}

void UTributeUI::SubHotbarItemAmount(int32 Index, int32 AmountToRemove)
{
	if (FarmerChar && FarmerChar->InventoryComponent)
	{
		// 인벤토리 컴포넌트의 RemoveItem API를 사용하여 핫바 슬롯(0~9) 차감
		FarmerChar->InventoryComponent->RemoveItem(Index, AmountToRemove);
	}
}

bool UTributeUI::NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	if (Operation)
	{
		UAlchemyCropSlotBase* DroppedSlot = Cast<UAlchemyCropSlotBase>(Operation->Payload);
		if (DroppedSlot && WBP_DragEnterWidget && WBP_DragEnterWidget->bHovered)
		{
			InsertMaterial(DroppedSlot);

			WBP_DragEnterWidget->bHovered = false;

			if (TributeActor)
			{
				TributeActor->SetFireBurnActive(false);
			}

			return true;
		}
	}

	return Super::NativeOnDrop(MyGeometry, DragDropEvent, Operation);
}

#if WITH_EDITOR
EDataValidationResult UTributeUI::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (ItemDataTable == nullptr)
	{
		Context.AddError(FText::FromString(TEXT("TributeUI: ItemDataTable이 블루프린트에서 할당되지 않았습니다!")));
		Result = EDataValidationResult::Invalid;
	}

	if (TributeItem == nullptr)
	{
		Context.AddError(FText::FromString(TEXT("TributeUI: TributeItem 클래스가 블루프린트에서 할당되지 않았습니다!")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

void UTributeUI::CloseUI_Implementation()
{
	if (FarmerChar)
	{
		FarmerChar->UnregisterCloseableUI(this);
	}
	if (TributeActor)
	{
		TributeActor->EndTribute();
	}
	RemoveFromParent();
}