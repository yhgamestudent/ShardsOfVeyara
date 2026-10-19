// Fill out your copyright notice in the Description page of Project Settings.

#include "SOVShortcutRequirementWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

USOVShortcutRequirementWidget::USOVShortcutRequirementWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USOVShortcutRequirementWidget::UpdateRequirement(UTexture2D* InIconTexture, int32 InCurrentAmount, int32 InRequiredAmount)
{
	CachedCurrentAmount = InCurrentAmount;
	CachedRequiredAmount = InRequiredAmount;
	bCanUnlock = (InCurrentAmount >= InRequiredAmount);

	// 1. 아이콘 이미지 갱신
	if (ItemIcon)
	{
		if (InIconTexture)
		{
			ItemIcon->SetBrushFromTexture(InIconTexture, true);
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 2. 수량 텍스트 갱신 (AmountText 또는 AmountTextBlock 중 바인딩된 위젯에 적용)
	UTextBlock* TargetTextWidget = AmountText ? AmountText.Get() : AmountTextBlock.Get();
	if (TargetTextWidget)
	{
		const FText FormattedAmount = FText::Format(
			FText::FromString(TEXT("{0}/{1}")),
			FText::AsNumber(InCurrentAmount),
			FText::AsNumber(InRequiredAmount)
		);
		TargetTextWidget->SetText(FormattedAmount);

		// 수량 충족 시 연두색, 부족 시 흰색/연붉은색
		if (bCanUnlock)
		{
			TargetTextWidget->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.0f, 0.4f, 1.0f)));
		}
		else
		{
			TargetTextWidget->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
		}
	}

	// 3. 충족 인디케이터 표시/숨김
	if (ReadyIndicatorWidget)
	{
		ReadyIndicatorWidget->SetVisibility(bCanUnlock ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 4. 블루프린트 연출 이벤트 호출
	OnRequirementUpdated(InCurrentAmount, InRequiredAmount, bCanUnlock);
}
