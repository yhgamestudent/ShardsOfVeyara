// Fill out your copyright notice in the Description page of Project Settings.

#include "FarmingSystem/AppleTree.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AGSDGameStateBase.h"
#include "SOVGameInstance.h"
#include "AGSDCharacter.h"
#include "AGSDPlayerController.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Inventory/UI/AGSDPlayerHUD.h"
#include "TextLog.h"

// Sets default values
AAppleTree::AAppleTree()
{
	PrimaryActorTick.bCanEverTick = false;

	TreeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TreeMesh"));
	RootComponent = TreeMesh;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AAppleTree::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AAppleTree::OnEndOverlap);
}

// Called when the game starts or when spawned
void AAppleTree::BeginPlay()
{
	Super::BeginPlay();

	USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance());
	int32 CurrentDay = 1;

	if (AAGSDGameStateBase* GS = Cast<AAGSDGameStateBase>(UGameplayStatics::GetGameState(GetWorld())))
	{
		GS->OnDayChangedDelegate.AddUObject(this, &AAppleTree::HandleDayPassed);
		CurrentDay = GS->GetCurrentDay();
	}
	else if (GI)
	{
		CurrentDay = GI->CurrentDay;
	}

	// 1. 인스턴스로부터 저장 상태 복원
	if (GI)
	{
		FAppleTreeSaveData LoadedData;
		if (GI->GetAppleTreeData(GetName(), LoadedData))
		{
			bHasApple = LoadedData.bHasApple;
			LastCheckedDay = LoadedData.LastCheckedDay;

			// 2. 거점 밖 또는 맵 이탈 상태에서 경과된 날짜 시뮬레이션
			if (!bHasApple && LastCheckedDay < CurrentDay)
			{
				int32 ElapsedDays = CurrentDay - LastCheckedDay;
				for (int32 i = 0; i < ElapsedDays; ++i)
				{
					if (FMath::FRand() <= AppleSpawnProbability)
					{
						bHasApple = true;
						break;
					}
				}
			}
		}
	}

	LastCheckedDay = CurrentDay;
	SaveStateToGI();
	UpdateTreeMesh();
}

void AAppleTree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SaveStateToGI();
	Super::EndPlay(EndPlayReason);
}

void AAppleTree::HandleDayPassed(int32 CurrentDay)
{
	if (!bHasApple)
	{
		if (FMath::FRand() <= AppleSpawnProbability)
		{
			bHasApple = true;
		}
	}

	LastCheckedDay = CurrentDay;
	SaveStateToGI();
	UpdateTreeMesh();
}

void AAppleTree::SaveStateToGI()
{
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		FAppleTreeSaveData SaveData;
		SaveData.TreeName = GetName();
		SaveData.bHasApple = bHasApple;
		SaveData.LastCheckedDay = LastCheckedDay;

		GI->UpdateAppleTreeData(SaveData);
	}
}

void AAppleTree::UpdateTreeMesh()
{
	if (bHasApple && AppleMesh)
	{
		TreeMesh->SetStaticMesh(AppleMesh);
	}
	else if (!bHasApple && NoAppleMesh)
	{
		TreeMesh->SetStaticMesh(NoAppleMesh);
	}
}

void AAppleTree::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

void AAppleTree::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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

bool AAppleTree::CanInteract_Implementation(AAGSDCharacter* player)
{
	return bHasApple;
}

void AAppleTree::ShowWidget_Implementation(ACharacter* player)
{
	if (!bHasApple) return;

	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
	{
		PlayerController->ShowInteractionWidget(InteractActionText);
	}
}

void AAppleTree::Interact_Implementation(AAGSDCharacter* player)
{
	if (!bHasApple || !player || !player->InventoryComponent) return;

	int32 OutRemainingQty = 0;
	FStruct_ItemData OutItemData;

	bool bAdded = player->InventoryComponent->AddItemByID(AppleItemID, AppleHarvestAmount, OutRemainingQty, OutItemData);
	if (bAdded)
	{
		int32 AcquiredQty = AppleHarvestAmount - OutRemainingQty;
		UTextLog::WriteTextLogByStringAndFloat(TEXT("아이템 획득"), OutItemData.ItemName.ToString(), static_cast<float>(AcquiredQty));

		if (player->PlayerHUDRef)
		{
			player->PlayerHUDRef->AddItemNotification(OutItemData, AcquiredQty);
		}

		bHasApple = false;
		SaveStateToGI();
		UpdateTreeMesh();
	}
}
