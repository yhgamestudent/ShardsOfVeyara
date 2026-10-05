// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBar.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 플레이어 체력 바 위젯
 * - 평상시에는 체력 텍스트가 Collapsed(숨김) 상태입니다.
 * - 마우스 호버 시 현재 체력과 최대 체력이 텍스트(예: 800 / 1000)로 표시됩니다.
 */
UCLASS()
class AGSD_API UHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 체력 게이지 프로그레스 바 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthProgressBar;

	/**
	 * 현재 체력 / 최대 체력 표시용 텍스트 블록
	 * (WBP_HealthBar 에디터에서 HealthText 또는 HealthTextBlock으로 이름을 지정하면 자동 바인딩됩니다)
	 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HealthBar")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HealthBar")
	TObjectPtr<UTextBlock> HealthTextBlock;

	/** 마우스 호버 상태가 아닐 때의 표시 여부 (기본: Collapsed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar")
	ESlateVisibility UnhoveredVisibility = ESlateVisibility::Collapsed;

	/** 마우스 호버 상태일 때의 표시 여부 (기본: SelfHitTestInvisible) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar")
	ESlateVisibility HoveredVisibility = ESlateVisibility::SelfHitTestInvisible;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HealthBar")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HealthBar")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HealthBar")
	bool bIsHovered = false;

public:
	/** 체력 수치 갱신 및 프로그레스 바 / 텍스트 동기화 */
	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void UpdateHealth(float InCurrentHealth, float InMaxHealth);

	/** 체력 텍스트 표시/숨김 수동 제어 */
	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void SetHealthTextVisible(bool bVisible);

	/** 체력 텍스트 포맷팅 (블루프린트에서 오버라이드하여 커스텀 포맷 적용 가능) */
	UFUNCTION(BlueprintNativeEvent, Category = "HealthBar")
	FText GetFormattedHealthText(float InCurrentHealth, float InMaxHealth) const;
	virtual FText GetFormattedHealthText_Implementation(float InCurrentHealth, float InMaxHealth) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** HealthText 또는 HealthTextBlock 중 유효한 위젯 포인터 반환 */
	UTextBlock* GetEffectiveHealthTextBlock() const;

	/** 현재 체력 수치를 바탕으로 텍스트 내용 갱신 */
	void RefreshHealthText();
};
