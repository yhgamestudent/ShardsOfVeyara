// Fill out your copyright notice in the Description page of Project Settings.


#include "AlchemyUI.h"
#include "Alchemy/AlchemyTable.h"
#include "AlchemyInventoryUI.h"
#include "AlchemyCropSlotBase.h"
#include "DragEnterWidget.h"
#include "AGSDCharacter.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Components/Button.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Inventory/UI/AGSDItemNotificationWidget.h"
#include "Struct_ItemData.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/DragDropOperation.h"
#include "EnhancedInputSubsystems.h"
#include "TextLog.h"

void UAlchemyUI::CallOnCropInserted()
{
	OnCropInserted.Broadcast();
}

void UAlchemyUI::PlayFadeIn()
{
	if (FadeinAnim)
	{
		PlayAnimation(FadeinAnim);
	}
}

void UAlchemyUI::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UAlchemyUI::OnCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UAlchemyUI::OnCloseButtonClicked);
	}

	RefreshMaterialAddresses();
	InitInsertedSlot();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn()))
		{
			OwningCharacter = Character;
			Character->RegisterCloseableUI(this);
		}
	}
}

void UAlchemyUI::NativeDestruct()
{
	if (OwningCharacter.IsValid())
	{
		OwningCharacter->UnregisterCloseableUI(this);
	}
	Super::NativeDestruct();
}

void UAlchemyUI::OnCloseButtonClicked()
{
	RemoveFromParent();
}

void UAlchemyUI::RefreshMaterialAddresses()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn());
	if (!Character) return;

	MaterialAddress = UAlchemyInventoryUI::ScanInventoryForMaterials(Character);

	// 자식 인벤토리 UI 초기화 및 슬롯 생성 (작물 및 차원 수정 분리 표시)
	if (CropSlot)
	{
		CropSlot->InitializeSlots(MaterialAddress, EHoldingState::EHS_Crop, false);
	}

	if (DimensionCrystalSlot)
	{
		DimensionCrystalSlot->InitializeSlots(MaterialAddress, EHoldingState::EHS_Alchemy, false);
	}
}

void UAlchemyUI::InsertMaterial(UAlchemyCropSlotBase* InsertedSlot)
{
	if (!InsertedSlot) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn());
	if (!Character || !Character->InventoryComponent) return;

	AAlchemyTable* Table = Cast<AAlchemyTable>(OwnerActor);
	if (!Table) return;

	FString ItemID = InsertedSlot->GetItemID();

	// 1. 차원 수정 투입 처리
	if (ItemID.Equals(TEXT("DimensionCrystal"), ESearchCase::IgnoreCase))
	{
		// 솥에 이미 작물이 2개 채워져 있을 때만 투입 허용
		if (Table->GetInsertedItemID().Num() != 2)
		{
			return;
		}

		// 차원 수정 투입 시점에 솥 내부 재료 기반으로 레시피 예비 검증
		Table->UpdateTargetRecipe();

		// 레시피 없는 조합(Sludge)인 경우 차원 수정 투입 자체를 차단
		if (Table->TargetRecipe.ItemID.Equals(TEXT("Sludge"), ESearchCase::IgnoreCase))
		{
			return;
		}

		// 차원 수정 1개 수량 차감
		FStruct_MaterialAddress* FoundAddr = MaterialAddress.Find(ItemID);
		if (FoundAddr && FoundAddr->Address.Num() > 0)
		{
			int32 TargetSlotIndex = FoundAddr->Address[0].Index;
			if (Character->InventoryComponent->RemoveItem(TargetSlotIndex, 1))
			{
				// 차원 수정 투입 시점에 솥 내부 재료 기반으로 레시피 최종 재검증(Double-check)
				Table->UpdateTargetRecipe();

				// 포션 완성 처리 (기존에 솥 내부 2개 재료에 대한 TargetRecipe가 세팅되어 있음)
				FString PotionItemID = Table->TargetRecipe.ItemID;
				if (PotionItemID.IsEmpty())
				{
					PotionItemID = TEXT("Sludge"); // 예외 방지 디폴트
				}

				// 인벤토리에 완성된 포션 지급
				int32 OutRemainingQty = 0;
				FStruct_ItemData OutItemData;
				Character->InventoryComponent->AddItemByID(PotionItemID, 1, OutRemainingQty, OutItemData);

				// UI 알림 리스트에 획득 포션 추가 알림 호출
				AddItemNotification(PotionItemID);

				// 솥 재료 리스트 비우기 및 완성 연출 트리거 (clear = true)
				Table->GetInsertedItemID().Empty();
				Table->SplashPot(true);

				// 델리게이트 이벤트 브로드캐스트
				CallOnCropInserted();

				// 주소 맵 및 UI 동기화
				RefreshMaterialAddresses();
				InitInsertedSlot();
			}
		}
		return;
	}

	// 2. 일반 농작물 투입 처리 (가방 영역 재료 등)
	// 솥에는 이미 최대 2개의 재료만 들어갈 수 있으므로 제한
	if (Table->GetInsertedItemID().Num() >= 2)
	{
		return;
	}

	// 인벤토리 주소 TMap에서 실제 슬롯 인덱스 찾아 수량 1 차감
	FStruct_MaterialAddress* FoundAddr = MaterialAddress.Find(ItemID);
	if (FoundAddr && FoundAddr->Address.Num() > 0)
	{
		int32 TargetSlotIndex = FoundAddr->Address[0].Index;
		
		// 캐릭터 인벤토리 컴포넌트에서 실제로 1개 제거
		if (Character->InventoryComponent->RemoveItem(TargetSlotIndex, 1))
		{
			// 솥 리스트에 투입된 아이템 ID 추가
			Table->GetInsertedItemID().Add(ItemID);
			
			// 솥 효과음 및 파티클 재생 (clear = false)
			Table->SplashPot(false);

			if (Table->GetInsertedItemID().Num() >= 2)
			{
				if (Table->TargetRecipe.ItemID.Equals(TEXT("Sludge"), ESearchCase::IgnoreCase))
				{
					UTextLog::WriteTextLogByKeyword(TEXT("포션 제조 실패"));
				}
				else
				{
					UTextLog::WriteTextLogByString(TEXT("포션 제조"), Table->TargetRecipe.ItemID);
				}
			}

			// 델리게이트 이벤트 브로드캐스트
			CallOnCropInserted();

			// 주소 맵 및 UI 동기화
			RefreshMaterialAddresses();
			InitInsertedSlot();
		}
	}
}

bool UAlchemyUI::NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	if (Operation)
	{
		UAlchemyCropSlotBase* DroppedSlot = Cast<UAlchemyCropSlotBase>(Operation->Payload);
		
		// 드래그 호버 상태에서만 투입 수락
		if (DroppedSlot && WBP_DragEnterWidget && WBP_DragEnterWidget->bHovered)
		{
			InsertMaterial(DroppedSlot);

			// 드롭 처리되었으므로 하이라이트 끄기
			AAlchemyTable* Table = Cast<AAlchemyTable>(OwnerActor);
			if (Table && Table->PotMeshComponent)
			{
				Table->PotMeshComponent->SetRenderCustomDepth(false);
			}

			// 호버 리셋
			WBP_DragEnterWidget->bHovered = false;

			return true;
		}
	}

	return Super::NativeOnDrop(MyGeometry, DragDropEvent, Operation);
}

void UAlchemyUI::InitInsertedSlot_Implementation()
{
	AAlchemyTable* Table = Cast<AAlchemyTable>(OwnerActor);
	if (!Table) return;

	const TArray<FString>& InsertedList = Table->GetInsertedItemID();

	// 1번째 투입 슬롯 갱신
	if (InsertedCropSlot_0)
	{
		if (InsertedList.Num() > 0)
		{
			FString ItemID = InsertedList[0];
			TMap<FString, FStruct_MaterialAddress> TempMap;
			FStruct_SlotAddress SlotAddr;
			SlotAddr.Index = -1;
			SlotAddr.Amount = 1;
			TempMap.FindOrAdd(ItemID).Address.Add(SlotAddr);

			InsertedCropSlot_0->InitializeSlots(TempMap, EHoldingState::EHS_None, true);
		}
		else
		{
			InsertedCropSlot_0->InitializeSlots(TMap<FString, FStruct_MaterialAddress>(), EHoldingState::EHS_None, true);
		}
	}

	// 2번째 투입 슬롯 갱신
	if (InsertedCropSlot_1)
	{
		if (InsertedList.Num() > 1)
		{
			FString ItemID = InsertedList[1];
			TMap<FString, FStruct_MaterialAddress> TempMap;
			FStruct_SlotAddress SlotAddr;
			SlotAddr.Index = -1;
			SlotAddr.Amount = 1;
			TempMap.FindOrAdd(ItemID).Address.Add(SlotAddr);

			InsertedCropSlot_1->InitializeSlots(TempMap, EHoldingState::EHS_None, true);
		}
		else
		{
			InsertedCropSlot_1->InitializeSlots(TMap<FString, FStruct_MaterialAddress>(), EHoldingState::EHS_None, true);
		}
	}
}

void UAlchemyUI::AddItemNotification(const FString& ItemID, int32 Amount)
{
	if (!VB_ItemNotificationList || !ItemNotificationClass || !ItemDataTable) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	FStruct_ItemData* ItemData = ItemDataTable->FindRow<FStruct_ItemData>(FName(*ItemID), TEXT("AddItemNotification"));
	if (!ItemData) return;

	UAGSDItemNotificationWidget* NotiWidget = CreateWidget<UAGSDItemNotificationWidget>(PC, ItemNotificationClass);
	if (NotiWidget)
	{
		NotiWidget->SetupNotification(ItemData->ItemName.ToString(), Amount, ItemData->ItemIcon);
		VB_ItemNotificationList->AddChild(NotiWidget);
	}
}

void UAlchemyUI::CloseUI_Implementation()
{
	if (AAlchemyTable* Table = Cast<AAlchemyTable>(OwnerActor))
	{
		Table->EndAlchemy();
	}
}
