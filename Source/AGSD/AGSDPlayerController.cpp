// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGSDPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "AGSD.h"
#include "Animation/WidgetAnimation.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "Struct_ItemData.h"

AAGSDPlayerController::AAGSDPlayerController()
{
	DefaultMouseCursor = EMouseCursor::Default;
	CurrentMouseCursor = EMouseCursor::Default;
}

void AAGSDPlayerController::ShowInteractionWidget(const FText& NewText)
{
	if (!WBP_InteractionWidget) return;
	if (!InteractionWidget) InteractionWidget = CreateWidget<UInteractionWidget>(this, WBP_InteractionWidget);
	InteractionWidget->SetInteractionText(NewText);
	InteractionWidget->SetTargetOpacity(1.0f);
	if (!InteractionWidget->IsInViewport())
	{
		InteractionWidget->AddToViewport();
	}
}

void AAGSDPlayerController::ShowClockWidget()
{
	if (!WBP_ClockWidget)
	{
		UE_LOG(LogTemp, Display, TEXT("WBP_ClockWidget Empty"));
		return;
	}
	if (!ClockWidget)
	{
		ClockWidget = CreateWidget<UUserWidget>(this, WBP_ClockWidget);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("ClockWidget Empty"));
	}
	if (!ClockWidget->IsInViewport())
	{
		ClockWidget->AddToViewport();
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("ClockWidget is In Viewport"));
	}
}

void AAGSDPlayerController::HideInteractionWidget()
{
	if (InteractionWidget && InteractionWidget->IsInViewport())
	{
		InteractionWidget->SetTargetOpacity(0.0f);
	}
}

void AAGSDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShowClockWidget();
	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogAGSD, Error, TEXT("Could not spawn mobile controls widget."));

		}
	}
}

void AAGSDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

// ═══════════════════════════════════════════════════
// ── 치트 콘솔 커맨드 (Cheat Console Commands) ──
// ═══════════════════════════════════════════════════

UAGSDInventoryComponent* AAGSDPlayerController::GetPlayerInventoryComponent() const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] Player pawn not found."));
		return nullptr;
	}

	UAGSDInventoryComponent* InvComp = ControlledPawn->FindComponentByClass<UAGSDInventoryComponent>();
	if (!InvComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] UAGSDInventoryComponent not found on player pawn."));
	}
	return InvComp;
}

void AAGSDPlayerController::GiveItem(const FString& ItemID, int32 Amount)
{
	if (Amount <= 0)
	{
		Amount = 1;
	}

	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	int32 RemainingQty = 0;
	FStruct_ItemData OutItemData;
	bool bSuccess = InvComp->AddItemByID(ItemID, Amount, RemainingQty, OutItemData);

	if (bSuccess)
	{
		int32 AddedQty = Amount - RemainingQty;
		FString Msg = FString::Printf(TEXT("[Cheat] Added item '%s' (%s) x%d to Inventory!"), *ItemID, *OutItemData.ItemName.ToString(), AddedQty);
		if (RemainingQty > 0)
		{
			Msg += FString::Printf(TEXT(" (Inventory Full! %d not added)"), RemainingQty);
		}

		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg);
	}
	else
	{
		FString Msg = FString::Printf(TEXT("[Cheat] Failed to give item '%s'. Check if ItemID exists in DataTable."), *ItemID);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, Msg);
	}
}

void AAGSDPlayerController::GiveAllItems(int32 Amount)
{
	if (Amount <= 0)
	{
		Amount = 1;
	}

	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	UDataTable* DataTable = InvComp->GetItemDataTable();
	if (!DataTable)
	{
		FString Msg = TEXT("[Cheat] Failed: ItemDataTable is null on InventoryComponent.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, Msg);
		return;
	}

	TArray<FName> RowNames = DataTable->GetRowNames();
	int32 SuccessCount = 0;

	for (const FName& RowName : RowNames)
	{
		int32 RemainingQty = 0;
		FStruct_ItemData OutItemData;
		if (InvComp->AddItemByID(RowName.ToString(), Amount, RemainingQty, OutItemData))
		{
			SuccessCount++;
		}
	}

	FString Msg = FString::Printf(TEXT("[Cheat] GiveAllItems finished: %d/%d item types added."), SuccessCount, RowNames.Num());
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, Msg);
}

void AAGSDPlayerController::ClearInventory()
{
	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	InvComp->ClearAllSlots();
	FString Msg = TEXT("[Cheat] Cleared all inventory slots.");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, Msg);
}

void AAGSDPlayerController::ListItems()
{
	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	UDataTable* DataTable = InvComp->GetItemDataTable();
	if (!DataTable)
	{
		FString Msg = TEXT("[Cheat] Failed: ItemDataTable is null on InventoryComponent.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, Msg);
		return;
	}

	TArray<FName> RowNames = DataTable->GetRowNames();
	UE_LOG(LogTemp, Log, TEXT("======== Available Items List (%d) ========"), RowNames.Num());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan, FString::Printf(TEXT("=== Available Items (%d) - Check Output Log ==="), RowNames.Num()));

	for (const FName& RowName : RowNames)
	{
		FStruct_ItemData* RowData = DataTable->FindRow<FStruct_ItemData>(RowName, TEXT("ListItems"));
		if (RowData)
		{
			UE_LOG(LogTemp, Log, TEXT("  - ID: [%s] | Name: [%s] | MaxQuantity: %d"), *RowName.ToString(), *RowData->ItemName.ToString(), RowData->MaxQuantity);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("  - ID: [%s]"), *RowName.ToString());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("==========================================="));
}
