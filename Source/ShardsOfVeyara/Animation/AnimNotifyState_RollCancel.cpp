// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNotifyState_RollCancel.h"
#include "AGSDCharacter.h"

void UAnimNotifyState_RollCancel::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp)
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
		{
			Character->SetCanRollCancel(true);
		}
	}
}

void UAnimNotifyState_RollCancel::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
		{
			Character->SetCanRollCancel(false);
		}
	}
}
