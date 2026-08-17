// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ClockWidget.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UClockWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* TimeText;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* DayText;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	
protected:
	UFUNCTION()
	void SetTimeText(float time);
	void SetDayText(int32 day);
};
