// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGSDPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "AGSD.h"
#include "Animation/WidgetAnimation.h"
#include "Widgets/Input/SVirtualJoystick.h"

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
