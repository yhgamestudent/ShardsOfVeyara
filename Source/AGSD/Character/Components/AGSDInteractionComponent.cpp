#include "AGSDInteractionComponent.h"
#include "AGSDCharacter.h"
#include "AGSDPlayerController.h"
#include "Interaction.h"
#include "Components/PrimitiveComponent.h"

UAGSDInteractionComponent::UAGSDInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 캐릭터 Tick에서 대행 호출하므로 자체 틱은 꺼둡니다.
}

void UAGSDInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AAGSDCharacter>(GetOwner());
}

void UAGSDInteractionComponent::UpdateInteractionState(float DeltaSeconds)
{
	if (!OwnerCharacter) return;

	AActor* MinDistanceActor = MinDistActor();

	if (CurrentInteractableActor == MinDistanceActor)
	{
		return;
	}
	
	// 이전 포커싱 대상의 하이라이트 비활성화
	SetHighLight(CurrentInteractableActor, false);

	CurrentInteractableActor = MinDistanceActor;

	AAGSDPlayerController* PC = OwnerCharacter->getPlayerController();

	if (CurrentInteractableActor != nullptr)
	{
		// 위젯 출력 요청
		IInteraction::Execute_ShowWidget(CurrentInteractableActor, OwnerCharacter);
		// 새 대상의 하이라이트 활성화
		SetHighLight(CurrentInteractableActor, true);
	}
	else
	{
		// 대상이 없으면 위젯 숨김
		if (PC)
		{
			PC->HideInteractionWidget();
		}
	}
}

void UAGSDInteractionComponent::TryInteract()
{
	if (!OwnerCharacter) return;

	if (CanInteract && IsValid(CurrentInteractableActor) && CurrentInteractableActor->Implements<UInteraction>())
	{
		IInteraction::Execute_Interact(CurrentInteractableActor, OwnerCharacter);
	}
}

void UAGSDInteractionComponent::AddInteractableActor(AActor* NewActor)
{
	if (NewActor)
	{
		InteractableActorsInRange.Add(NewActor);
		CanInteract = true;
	}
}

void UAGSDInteractionComponent::RemoveInteractableActor(AActor* ActorToRemove)
{
	if (ActorToRemove)
	{
		InteractableActorsInRange.Remove(ActorToRemove);
	}
	
	if (InteractableActorsInRange.Num() <= 0)
	{
		CanInteract = false;
	}
}

AActor* UAGSDInteractionComponent::MinDistActor()
{
	if (!OwnerCharacter) return nullptr;

	float MinDistance = FLT_MAX;
	const FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	AActor* MinDistanceActor = nullptr;
	
	for (AActor* CurrentActor : InteractableActorsInRange)
	{
		if (!CurrentActor) continue;

		if (!IInteraction::Execute_CanInteract(CurrentActor, OwnerCharacter)) continue;
		
		const float DistanceSq = FVector::DistSquared(PlayerLocation, CurrentActor->GetActorLocation());

		if (DistanceSq < MinDistance)
		{
			MinDistance = DistanceSq;
			MinDistanceActor = CurrentActor;
		}
	}

	return MinDistanceActor;
}

void UAGSDInteractionComponent::SetHighLight(AActor* TargetActor, bool bActive)
{
	if (TargetActor)
	{
		if (TargetActor->ActorHasTag(TEXT("NPC")))
		{
			return;
		}

		TArray<UPrimitiveComponent*> Comps;
		TargetActor->GetComponents(Comps);
		for (UPrimitiveComponent* Comp : Comps)
		{
			if (Comp)
			{
				Comp->SetRenderCustomDepth(bActive);
			}
		}
	}
}
