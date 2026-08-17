// Fill out your copyright notice in the Description page of Project Settings.

#include "Chest.h"
#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "Components/BoxComponent.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "AGSDChestAndInventory.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Inventory/UI/AGSDPlayerHUD.h"
#include "SOVGameInstance.h"

// Sets default values
AChest::AChest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	//루트 컴포넌트 설정
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkeletalMesh"));
	RootComponent = Mesh;
	//콜리전 박스 설정
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AChest::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AChest::OnEndOverlap);

	// C++ 인벤토리 컴포넌트 생성
	InventoryComponent = CreateDefaultSubobject<UAGSDInventoryComponent>(TEXT("InventoryComponent"));
}

// Called when the game starts or when spawned
void AChest::BeginPlay()
{
	Super::BeginPlay();
	
	// 게임인스턴스에서 상자 데이터 복원
	USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance());
	if (GI)
	{
		FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
		FString UniqueKey = CurrentLevelName + GetName();

		if (GI->ChestMap.Contains(UniqueKey))
		{
			FStruct_WorldChestData* ChestData = GI->ChestMap.Find(UniqueKey);
			if (ChestData && InventoryComponent)
			{
				InventoryComponent->SetAllSlots(ChestData->ChestSlots);
			}
		}
	}
}

void AChest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 게임인스턴스에 상자 데이터 저장
	USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance());
	if (GI)
	{
		FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
		FString UniqueKey = CurrentLevelName + GetName();

		FStruct_WorldChestData ChestData;
		if (InventoryComponent)
		{
			ChestData.ChestSlots = InventoryComponent->GetAllSlots();
		}

		GI->ChestMap.Add(UniqueKey, ChestData);
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AChest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AChest::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

void AChest::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
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

void AChest::Interact_Implementation(AAGSDCharacter* player)
{
	Open_Chest_UI();
}

void AChest::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool AChest::CanInteract_Implementation(AAGSDCharacter* player)
{
	return player->bCanOpenChest;
}

void AChest::Open_Chest_UI()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn());
	if (!Character) return;

	// 플레이어 HUD 참조 확인 및 인벤토리 닫기 처리
	if (Character->PlayerHUDRef)
	{
		if (Character->PlayerHUDRef->IsInventoryOpen())
		{
			Character->PlayerHUDRef->CloseInventory();
		}
	}

	if (ChestWidgetClass)
	{
		// 캐릭터 상태 참조를 먼저 설정하여 UI 생성(Construct) 시점에 유효한 상자 참조를 가지도록 합니다.
		Character->OpenedChest = this;

		UAGSDChestAndInventory* ChestWidget = CreateWidget<UAGSDChestAndInventory>(PC, ChestWidgetClass);
		if (ChestWidget)
		{

			// PlayerHUD의 TopCanvasPanel에 추가
			if (Character->PlayerHUDRef)
			{
				UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(Character->PlayerHUDRef->GetWidgetFromName(TEXT("TopCanvasPanel")));
				if (CanvasPanel)
				{
					UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(ChestWidget));
					if (CanvasSlot)
					{
						CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
						CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					}
				}
			}

			// 화면 표시 (Viewport 추가)
			ChestWidget->AddToViewport();

			// 입력 모드 설정 (GameAndUI로 변경하여 마우스 락 풀고 캐릭터 퍼즈 감지 지원)
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(ChestWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
}

