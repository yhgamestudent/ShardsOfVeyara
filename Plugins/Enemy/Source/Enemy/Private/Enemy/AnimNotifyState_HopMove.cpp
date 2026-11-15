#include "Enemy/AnimNotifyState_HopMove.h"

#include "Components/SkeletalMeshComponent.h"
#include "Enemy/BaseExploderEnemy.h"

void UAnimNotifyState_HopMove::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (ABaseExploderEnemy* Exploder = MeshComp ? Cast<ABaseExploderEnemy>(MeshComp->GetOwner()) : nullptr)
	{
		Exploder->SetHopMoveWindowActive(true);
	}
}

void UAnimNotifyState_HopMove::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (ABaseExploderEnemy* Exploder = MeshComp ? Cast<ABaseExploderEnemy>(MeshComp->GetOwner()) : nullptr)
	{
		Exploder->SetHopMoveWindowActive(false);
	}
}
