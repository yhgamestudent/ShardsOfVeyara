// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AGSDCloseableUIInterface.h"
#include "CookingUI.generated.h"

class UButton;
class AAGSDCharacter;

/**
 * UCookingUI
 * 요리(Cooking) UI 화면을 담당하는 C++ 부모 클래스입니다.
 */
UCLASS()
class AGSD_API UCookingUI : public UUserWidget, public IUIClosable
{
	GENERATED_BODY()

public:
	// IUIClosable 인터페이스 구현
	virtual void CloseUI_Implementation() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnCloseButtonClicked();

	// ── UI 바인딩 ──
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Close;

	// WBP_CookHotbar 1~10 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_5;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_6;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_7;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_8;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_9;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> WBP_CookHotbar_10;

	// ── 데이터 변수 ──
	UPROPERTY(BlueprintReadOnly, Category = "Cooking")
	TObjectPtr<AAGSDCharacter> Farmer;

	UPROPERTY(BlueprintReadOnly, Category = "Cooking")
	TArray<TObjectPtr<UUserWidget>> InsertedCropSlotArray;
};
