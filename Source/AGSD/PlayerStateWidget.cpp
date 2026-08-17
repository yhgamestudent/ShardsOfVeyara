// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerStateWidget.h"
#include "Components/TextBlock.h"

void UPlayerStateWidget::SetDamageText(float playerdamage)
{
	DamageText->SetText(FText::AsNumber(playerdamage));
}
