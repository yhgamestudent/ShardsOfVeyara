// Fill out your copyright notice in the Description page of Project Settings.


#include "Bed.h"

#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "FarmingGameMode.h"
#include "TextLog.h"
#include "Components/BoxComponent.h"

// Sets default values
ABed::ABed()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	//콜리전 박스 설정
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ABed::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &ABed::OnEndOverlap);
}

void ABed::WakeUp()
{
	FadeWidget->OnFadeFinished.RemoveDynamic(this, &ABed::WakeUp);
	AFarmingGameMode* GM = Cast<AFarmingGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		GM->NextDay(WakeUpTime);
	}
	TargetPlayer->HealthRecovery(TargetPlayer->getPlayerMaxhealth());
	TargetPlayer->EnableInput(TargetPlayer->getPlayerController());
	if (FadeWidget) FadeWidget->SetTargetOpacity(0.f);
}

void ABed::Interact_Implementation(AAGSDCharacter* player)
{
	TargetPlayer = player;
	TargetPlayer->DisableInput(TargetPlayer->getPlayerController());
	if (TargetPlayer && TargetPlayer->Implements<UInteractionOwnerInterface>())
	{
		if (IInteractionOwnerInterface* InteractOwner = Cast<IInteractionOwnerInterface>(TargetPlayer))
		{
			if (UAGSDInteractionComponent* InteractionComp = InteractOwner->GetInteractionComponent())
			{
				InteractionComp->RemoveInteractableActor(this);
			}
		}
	}
	if (!WBP_FadeWidget) return;
	if (!FadeWidget) FadeWidget = CreateWidget<UFadeWidget>(TargetPlayer->getPlayerController(), WBP_FadeWidget);
	FadeWidget->OnFadeFinished.RemoveDynamic(this, &ABed::WakeUp);
	FadeWidget->OnFadeFinished.AddDynamic(this, &ABed::WakeUp);
	FadeWidget->SetRenderOpacity(0.0f);
	FadeWidget->SetTargetOpacity(1.0f);
	if (!FadeWidget->IsInViewport()) FadeWidget->AddToViewport(100);
	
	UTextLog::WriteTextLogByKeyword(TEXT("잠자기"));
}

void ABed::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool ABed::CanInteract_Implementation(AAGSDCharacter* player)
{
	return true;
}

void ABed::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->Implements<UInteractionOwnerInterface>())
	{
		if (IInteractionOwnerInterface* InteractOwner = Cast<IInteractionOwnerInterface>(OtherActor))
		{
			if (UAGSDInteractionComponent* InteractionComp = InteractOwner->GetInteractionComponent())
			{
				InteractionComp->AddInteractableActor(this);
			}
		}
	}
}

void ABed::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->Implements<UInteractionOwnerInterface>())
	{
		if (IInteractionOwnerInterface* InteractOwner = Cast<IInteractionOwnerInterface>(OtherActor))
		{
			if (UAGSDInteractionComponent* InteractionComp = InteractOwner->GetInteractionComponent())
			{
				InteractionComp->RemoveInteractableActor(this);
			}
		}
	}
}

