// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FadeWidget.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFadeFinished);
UCLASS()
class AGSD_API UFadeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Fade")
	void SetTargetOpacity(float NewOpacity);

	UFUNCTION(BlueprintCallable, Category = "Fade")
	void SetFadeSpeed(float NewSpeed);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnFadeFinished OnFadeFinished;

	bool bIsTickPaused = true;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;
	
private:
	/** 최종적으로 도달하고자 하는 목표 투명도 (0.0 또는 1.0) */
	float TargetOpacity = 1.0f;
    
	/** 투명도 보간 속도 (EditDefaultsOnly로 BP에서 설정 가능) */
	UPROPERTY(EditDefaultsOnly, Category = "Fade")
	float FadeSpeed = 2.0f;
};
