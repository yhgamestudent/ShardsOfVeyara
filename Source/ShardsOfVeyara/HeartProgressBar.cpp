// Fill out your copyright notice in the Description page of Project Settings.


#include "HeartProgressBar.h"

#include "Components/Image.h"

void UHeartProgressBar::SetPercent(float percent)
{
	if (!IsValid(HeartProgressBar))
	{
		HeartProgressBar = UMaterialInstanceDynamic::Create(MI_HeartProgressBar, this);
		HeartProgressBarImage->SetBrushFromMaterial(HeartProgressBar);
	}
	HeartProgressBar->SetScalarParameterValue(FName("Percent"), percent);
}
