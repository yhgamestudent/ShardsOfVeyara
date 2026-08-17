// Fill out your copyright notice in the Description page of Project Settings.


#include "ACultivationPlot.h"
#include "Components/BoxComponent.h"
#include "AGSDPlayerController.h"
#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "UsableItem.h"
#include "SeedDataTable.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TextLog.h"

// Sets default values
AACultivationPlot::AACultivationPlot()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	//루트 컴포넌트 설정
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	//콜리전 박스 설정
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetBoxExtent(FVector(50.0f, 50.f, 50.f));
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AACultivationPlot::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AACultivationPlot::OnEndOverlap);

	// 나이아가라 컴포넌트 설정
	FertilizerEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FertilizerEffectComponent"));
	FertilizerEffectComponent->SetupAttachment(RootComponent);
	FertilizerEffectComponent->bAutoActivate = false;
}

void AACultivationPlot::HandleDayPassed(int32 CurrentDay)
{
	if (PlantedCrop)
	{
		if (bHasWeeds)
		{
			ScheduledDay++;
			DelayedDays++;
			FString LogMsg = FString::Printf(TEXT("%s (누적 %d일 지연)"), *CropData->CropName.ToString(), DelayedDays);
			UTextLog::WriteTextLogByString(TEXT("성장 지연"), LogMsg);
		}
		else if (ScheduledDay <= CurrentDay) AdvanceGrowth();
	}
	if (!bHasWeeds && FMath::FRand() <= weedsProb)
	{
		bHasWeeds = true;
		SpawnWeeds();
	}
}

//오버랩 시작 함수 구현부
void AACultivationPlot::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

void AACultivationPlot::ShowWidget_Implementation(ACharacter* player)
{
	AAGSDCharacter* GSDCharacter = Cast<AAGSDCharacter>(player);
	if (!GSDCharacter) return;

	FText DynamicText = InteractActionText;

	if (PlantedCrop == nullptr)
	{
		DynamicText = FText::FromString(TEXT("작물심기"));
	}
	else if (!FullyGrown)
	{
		FStruct_ItemData HoldingItem = GSDCharacter->GetHoldingItemData();
		if (HoldingItem.ItemBPClass)
		{
			AActor* DefaultActor = Cast<AActor>(HoldingItem.ItemBPClass->GetDefaultObject());
			if (DefaultActor && DefaultActor->Implements<UUsableItem>())
			{
				DynamicText = FText::FromString(TEXT("물약사용"));
			}
		}
	}

	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
	{
		PlayerController->ShowInteractionWidget(DynamicText);
	}
}

bool AACultivationPlot::CanInteract_Implementation(AAGSDCharacter* player)
{
	// 1. 작물이 심겨져 있지 않을 때: 씨앗을 들고 있으면 상호작용 가능
	if (PlantedCrop == nullptr)
	{
		return player->HoldingState == EHoldingState::EHS_Seed;
	}

	// 2. 작물이 심겨져 있고 아직 다 자라지 않았을 때: 풍요나 성장 물약을 들고 있으면 상호작용 가능
	if (!FullyGrown)
	{
		FString ItemIDStr = player->GetHoldingItemData().ItemID;
		if (ItemIDStr.Contains(TEXT("Growth")) || ItemIDStr.Contains(TEXT("Rich")))
		{
			return true;
		}
	}

	return false;
}

//오버랩 종료 함수 구현부
void AACultivationPlot::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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

void AACultivationPlot::Interact_Implementation(AAGSDCharacter* player)
{
	if (!SeedDataTable) return;
	UE_LOG(LogTemp, Warning, TEXT("AACultivationPlot::OnBeginOverlap"));
	
	// 1. 작물이 심겨져 있지 않고, 플레이어가 씨앗을 들고 있는 경우 -> 기존의 씨앗 심기 동작
	if (PlantedCrop == nullptr && player->HoldingState == EHoldingState::EHS_Seed)
	{
		SeedName = FName(*player->SubItemAmount());
		
		GetSeedInfo(SeedName);
		
		CurrentGrowStageIndex = 0;
		GrowthTimeCounter = CropData->GrowthStages[CurrentGrowStageIndex].TimeToGrow;
		FinishGrowStageIndex = CropData->GrowthStages.Num() - 1;
		DelayedDays = 0;
		
		int32 currentDay = GS->GetCurrentDay();
		ScheduledDay =  currentDay + GrowthTimeCounter;
		
		PlantCrop();
		PlantedCrop->MeshUpdate(CurrentGrowStageIndex);
		return;
	}

	// 2. 작물이 심겨져 있고 아직 다 자라지 않았으며, 풍요나 성장 물약을 들고 있는 경우 -> 물약의 UseItem 실행
	if (PlantedCrop && !FullyGrown)
	{
		FStruct_ItemData HoldingItem = player->GetHoldingItemData();
		if (HoldingItem.ItemBPClass)
		{
			AActor* DefaultActor = Cast<AActor>(HoldingItem.ItemBPClass->GetDefaultObject());
			if (DefaultActor && DefaultActor->Implements<UUsableItem>())
			{
				IUsableItem::Execute_UseItem(DefaultActor, player);
				return;
			}
		}
	}
}

// Called when the game starts or when spawned
void AACultivationPlot::BeginPlay()
{
	Super::BeginPlay();
	GI = Cast<USOVGameInstance>(GetGameInstance());
	GS = Cast<AAGSDGameStateBase>(UGameplayStatics::GetGameState(GetWorld()));

	if (AAGSDGameStateBase* GameState = Cast<AAGSDGameStateBase>(UGameplayStatics::GetGameState(GetWorld())))
	{
		GameState->OnDayChangedDelegate.AddUObject(this, &AACultivationPlot::HandleDayPassed);
	}
	if (!GI) return;
	
	FPlotSaveData LoadedData;
	// 아까 만든 구조체 데이터를 통째로 넘겨서 저장합니다.
	if (!GI->GetPlotData(GetName(), LoadedData)) return;

	SeedName = LoadedData.SeedName;
	CurrentGrowStageIndex = LoadedData.CurrentGrowStageIndex;
	GrowthTimeCounter = LoadedData.GrowthTimeCounter;
	FinishGrowStageIndex = LoadedData.FinishGrowStageIndex;
	FullyGrown = LoadedData.FullyGrown;
	ScheduledDay = LoadedData.ScheduledDay;
	DelayedDays = LoadedData.DelayedDays;
	bHasWeeds = LoadedData.bHasWeeds;
	BonusYield = LoadedData.BonusYield;

	
	//CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (bHasWeeds) SpawnWeeds();
	if (SeedName == NAME_None) return;
	
	GetSeedInfo(SeedName);
	PlantCrop();
	
	int32 CurrentDay = GI->CurrentDay;
	while (ScheduledDay <= CurrentDay && !FullyGrown && !bHasWeeds)
	{
		GrowthLogic();
	}
	
	PlantedCrop->MeshUpdate(CurrentGrowStageIndex);

	if (CurrentGrowStageIndex >= FinishGrowStageIndex)
	{
		FullyGrown = true;
		PlantedCrop->SetCollisionEnable();
	}

	UpdateFertilizerEffect();
}

void AACultivationPlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	FPlotSaveData SaveData;
	SaveData.PlotName = GetName();
	SaveData.SeedName = SeedName;
	SaveData.CurrentGrowStageIndex = CurrentGrowStageIndex;
	SaveData.GrowthTimeCounter = GrowthTimeCounter;
	SaveData.FinishGrowStageIndex = FinishGrowStageIndex;
	SaveData.FullyGrown = FullyGrown;
	SaveData.ScheduledDay = ScheduledDay;
	SaveData.DelayedDays = DelayedDays;
	SaveData.bHasWeeds = bHasWeeds;
	SaveData.BonusYield = BonusYield;

	if (GI)
	{
		// 아까 만든 구조체 데이터를 통째로 넘겨서 저장합니다.
		GI->UpdatePlotData(SaveData);
	}
}

void AACultivationPlot::PlantCrop()
{
	// ★ 1. 에디터에서 작물 블루프린트(CropClassToPlant)를 설정했는지 확인
	if (!CropClassToPlant)
	{
		UE_LOG(LogTemp, Error, TEXT("'%s'에 CropClassToPlant가 설정되지 않았습니다!"), *GetName());
		return;
	}
	
	FTransform SpawnTransform = GetTransform();

	PlantedCrop = GetWorld()->SpawnActorDeferred<ACrop>(
	CropClassToPlant,
	SpawnTransform,
	this,
	nullptr,
	ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	if (PlantedCrop)
	{
		PlantedCrop->SetCropData(CropData);
		PlantedCrop->SetBonusYield(BonusYield);

		PlantedCrop->OnHarvested.AddDynamic(this, &AACultivationPlot::OnPlantedCropDestroyed);		
		UGameplayStatics::FinishSpawningActor(PlantedCrop, SpawnTransform);

		UpdateFertilizerEffect();
	}
}

void AACultivationPlot::OnPlantedCropDestroyed()
{
	PlantedCrop = nullptr;
	
	CurrentGrowStageIndex = 0;
	FullyGrown = false;
	ScheduledDay = 0;
	DelayedDays = 0;
	SeedName = NAME_None;
	BonusYield = 0;

	/*
	if (CollisionBox)
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	*/

	UpdateFertilizerEffect();
}

void AACultivationPlot::GetSeedInfo(FName TargetRowName)
{
	if (!SeedDataTable) return;
	
	static const FString Context(TEXT("Seed Data Context"));

	FSeedData* SeedRow = SeedDataTable->FindRow<FSeedData>(TargetRowName, Context);
	if (SeedRow)
	{
		CropData = SeedRow->CropData.Get();

		if (!CropData) CropData = SeedRow->CropData.LoadSynchronous();
	}
}

void AACultivationPlot::OnWeedRemoved()
{
	bHasWeeds = false;
}

void AACultivationPlot::GrowthLogic() {
	if (CurrentGrowStageIndex < FinishGrowStageIndex)
	{
		CurrentGrowStageIndex++;
		GrowthTimeCounter = CropData->GrowthStages[CurrentGrowStageIndex].TimeToGrow;
		
		ScheduledDay += GrowthTimeCounter;
	}
	if (CurrentGrowStageIndex >= FinishGrowStageIndex)
	{
		FullyGrown = true;
		PlantedCrop->SetCollisionEnable();
	}
}

void AACultivationPlot::SpawnWeeds()
{
	if (!WeedsActorClass) return;

	FTransform SpawnTransform = GetTransform();
	
	WeedsActor = GetWorld()->SpawnActorDeferred<AWeeds>(
	WeedsActorClass,
	SpawnTransform,
	this,
	nullptr,
	ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	UGameplayStatics::FinishSpawningActor(WeedsActor, SpawnTransform);
	WeedsActor->OnWeeding.AddDynamic(this, &AACultivationPlot::OnWeedRemoved);
}

void AACultivationPlot::AdvanceGrowth()
{
	GrowthLogic();
	PlantedCrop->MeshUpdate(CurrentGrowStageIndex);
}

void AACultivationPlot::ApplyFertilizer(int32 Amount)
{
	BonusYield = Amount;
	if (PlantedCrop)
	{
		PlantedCrop->SetBonusYield(BonusYield);
	}
	UpdateFertilizerEffect();
}

void AACultivationPlot::ApplyGrowthElixir()
{
	if (PlantedCrop && !FullyGrown && !bHasWeeds)
	{
		while (!FullyGrown)
		{
			AdvanceGrowth();
		}
	}
}

void AACultivationPlot::UpdateFertilizerEffect()
{
	if (FertilizerEffectComponent)
	{
		if (PlantedCrop != nullptr && BonusYield > 0)
		{
			if (FertilizerEffectSystem && FertilizerEffectComponent->GetAsset() != FertilizerEffectSystem)
			{
				FertilizerEffectComponent->SetAsset(FertilizerEffectSystem);
			}

			if (!FertilizerEffectComponent->IsActive())
			{
				FertilizerEffectComponent->Activate(true);
			}
		}
		else
		{
			if (FertilizerEffectComponent->IsActive())
			{
				FertilizerEffectComponent->Deactivate();
			}
		}
	}
}

void AACultivationPlot::SpawnGrowthEffect(UNiagaraSystem* EffectSystem)
{
	if (EffectSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), EffectSystem, GetActorLocation());
	}
}

void AACultivationPlot::SetSeedInfo(FName InSeedName, UDataTable* InSeedDataTable)
{
	if (InSeedDataTable)
	{
		SeedDataTable = InSeedDataTable;
	}
	SeedName = InSeedName;
}


