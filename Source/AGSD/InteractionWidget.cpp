// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionWidget.h"

#include "Components/TextBlock.h"

void UInteractionWidget::SetInteractionText(const FText& NewText)
{
	if (InteractionText)
	{
		InteractionText->SetText(NewText);
	}
}

void UInteractionWidget::SetTargetOpacity(float NewOpacity)
{
	TargetOpacity = FMath::Clamp<float>(NewOpacity, 0.0f, 1.0f);
}

void UInteractionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	float CurrentOpacity = GetRenderOpacity();

	// 1. [보간] 현재 투명도에서 목표 투명도로 부드럽게 이동합니다.
	// FInterpTo는 Target에 가까워질수록 속도가 줄어들어 자연스러운 감속 효과를 줍니다.
	float NextOpacity = FMath::FInterpTo(CurrentOpacity, TargetOpacity, InDeltaTime, FadeSpeed);

	// Render Opacity 적용
	SetRenderOpacity(NextOpacity);

	// 2. [최적화/Cleanup] 목표가 0(사라짐)이고, 거의 다 사라졌다면 위젯을 뷰포트에서 제거합니다.
	if (TargetOpacity == 0.0f && NextOpacity <= 0.01f)
	{
		if (IsInViewport()) // 이중 체크: 이미 제거되었는지 확인
		{
			RemoveFromParent();
		}
	}
}

void UInteractionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetRenderOpacity(0.0f);
}
