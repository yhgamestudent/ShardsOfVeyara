#include "Inventory/UI/AGSDItemNotificationWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

void UAGSDItemNotificationWidget::SetupNotification(const FString& InName, int32 InAmount, UTexture2D* InIcon)
{
	ItemName = InName;
	ItemAmount = InAmount;
	ItemIcon = InIcon;

	// 1. 비주얼 컴포넌트 데이터 세팅
	if (IMG_ItemIcon && ItemIcon)
	{
		IMG_ItemIcon->SetBrushFromTexture(ItemIcon);
	}

	if (TXT_ItemNameAmount)
	{
		// "{ItemName} x {ItemAmount}" 포맷팅 적용
		FText FormattedText = FText::Format(
			FText::FromString(TEXT("{0} x {1}")),
			FText::FromString(ItemName),
			FText::AsNumber(ItemAmount)
		);
		TXT_ItemNameAmount->SetText(FormattedText);
	}

	// 2. 슬라이드 인 애니메이션 재생 (Forward)
	if (Animation_SlideRight)
	{
		PlayAnimation(Animation_SlideRight, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
	}

	// 3. 2초 동안 화면에 표시한 후 슬라이드 아웃 타이머를 동작
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			SlideOutTimerHandle,
			this,
			&UAGSDItemNotificationWidget::StartSlideOut,
			2.0f,
			false
		);
	}
}

void UAGSDItemNotificationWidget::StartSlideOut()
{
	// 슬라이드 아웃 애니메이션 재생 (Reverse)
	if (Animation_SlideRight)
	{
		PlayAnimation(Animation_SlideRight, 0.f, 1, EUMGSequencePlayMode::Reverse, 1.f);
		
		float AnimDuration = Animation_SlideRight->GetEndTime();
		
		// 애니메이션 역재생이 완료되는 시점에 위젯 파괴
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				DestroyTimerHandle,
				this,
				&UAGSDItemNotificationWidget::DestroyNotification,
				AnimDuration,
				false
			);
		}
	}
	else
	{
		// 애니메이션이 유효하지 않을 경우 즉시 파괴
		DestroyNotification();
	}
}

void UAGSDItemNotificationWidget::DestroyNotification()
{
	RemoveFromParent();
}
