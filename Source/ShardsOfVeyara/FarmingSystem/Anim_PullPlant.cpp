#include "Anim_PullPlant.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "Weeds.h"
#include "NiagaraFunctionLibrary.h"

void UAnim_PullPlant::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                             const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	AActor* OwnerActor = MeshComp->GetOwner();
	if (OwnerActor && OwnerActor->Implements<UInteractionOwnerInterface>())
	{
		IInteractionOwnerInterface* InteractOwner = Cast<IInteractionOwnerInterface>(OwnerActor);
		AActor* InteractableActor = nullptr;
		if (InteractOwner)
		{
			if (UAGSDInteractionComponent* InteractionComp = InteractOwner->GetInteractionComponent())
			{
				InteractableActor = InteractionComp->GetCurrentInteractableActor();
			}
		}
		if (InteractableActor)
		{
			InteractableActor->SetActorHiddenInGame(true);

			if (AWeeds* Weed = Cast<AWeeds>(InteractableActor))
			{
				if (Weed->WeedEffect)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						MeshComp->GetWorld(),
						Weed->WeedEffect,
						Weed->GetActorLocation(),
						Weed->GetActorRotation()
					);
				}
			}
		}
	}
}
