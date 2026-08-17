// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeartProgressBar.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UHeartProgressBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetPercent(float Percent);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* MI_HeartProgressBar;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstanceDynamic* HeartProgressBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Percent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	class UImage* HeartProgressBarImage;
};
