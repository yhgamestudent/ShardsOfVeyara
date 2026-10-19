// Fill out your copyright notice in the Description page of Project Settings.


#include "AlchemyUI.h"
#include "Alchemy/AlchemyTable.h"
#include "AlchemyInventoryUI.h"
#include "AlchemyCropSlotBase.h"
#include "Alchemy/UI/AlchemyRecipeBookWidget.h"
#include "DragEnterWidget.h"
#include "AGSDCharacter.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Inventory/UI/AGSDItemNotificationWidget.h"
#include "Struct_ItemData.h"
#include "Engine/DataTable.h"
#include "GameplayLogSubsystem.h"
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

	if (RecipeBookButton)
	{
		RecipeBookButton->OnClicked.RemoveDynamic(this, &UAlchemyUI::ToggleRecipeBook);
		RecipeBookButton->OnClicked.AddDynamic(this, &UAlchemyUI::ToggleRecipeBook);
	}

	if (WBP_AlchemyRecipeBookWidget)
	{
		WBP_AlchemyRecipeBookWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	bIsRecipeBookOpen = false;

	RefreshMaterialAddresses();
	InitInsertedSlot();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn()))
		{
			OwningCharacter = Character;
			Character->RegisterCloseableUI(this);
			Character->SetCurrentActionCategory(TEXT("PotionCrafting"));
		}
	}
}

void UAlchemyUI::NativeDestruct()
{
	if (OwningCharacter.IsValid())
	{
		OwningCharacter->UnregisterCloseableUI(this);
		OwningCharacter->SetCurrentActionCategory(TEXT(""));
	}
	Super::NativeDestruct();
}

void UAlchemyUI::OnCloseButtonClicked()
{
	RemoveFromParent();
}

void UAlchemyUI::CloseUI_Implementation()
{
	// 1차 ESC: 레시피 북이 열려있다면 레시피 북 패널만 먼저 닫음
	if (bIsRecipeBookOpen)
	{
		OpenRecipeBook(false);
		return;
	}

	// 2차 ESC: 연금술 UI 전체 닫기 및 카메라 복귀
	if (AAlchemyTable* Table = Cast<AAlchemyTable>(OwnerActor))
	{
		Table->EndAlchemy();
	}
	else
	{
		OnWidgetClosed.Broadcast();
		OnCloseButtonClicked();
	}
}

void UAlchemyUI::ToggleRecipeBook()
{
	OpenRecipeBook(!bIsRecipeBookOpen);
}

void UAlchemyUI::OpenRecipeBook(bool bOpen)
{
	bIsRecipeBookOpen = bOpen;

	if (!WBP_AlchemyRecipeBookWidget) return;

	if (bOpen)
	{
		// 닫히는 도중 다시 열렸을 경우, 숨김 콜백 해제
		if (RecipeBookAnim)
		{
			UnbindAllFromAnimationFinished(RecipeBookAnim);
		}

		WBP_AlchemyRecipeBookWidget->SetVisibility(ESlateVisibility::Visible);
		WBP_AlchemyRecipeBookWidget->RefreshRecipeList();

		if (RecipeBookAnim)
		{
			// 닫히던 도중에 다시 열었다면 현재 위치에서 이어서 정방향 재생!
			float StartTime = 0.0f;
			if (IsAnimationPlaying(RecipeBookAnim))
			{
				StartTime = GetAnimationCurrentTime(RecipeBookAnim);
			}
			PlayAnimation(RecipeBookAnim, StartTime, 1, EUMGSequencePlayMode::Forward);
		}
	}
	else
	{
		if (RecipeBookAnim)
		{
			// 열리던 도중에 닫았다면 현재 위치에서 이어서 역방향 재생!
			// (멈춰있는 상태라면 StartTime 0.0f 전달 시 엔진이 자동으로 끝 지점에서부터 역재생)
			float StartTime = 0.0f;
			if (IsAnimationPlaying(RecipeBookAnim))
			{
				StartTime = GetAnimationCurrentTime(RecipeBookAnim);
			}

			FWidgetAnimationDynamicEvent FinishedEvent;
			FinishedEvent.BindDynamic(this, &UAlchemyUI::OnRecipeBookCloseFinished);
			BindToAnimationFinished(RecipeBookAnim, FinishedEvent);

			PlayAnimation(RecipeBookAnim, StartTime, 1, EUMGSequencePlayMode::Reverse);
		}
		else
		{
			WBP_AlchemyRecipeBookWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UAlchemyUI::OnRecipeBookCloseFinished()
{
	if (!bIsRecipeBookOpen && WBP_AlchemyRecipeBookWidget)
	{
		WBP_AlchemyRecipeBookWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAlchemyUI::ShowNotice_Implementation(const FText& NoticeText)
{
	if (TXT_NoticeMessage)
	{
		TXT_NoticeMessage->SetText(NoticeText);
	}

	if (NoticeAnim)
	{
		PlayAnimation(NoticeAnim);
	}
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
			ShowNotice(FText::FromString(TEXT("재료 2개를 먼저 채워주세요!")));
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
				if (PotionItemID.IsEmpty() || PotionItemID == TEXT("Sludge"))
				{
					PotionItemID = TEXT("Sludge"); // 예외 방지 디폴트
					if (UGameInstance* GameInst = Character->GetGameInstance())
					{
						if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())
						{
							LogSubsystem->IncrementFailedPotionCrafting();
						}
					}
				}
				else
				{
					if (UGameInstance* GameInst = Character->GetGameInstance())
					{
						if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())
						{
							LogSubsystem->IncrementSuccessfulPotionCrafting(1);
							LogSubsystem->RecordPotionCrafted(PotionItemID);
						}
					}
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

				if (WBP_AlchemyRecipeBookWidget && bIsRecipeBookOpen)
				{
					WBP_AlchemyRecipeBookWidget->RefreshRecipeList();
				}
			}
		}
		return;
	}

	// 2. 일반 농작물 투입 처리 (가방 영역 재료 등)
	// 솥에는 이미 최대 2개의 재료만 들어갈 수 있으므로 제한
	if (Table->GetInsertedItemID().Num() >= 2)
	{
		ShowNotice(FText::FromString(TEXT("솥이 가득 찼습니다. 솥을 먼저 비워주세요!")));
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

void UAlchemyUI::ExecuteQuickBrew(const FPotionData& RecipeToBrew)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn());
	if (!Character || !Character->InventoryComponent) return;

	AAlchemyTable* Table = Cast<AAlchemyTable>(OwnerActor);
	if (!Table) return;

	// 1. 솥 상태 검사: 재료가 1개라도 들어있으면 빠른 제작 차단
	if (Table->GetInsertedItemID().Num() > 0)
	{
		ShowNotice(FText::FromString(TEXT("솥을 먼저 비워주세요!")));
		return;
	}

	// 2. 인벤토리 공간 검사: 완성 포션이 들어갈 슬롯이 있는지 확인
	UAGSDInventoryComponent* InvComp = Character->InventoryComponent;
	bool bHasInventorySpace = (InvComp->FindStackableSlot(RecipeToBrew.ItemID) != -1) || (InvComp->FindEmptySlot() != -1);
	if (!bHasInventorySpace)
	{
		ShowNotice(FText::FromString(TEXT("가방 공간이 부족합니다!")));
		return;
	}

	// 3. 재료 보유 수량 검사 (작물 A, B, 차원 결정)
	RefreshMaterialAddresses();

	auto GetItemTotalCount = [this](const FString& ItemID) -> int32
	{
		if (const FStruct_MaterialAddress* FoundAddr = MaterialAddress.Find(ItemID))
		{
			int32 Total = 0;
			for (const FStruct_SlotAddress& Slot : FoundAddr->Address)
			{
				Total += Slot.Amount;
			}
			return Total;
		}
		return 0;
	};

	int32 CountA = GetItemTotalCount(RecipeToBrew.IngredientA);
	int32 CountB = GetItemTotalCount(RecipeToBrew.IngredientB);
	int32 CrystalCount = GetItemTotalCount(TEXT("DimensionCrystal"));

	bool bCanCraft = false;
	if (RecipeToBrew.IngredientA.Equals(RecipeToBrew.IngredientB, ESearchCase::IgnoreCase))
	{
		bCanCraft = (CountA >= 2) && (CrystalCount >= 1);
	}
	else
	{
		bCanCraft = (CountA >= 1) && (CountB >= 1) && (CrystalCount >= 1);
	}

	if (!bCanCraft)
	{
		ShowNotice(FText::FromString(TEXT("재료가 부족합니다!")));
		return;
	}

	// 4. 재료 차감 헬퍼 람다
	auto DeductItems = [this, InvComp](const FString& ItemID, int32 RequiredAmount) -> bool
	{
		FStruct_MaterialAddress* FoundAddr = MaterialAddress.Find(ItemID);
		if (!FoundAddr) return false;

		int32 Needed = RequiredAmount;
		for (int32 i = 0; i < FoundAddr->Address.Num() && Needed > 0; ++i)
		{
			int32 SlotIdx = FoundAddr->Address[i].Index;
			int32 Available = FoundAddr->Address[i].Amount;
			int32 ToRemove = FMath::Min(Needed, Available);

			if (InvComp->RemoveItem(SlotIdx, ToRemove))
			{
				Needed -= ToRemove;
			}
		}
		return Needed == 0;
	};

	// 5. 실제 인벤토리에서 차감 진행
	if (RecipeToBrew.IngredientA.Equals(RecipeToBrew.IngredientB, ESearchCase::IgnoreCase))
	{
		DeductItems(RecipeToBrew.IngredientA, 2);
	}
	else
	{
		DeductItems(RecipeToBrew.IngredientA, 1);
		DeductItems(RecipeToBrew.IngredientB, 1);
	}
	DeductItems(TEXT("DimensionCrystal"), 1);

	// 6. 포션 획득 및 통계 기록
	FString PotionItemID = RecipeToBrew.ItemID;
	if (UGameInstance* GameInst = Character->GetGameInstance())
	{
		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())
		{
			LogSubsystem->IncrementSuccessfulPotionCrafting(1);
			LogSubsystem->RecordPotionCrafted(PotionItemID);
		}
	}

	int32 OutRemainingQty = 0;
	FStruct_ItemData OutItemData;
	InvComp->AddItemByID(PotionItemID, 1, OutRemainingQty, OutItemData);

	// 7. 알림 및 솥 연출 트리거
	AddItemNotification(PotionItemID);
	Table->TargetRecipe = RecipeToBrew;
	Table->SplashPot(true); // 물 튀김 및 파티클 연출

	UTextLog::WriteTextLogByString(TEXT("원클릭 포션 제조 성공"), PotionItemID);

	// 8. UI 동기화 및 레시피 북 갱신
	RefreshMaterialAddresses();
	InitInsertedSlot();

	if (WBP_AlchemyRecipeBookWidget && bIsRecipeBookOpen)
	{
		WBP_AlchemyRecipeBookWidget->RefreshRecipeList();
	}
}
