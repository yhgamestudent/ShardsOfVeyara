// Fill out your copyright notice in the Description page of Project Settings.


#include "AlchemyTable.h"
#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "AGSDPlayerController.h"
#include "PotionDataTable.h"
#include "Components/Button.h"
// #include "../../../../../../../Program Files/Epic Games/UE_5.6/Engine/Plugins/FX/Niagara/Source/Niagara/Public/NiagaraComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/UI/AGSDPlayerHUD.h"

#if WITH_EDITOR
#include "Commandlets/WorldPartitionCommandletHelpers.h"
#endif

// Sets default values
AAlchemyTable::AAlchemyTable()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;

	//콜리전 박스 설정
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AAlchemyTable::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AAlchemyTable::OnEndOverlap);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComp->SetupAttachment(RootComponent);

	PotMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pot"));
	PotMeshComponent->SetupAttachment(RootComponent);
	
	SplashVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SplashVFX"));
	SplashVFXComponent->SetupAttachment(PotMeshComponent);
	SplashVFXComponent->bAutoActivate = false;

	BubbleVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BubbleVFX"));
	BubbleVFXComponent->SetupAttachment(PotMeshComponent);
	BubbleVFXComponent->bAutoActivate = true;
}

// Called when the game starts or when spawned
void AAlchemyTable::BeginPlay()
{
	Super::BeginPlay();

	GI = Cast<USOVGameInstance>(GetGameInstance());
	
	if (PotMeshComponent)
	{
		// 1번 인덱스의 머티리얼을 가져와 다이나믹 인스턴스로 생성합니다.
		// CreateDynamicMaterialInstance(인덱스, 소스 머티리얼, 이름)
		PotDynamicMaterial = PotMeshComponent->CreateDynamicMaterialInstance(1);
	}
	
	if (AlchemyDataTable)
	{
		static const FString ContextString(TEXT("Alchemy Recipe Context"));
		AlchemyDataTable->GetAllRows<FPotionData>(ContextString, AlchemyRecipes);
	}
	
	FAlchemySaveData ItemIDData;
	if (GI->GetAlchemyData(GetName(), ItemIDData))
	{
		InsertedItemID = ItemIDData.ItemID;

		UpdateTargetRecipe();
	}
}

void AAlchemyTable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	FAlchemySaveData SaveData;
	SaveData.TableName = GetName();
	SaveData.ItemID = InsertedItemID;

	if (GI)
	{
		GI->UpdateTableData(SaveData);
	}
}

void AAlchemyTable::ApplyPotColor(FLinearColor NewColor)
{
	LiquidColor = NewColor; // 현재 색상 업데이트

	if (PotDynamicMaterial)
		PotDynamicMaterial->SetVectorParameterValue(TEXT("Color"), NewColor);

	if (SplashVFXComponent)
		SplashVFXComponent->SetVariableLinearColor(TEXT("User.Color"), NewColor);

	if (BubbleVFXComponent)
		BubbleVFXComponent->SetVariableLinearColor(TEXT("User.Color"), NewColor);
}

void AAlchemyTable::EndAlchemy()
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
		if (Player->PlayerHUDRef)
		{
			Player->PlayerHUDRef->SetVisibility(ESlateVisibility::Visible);
		}
	}
	bCanUseAlchemyTable = true; // 다시 상호작용 가능하게

	// 6. 위젯 제거
	if (AlchemyWidget)
	{
		AlchemyWidget->getCloseButton()->OnClicked.RemoveDynamic(this, &AAlchemyTable::EndAlchemy);
		AlchemyWidget->getEmptyButton()->OnClicked.RemoveDynamic(this, &AAlchemyTable::EmptyPot);
		AlchemyWidget->RemoveFromParent();
		// 메모리 관리를 위해 필요하다면 AlchemyWidget = nullptr; 를 해줄 수도 있지만, 
		// 다시 열 때를 대비해 유지하는 것이 일반적입니다.
	}
}

void AAlchemyTable::SplashPot(bool clear)
{
	SplashVFXComponent->Activate(true);
	
	if (SplashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SplashSound, GetActorLocation());
	}

	UpdateTargetRecipe();
	if (clear)
	{
		TargetRecipe.LiquidColor = BaseLiquidColor;
		TransmutationComplete();
	}
}

void AAlchemyTable::Interact_Implementation(AAGSDCharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
	{
		Player = player;
		Player->Mining = true;
		bCanUseAlchemyTable = false;
		
		if (Player->PlayerHUDRef)
		{
			Player->PlayerHUDRef->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		PlayerController->SetViewTargetWithBlend(this, BlendTime);

		// 위젯이 아직 없다면 여기서 생성
		if (!AlchemyWidget && AlchemyWidgetClass)
		{
			AlchemyWidget = CreateWidget<UAlchemyUI>(PlayerController, AlchemyWidgetClass);
		}
		if (AlchemyWidget)
		{
			AlchemyWidget->setOwnerActor(this);

			// 중복 바인딩 방지: 추가 전에 항상 먼저 제거
			AlchemyWidget->getCloseButton()->OnClicked.RemoveDynamic(this, &AAlchemyTable::EndAlchemy);
			AlchemyWidget->getCloseButton()->OnClicked.AddDynamic(this, &AAlchemyTable::EndAlchemy);
			
			if (UButton* EmptyButton = AlchemyWidget->getEmptyButton())
			{
				EmptyButton->OnClicked.RemoveDynamic(this, &AAlchemyTable::EmptyPot);
				EmptyButton->OnClicked.AddDynamic(this, &AAlchemyTable::EmptyPot);
			}
			AlchemyWidget->AddToViewport();

			AlchemyWidget->PlayFadeIn();
			
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(AlchemyWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
			PlayerController->bShowMouseCursor = true;
		}
		//FTimerHandle TimerHandle;
		//GetWorldTimerManager().SetTimer(TimerHandle, this, &AAlchemyTable::OnCameraBlendFinished, BlendTime, false);
	}
}

void AAlchemyTable::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool AAlchemyTable::CanInteract_Implementation(AAGSDCharacter* player)
{
	return bCanUseAlchemyTable;
}

void AAlchemyTable::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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

void AAlchemyTable::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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

void AAlchemyTable::UpdateTargetRecipe()
{
	if (InsertedItemID.Num() == 2)
	{
		TargetRecipe.ItemID = TEXT("Sludge");
		TargetRecipe.LiquidColor = FLinearColor(0.1f, 0.05f, 0.1f, 1.0f); // 어두운 보라색 (찌꺼기 색상)

		TArray<FString> TempItemID = InsertedItemID;
		TempItemID.Sort();
		
		for (FPotionData* Recipe : AlchemyRecipes)
		{
			TArray<FString> RecipeMaterial = {Recipe->IngredientA, Recipe->IngredientB};
			RecipeMaterial.Sort();
			if (RecipeMaterial == TempItemID)
			{
				TargetRecipe = *Recipe;
				break;
			}
		}
		LerpMixLiquidColor();
	}
}

