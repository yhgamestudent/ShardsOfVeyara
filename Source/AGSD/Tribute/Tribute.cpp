// Fill out your copyright notice in the Description page of Project Settings.


#include "Tribute/Tribute.h"

#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "TextLog.h"
#include "Tribute/TributeTextUI.h"
#include "Tribute/TributeUI.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/UI/AGSDPlayerHUD.h"

// Sets default values
ATribute::ATribute()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATribute::OnBeginOverlap);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ATribute::OnEndOverlap);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComp->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ATribute::BeginPlay()
{
	Super::BeginPlay();
	if (!TributeDataTable) return;
	
	GI = Cast<USOVGameInstance>(GetGameInstance());

	TributeLevel = GI->TributeLevel;
	CurrentLevelTributeItems = GI->CurrentLevelTributeItems;
	
	if (TributeLevel == 1)
	{
		SetNextTributeUI(TributeLevel);
	}
}

void ATribute::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GI->TributeLevel = TributeLevel;
	GI->CurrentLevelTributeItems = CurrentLevelTributeItems;
}

void ATribute::SetNextTributeUI(int32 level)
{
	FTributeData* CurrentLevelRow = TributeDataTable->FindRow<FTributeData>(FName(*FString::FromInt(level)), TEXT("ContextString"));
	if (CurrentLevelRow)
	{
		CurrentLevelTributeItems = CurrentLevelRow->TributeItems;
	}
}

// Called every frame
void ATribute::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATribute::Interact_Implementation(AAGSDCharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
	{
		Player = player;
		Player->Mining = true;
		bCanUseTribute = false;
		
		PlayerController->SetViewTargetWithBlend(this, BlendTime);
		
		// 위젯이 아직 없다면 여기서 생성
		if (!TributeWidget && TributeWidgetClass)
		{
			TributeWidget = CreateWidget<UTributeUI>(GetWorld(), TributeWidgetClass);
		}
		if (TributeWidget)
		{
			TributeWidget->setOwnerActor(this);
			TributeWidget->getCloseButton()->OnClicked.AddDynamic(this, &ATribute::EndTribute);
			
			TributeWidget->AddToViewport();

			TributeWidget->PlayFadeIn();
			
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(TributeWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
			PlayerController->bShowMouseCursor = true;

			// 봉헌 UI 활성화 시 기존 PlayerHUD 숨기기
			if (Player && Player->PlayerHUDRef)
			{
				Player->PlayerHUDRef->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void ATribute::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool ATribute::CanInteract_Implementation(AAGSDCharacter* player)
{
	return bCanUseTribute;
}

void ATribute::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
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

void ATribute::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
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

void ATribute::EndTribute()
{
	// 1. 플레이어 컨트롤러 가져오기
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;
    
	// 3. 카메라를 다시 캐릭터에게로 (부드럽게 복구)
	PC->SetViewTargetWithBlend(Player, BlendTime);

	// 4. 입력 모드를 다시 게임 전용으로 변경
	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = false;

	// 5. 캐릭터 상태 및 테이블 상태 복구
	if (Player)
	{
		Player->Mining = false; // 이동 가능하게 변경

		// 봉헌 UI 종료 시 PlayerHUD 복구
		if (Player->PlayerHUDRef)
		{
			Player->PlayerHUDRef->SetVisibility(ESlateVisibility::Visible);
		}
	}
	bCanUseTribute = true; // 다시 상호작용 가능하게

	// 6. 위젯 제거
	if (TributeWidget)
	{
		TributeWidget->getCloseButton()->OnClicked.RemoveDynamic(this, &ATribute::EndTribute);
		TributeWidget->RemoveFromParent();
		// 메모리 관리를 위해 필요하다면 AlchemyWidget = nullptr; 를 해줄 수도 있지만, 
		// 다시 열 때를 대비해 유지하는 것이 일반적입니다.
	}
}

void ATribute::SuccessInsert(FString ItemID, int32 AmountToRemove)
	{
	PlayFireNiagara();
	int* FoundAmount = CurrentLevelTributeItems.Find(ItemID);
	if (FoundAmount)
	{
		(*FoundAmount) = (*FoundAmount) - AmountToRemove;
	}

	bool bIsLevelComplete = true;
	for (auto& Elem : CurrentLevelTributeItems)
	{
		if (Elem.Value > 0)
		{
			bIsLevelComplete = false;
			break;
		}
	}

	if (bIsLevelComplete)
	{
		CurrentLevelTributeItems = {};
		PlayFireExplosionNiagara();
		Player->AddDamage(10.0f);
		SetNextTributeUI(++TributeLevel);
		UTextLog::WriteTextLogByFloat(TEXT("봉헌 단계"), TributeLevel);
	} 
}

