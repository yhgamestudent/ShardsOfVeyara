// Fill out your copyright notice in the Description page of Project Settings.

#include "AGSDWarningNoticeWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UAGSDWarningNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentOpacity = 0.0f;
	TargetOpacity = 0.0f;
	SetRenderOpacity(0.0f);

	// 알림은 표시 전용 위젯입니다. RenderOpacity 0이어도 히트 테스트는 살아 있으므로,
	// 뷰포트에 남아 있는 동안 아래 UI(인벤토리/봉헌 슬롯)의 마우스 입력을 가로채지 않도록 차단합니다.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UAGSDWarningNoticeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FMath::IsNearlyEqual(CurrentOpacity, TargetOpacity, 0.005f))
	{
		CurrentOpacity = FMath::FInterpTo(CurrentOpacity, TargetOpacity, InDeltaTime, FadeSpeed);
		SetRenderOpacity(CurrentOpacity);
	}
	else if (CurrentOpacity != TargetOpacity)
	{
		CurrentOpacity = TargetOpacity;
		SetRenderOpacity(CurrentOpacity);
	}
}

void UAGSDWarningNoticeWidget::SetNoticeText(const FText& InMessage)
{
	if (NoticeTextBlock)
	{
		NoticeTextBlock->SetText(InMessage);
	}
}

void UAGSDWarningNoticeWidget::SetTargetOpacity(float NewOpacity)
{
	TargetOpacity = FMath::Clamp<float>(NewOpacity, 0.0f, 1.0f);
}

void UAGSDWarningNoticeWidget::ShowNotice(const FText& InMessage, float InDuration)
{
	SetNoticeText(InMessage);
	SetTargetOpacity(1.0f);

	// 블루프린트 등에서 Visible로 되돌려도 하위 UI 입력을 가로채지 않도록 매 표시 시점마다 재보장
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 기존에 돌고 있던 페이드아웃 타이머가 있다면 취소하고 새로 설정 (연타 시 표시 시간 자동 연장)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		World->GetTimerManager().SetTimer(
			HideTimerHandle,
			this,
			&UAGSDWarningNoticeWidget::StartFadeOut,
			InDuration,
			false
		);
	}
}

void UAGSDWarningNoticeWidget::StartFadeOut()
{
	SetTargetOpacity(0.0f);
}

