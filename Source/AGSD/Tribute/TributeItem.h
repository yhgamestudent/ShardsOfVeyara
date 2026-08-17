// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "TributeItem.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UTributeItem : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tribute")
	FORCEINLINE void SetCropIcon(UTexture2D* icon) { if(CropIcon) CropIcon->SetBrushFromTexture(icon); };

	UFUNCTION(BlueprintCallable, Category = "Tribute")
	FORCEINLINE void SetCropAmount(int amount) { if(CropAmount) CropAmount->SetText(FText::AsNumber(amount)); }
	
protected:
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UImage* CropIcon;
	
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* CropAmount;
};
