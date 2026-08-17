// AGSDPlayerHUD.cpp - HUD 위젯 구현

#include "AGSDPlayerHUD.h"
#include "AGSDCharacter.h"
#include "AGSDHotbarWidget.h"
#include "AGSDInventoryWidget.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Animation/WidgetAnimation.h"
#include "Components/VerticalBox.h"
#include "Inventory/UI/AGSDItemNotificationWidget.h"
#include "Inventory/UI/AGSDComboGuideWidget.h"
#include "Character/Components/AGSDComboGuideComponent.h"

void UAGSDPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// 소유 캐릭터를 가져와 콤보 가이드 컴포넌트 델리게이트 및 인벤토리 컴포넌트 바인딩
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(OwningPawn))
		{
			if (WBP_ComboGuide && Character->GetComboGuideComponent())
			{
				Character->GetComboGuideComponent()->OnComboGuideUpdated.AddDynamic(WBP_ComboGuide, &UAGSDComboGuideWidget::UpdateComboGuide);
			}
		}

		if (UAGSDInventoryComponent* InvComp = OwningPawn->FindComponentByClass<UAGSDInventoryComponent>())
		{
			InitializeHUD(InvComp);
		}
	}

	// 기본적으로 메인 인벤토리 창은 닫힌 상태로 시작
	CloseInventory();
}

void UAGSDPlayerHUD::InitializeHUD(UAGSDInventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent) return;

	InventoryComponent = InInventoryComponent;

	if (WBP_Hotbar)
	{
		WBP_Hotbar->InitializeHotbar(InInventoryComponent);
	}

	if (WBP_InventoryUI)
	{
		WBP_InventoryUI->InitializeInventory(InInventoryComponent);
	}
}

void UAGSDPlayerHUD::ToggleInventory()
{
	if (IsInventoryOpen())
	{
		CloseInventory();
	}
	else
	{
		if (WBP_InventoryUI)
		{
			WBP_InventoryUI->SetVisibility(ESlateVisibility::Visible);
		}

		if (HotbarSlide)
		{
			PlayAnimation(HotbarSlide, 0.0f, 1, EUMGSequencePlayMode::Forward);
		}

		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetShowMouseCursor(true);
			
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
		}

		if (APawn* OwningPawn = GetOwningPlayerPawn())
		{
			if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(OwningPawn))
			{
				Character->RegisterCloseableUI(this);
			}
		}
	}
}

void UAGSDPlayerHUD::CloseInventory()
{
	if (WBP_InventoryUI)
	{
		WBP_InventoryUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (HotbarSlide)
	{
		PlayAnimation(HotbarSlide, 0.0f, 1, EUMGSequencePlayMode::Reverse);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(OwningPawn))
		{
			Character->UnregisterCloseableUI(this);
		}
	}
}

bool UAGSDPlayerHUD::IsInventoryOpen() const
{
	if (WBP_InventoryUI)
	{
		return WBP_InventoryUI->GetVisibility() == ESlateVisibility::Visible;
	}
	return false;
}

void UAGSDPlayerHUD::AddItemNotification(FStruct_ItemData ItemData, int32 Amount)
{
	if (!VB_ItemNotificationList || !ItemNotificationClass)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	UAGSDItemNotificationWidget* NotiWidget = CreateWidget<UAGSDItemNotificationWidget>(PC, ItemNotificationClass);
	if (NotiWidget)
	{
		NotiWidget->SetupNotification(ItemData.ItemName.ToString(), Amount, ItemData.ItemIcon);
		VB_ItemNotificationList->AddChild(NotiWidget);
	}
}

void UAGSDPlayerHUD::CloseUI_Implementation()
{
	CloseInventory();
}

