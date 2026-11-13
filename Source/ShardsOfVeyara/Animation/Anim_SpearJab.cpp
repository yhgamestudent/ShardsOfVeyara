// Fill out your copyright notice in the Description page of Project Settings.


#include "Anim_SpearJab.h"

#include "AGSDCharacter.h"

void UAnim_SpearJab::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                            const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	AAGSDCharacter* Player = Cast<AAGSDCharacter>(MeshComp->GetOwner());
	if (Player)
	{
	}
}