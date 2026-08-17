// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStateWidget.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UPlayerStateWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* DamageText;

public:
	UFUNCTION(BlueprintCallable)
	void SetDamageText(float playerdamage);
};
