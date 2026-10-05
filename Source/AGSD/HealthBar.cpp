// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHealthBar::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 생성 시 텍스트를 Collapsed 상태로 초기화
	SetHealthTextVisible(false);
	RefreshHealthText();
}

void UHealthBar::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bIsHovered = true;
	SetHealthTextVisible(true);
}

void UHealthBar::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bIsHovered = false;
	SetHealthTextVisible(false);
}

UTextBlock* UHealthBar::GetEffectiveHealthTextBlock() const
{
	if (HealthText)
	{
		return HealthText;
	}
	return HealthTextBlock;
}

void UHealthBar::SetHealthTextVisible(bool bVisible)
{
	if (UTextBlock* TargetText = GetEffectiveHealthTextBlock())
	{
		TargetText->SetVisibility(bVisible ? HoveredVisibility : UnhoveredVisibility);
	}
}

void UHealthBar::UpdateHealth(float InCurrentHealth, float InMaxHealth)
{
	CurrentHealth = FMath::Max(0.0f, InCurrentHealth);
	MaxHealth = FMath::Max(0.0f, InMaxHealth);

	if (HealthProgressBar)
	{
		const float Percent = (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;
		HealthProgressBar->SetPercent(Percent);
	}

	RefreshHealthText();
}

FText UHealthBar::GetFormattedHealthText_Implementation(float InCurrentHealth, float InMaxHealth) const
{
	const int32 CurInt = FMath::RoundToInt(InCurrentHealth);
	const int32 MaxInt = FMath::RoundToInt(InMaxHealth);
	return FText::FromString(FString::Printf(TEXT("%d / %d"), CurInt, MaxInt));
}

void UHealthBar::RefreshHealthText()
{
	if (UTextBlock* TargetText = GetEffectiveHealthTextBlock())
	{
		TargetText->SetText(GetFormattedHealthText(CurrentHealth, MaxHealth));
	}
}
