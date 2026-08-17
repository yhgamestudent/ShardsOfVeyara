// Fill out your copyright notice in the Description page of Project Settings.


#include "FadeWidget.h"

void UFadeWidget::SetTargetOpacity(float NewOpacity)
{
	TargetOpacity = FMath::Clamp<float>(NewOpacity, 0.0f, 1.0f);
	bIsTickPaused = false;
}

void UFadeWidget::SetFadeSpeed(float NewSpeed)
{
	FadeSpeed = NewSpeed;
}

void UFadeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsTickPaused) return;
	
	float CurrentOpacity = GetRenderOpacity();

	// 1. [보간] 현재 투명도에서 목표 투명도로 부드럽게 이동합니다.
	// FInterpTo는 Target에 가까워질수록 속도가 줄어들어 자연스러운 감속 효과를 줍니다.
	float NextOpacity = FMath::FInterpTo(CurrentOpacity, TargetOpacity, InDeltaTime, FadeSpeed);

	// Render Opacity 적용
	SetRenderOpacity(NextOpacity);

	if (FMath::IsNearlyEqual(NextOpacity, TargetOpacity, 0.005f))
	{
		if (TargetOpacity == 0.0f)
		{
			if (IsInViewport()) // 이중 체크: 이미 제거되었는지 확인
			{
				RemoveFromParent();
			}
		}
		OnFadeFinished.Broadcast();
	}
}

void UFadeWidget::NativeConstruct()
{
	Super::NativeConstruct();
}
