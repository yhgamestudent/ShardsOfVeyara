// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNotify_EnableMovement.h"
#include "AGSDCharacter.h"

void UAnimNotify_EnableMovement::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
		{
			Character->EnableMovementFromAttack();
		}
	}
}
