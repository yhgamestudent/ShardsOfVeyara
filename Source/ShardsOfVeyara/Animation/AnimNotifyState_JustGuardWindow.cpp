#include "AnimNotifyState_JustGuardWindow.h"
#include "Character/AGSDCharacter.h"
#include "Character/Components/AGSDGuardComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_JustGuardWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp)
	{
		if (AAGSDCharacter* OwnerChar = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
		{
			if (UAGSDGuardComponent* GuardComp = OwnerChar->GetGuardComponent())
			{
				GuardComp->SetJustGuardWindowActive(true);
			}
		}
	}
}

void UAnimNotifyState_JustGuardWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		if (AAGSDCharacter* OwnerChar = Cast<AAGSDCharacter>(MeshComp->GetOwner()))
		{
			if (UAGSDGuardComponent* GuardComp = OwnerChar->GetGuardComponent())
			{
				GuardComp->SetJustGuardWindowActive(false);
			}
		}
	}
}
