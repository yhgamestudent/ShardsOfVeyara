// Fill out your copyright notice in the Description page of Project Settings.


#include "CropManager.h"
#include "AGSDGameStateBase.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACropManager::ACropManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void ACropManager::RegisterCrop(AACultivationPlot* plot, int32 NextAdvanceDay)
{
	if (plot == nullptr || NextAdvanceDay <= 0) return;
	
	FCropArrayWrapper& DaySchedule = ResisterCropArray.FindOrAdd(NextAdvanceDay);

	if (DaySchedule.CropsToAdvance.Contains(plot))	return;
	
	DaySchedule.CropsToAdvance.Add(plot);
}

void ACropManager::HandleDayPassed(int32 CurrentDay)
{
	if (CurrentDay <= 0) return;

	if (FCropArrayWrapper* DaySchedule = ResisterCropArray.Find(CurrentDay))
	{
		for (AACultivationPlot* plot : DaySchedule->CropsToAdvance)
		{
			if (plot == nullptr) continue;
			plot->AdvanceGrowth();
			if (!plot->GetFullyGrown())
				RegisterCrop(plot, plot->GetScheduledDay());
		}
	}
	ResisterCropArray.Remove(CurrentDay);
}

void ACropManager::UnregisterCrop(AACultivationPlot* plot, int32 ScheduledDay)
{
	if (plot == nullptr || ScheduledDay <= 0) return;
	
	if(FCropArrayWrapper* DaySchedule = ResisterCropArray.Find(ScheduledDay))
	{
		DaySchedule->CropsToAdvance.RemoveSingleSwap(plot);

		if (DaySchedule->CropsToAdvance.Num() == 0) ResisterCropArray.Remove(ScheduledDay);
	}
}

// Called when the game starts or when spawned
void ACropManager::BeginPlay()
{
	Super::BeginPlay();

	if (AAGSDGameStateBase* GameState = Cast<AAGSDGameStateBase>(UGameplayStatics::GetGameState(GetWorld())))
	{
		GameState->OnDayChangedDelegate.AddUObject(this, &ACropManager::HandleDayPassed);
	}
}

