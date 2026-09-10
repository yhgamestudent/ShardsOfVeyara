// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNotifyState_IgnorePawnCollision.h"
#include "AGSDCharacter.h"

UAnimNotifyState_IgnorePawnCollision::UAnimNotifyState_IgnorePawnCollision()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(0, 200, 255, 255); // 시원한 청록색
#endif
}

void UAnimNotifyState_IgnorePawnCollision::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
	{
		if (bIgnorePawnCollision)
		{
			Character->SetIgnorePawnCollision(true);
		}

		if (bMakeInvulnerable)
		{
			Character->SetCanBeDamaged(false);
		}
	}
}

void UAnimNotifyState_IgnorePawnCollision::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
	{
		if (bIgnorePawnCollision)
		{
			Character->SetIgnorePawnCollision(false);
		}

		if (bMakeInvulnerable)
		{
			Character->SetCanBeDamaged(true);
		}
	}
}

FString UAnimNotifyState_IgnorePawnCollision::GetNotifyName_Implementation() const
{
	return TEXT("Ignore Pawn Collision");
}
