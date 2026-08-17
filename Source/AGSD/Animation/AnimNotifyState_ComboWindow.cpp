// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_ComboWindow.h"

#include "AGSDCharacter.h"

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
	{
		Character->SetCanCombo(true);
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
	{
		Character->OnComboWindowEnd();
	}
}
