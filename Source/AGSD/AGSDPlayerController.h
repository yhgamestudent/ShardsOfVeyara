// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InteractionWidget.h"
#include "ClockWidget.h"
#include "AGSDPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AAGSDPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AAGSDPlayerController();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UInteractionWidget> WBP_InteractionWidget;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> WBP_ClockWidget;

	UFUNCTION(BlueprintCallable)
	void ShowInteractionWidget(const FText& NewText);
	
	void ShowClockWidget();
	void HideInteractionWidget();
private:
	UPROPERTY()
	class UInteractionWidget* InteractionWidget;
	UPROPERTY()
	class UUserWidget* ClockWidget;
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};
