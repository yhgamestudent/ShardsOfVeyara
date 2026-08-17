// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void SetInteractionText(const FText& NewText);
	void SetTargetOpacity(float NewOpacity);
	
protected:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InteractionText;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;
	
private:
	/** 최종적으로 도달하고자 하는 목표 투명도 (0.0 또는 1.0) */
	float TargetOpacity = 0.0f;
    
	/** 투명도 보간 속도 (EditDefaultsOnly로 BP에서 설정 가능) */
	UPROPERTY(EditDefaultsOnly, Category = "Fade")
	float FadeSpeed = 8.0f;
};
