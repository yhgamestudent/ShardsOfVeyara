// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHealthBar::NativeConstruct()
{
	Super::NativeConstruct();

	if (HealthProgressBar)
	{
		// 에디터/블루프린트에 설정된 프로그레스바 기본 Fill 색상 자동 캐싱
		BaseFillColor = HealthProgressBar->FillColorAndOpacity;
	}

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
		const float RawPercent = (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;

		// 배경 이미지는 항상 100% 온전하게 표시 (RenderOpacity, Visibility를 건드리지 않음)
		HealthProgressBar->SetVisibility(ESlateVisibility::Visible);
		HealthProgressBar->SetRenderOpacity(1.0f);

		// 1. 체력이 0인 경우: 퍼센트 0 및 빨간색 Fill만 완전 투명화 (배경 틀은 선명하게 유지)
		if (CurrentHealth <= 0.0f || RawPercent <= 0.0f)
		{
			HealthProgressBar->SetPercent(0.0f);
			FLinearColor ZeroColor = BaseFillColor;
			ZeroColor.A = 0.0f;
			HealthProgressBar->SetFillColorAndOpacity(ZeroColor);
		}
		else
		{
			// 2. 0.032 미만 초빈사 구간: 최소 원형 퍼센트(0.032) 유지 + 빨간색 Fill의 알파만 페이드아웃
			if (RawPercent < MinCapsulePercent)
			{
				HealthProgressBar->SetPercent(MinCapsulePercent);

				if (bFadeOpacityBelowMinPercent)
				{
					const float FadeAlpha = FMath::Clamp(RawPercent / MinCapsulePercent, 0.0f, 1.0f);
					FLinearColor FadedColor = BaseFillColor;
					FadedColor.A = BaseFillColor.A * FadeAlpha;
					HealthProgressBar->SetFillColorAndOpacity(FadedColor);
				}
				else
				{
					HealthProgressBar->SetFillColorAndOpacity(BaseFillColor);
				}
			}
			else
			{
				// 3. 0.032 이상 정상 구간: 원형 비율 온전 유지 및 원래 빨간색 복원
				HealthProgressBar->SetPercent(RawPercent);
				HealthProgressBar->SetFillColorAndOpacity(BaseFillColor);
			}
		}
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
