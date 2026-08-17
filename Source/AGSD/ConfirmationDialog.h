// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmationDialog.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UConfirmationDialog : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UButton* YesButton;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UButton* NoButton;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UButton* CloseButton;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnClickCloseButton();
};
