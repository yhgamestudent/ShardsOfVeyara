// Fill out your copyright notice in the Description page of Project Settings.


#include "PickUpItem.h"
#include "Components/SphereComponent.h"
#include "AGSDPlayerController.h"
#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "Struct_ItemData.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Inventory/UI/AGSDPlayerHUD.h"
#include "TextLog.h"

APickUpItem::APickUpItem()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetSimulatePhysics(true);

	Mesh->SetCollisionResponseToChannel(
	ECollisionChannel::ECC_Pawn,
	ECR_Overlap
	);
	Mesh->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Camera, 
		ECR_Overlap
		);
	Mesh->SetCollisionResponseToChannel(
			ECollisionChannel::ECC_PhysicsBody,  // 물리 시뮬레이션 중인 다른 액터
			ECollisionResponse::ECR_Ignore       // 충돌을 완전히 무시
		);

	//콜리전 박스 설정
	CollisionBox = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetSphereRadius(50);
	CollisionBox->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &APickUpItem::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &APickUpItem::OnEndOverlap);

}

void APickUpItem::BeginPlay()
{
	Super::BeginPlay();
	if (Holding)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APickUpItem::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("PickUpItemOnBeginOverlap"));
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

void APickUpItem::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
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

void APickUpItem::Interact_Implementation(AAGSDCharacter* player)
{
	if (!player || !player->InventoryComponent)
	{
		return;
	}

	int32 OutRemainingQty = 0;
	FStruct_ItemData OutItemData;

	// 인벤토리 컴포넌트의 AddItemByID를 호출하여 공용 데이터 테이블에서 처리하도록 함
	bool bAdded = player->InventoryComponent->AddItemByID(ItemID, Amount, OutRemainingQty, OutItemData);

	if (bAdded)
	{
		// 실제 획득 수량 계산
		int32 AcquiredQty = Amount - OutRemainingQty;

		// 아이템 획득 TextLog 기록
		UTextLog::WriteTextLogByStringAndFloat(TEXT("아이템 획득"), OutItemData.ItemName.ToString(), static_cast<float>(AcquiredQty));

		// 플레이어 HUD에 아이템 획득 알림 출력
		if (player->PlayerHUDRef)
		{
			player->PlayerHUDRef->AddItemNotification(OutItemData, AcquiredQty);
		}

		// 부분 획득 정책 반영 (완전히 획득했으면 소멸, 남았다면 수량 갱신 후 월드 잔류)
		if (OutRemainingQty <= 0)
		{
			Destroy();
		}
		else
		{
			Amount = OutRemainingQty;
		}
	}
}

void APickUpItem::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool APickUpItem::CanInteract_Implementation(AAGSDCharacter* player)
{
	return true;
}

void APickUpItem::DisableCollisionForHolding()
{
	Holding = true;
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionBox->SetGenerateOverlapEvents(false);
	}
	if (Mesh)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
	}
}


