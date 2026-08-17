// Fill out your copyright notice in the Description page of Project Settings.


#include "FarmingGameMode.h"
#include "AGSDPlayerController.h"
#include "Kismet/GameplayStatics.h"


AFarmingGameMode::AFarmingGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	GameStateClass = AAGSDGameStateBase::StaticClass();
}

void AFarmingGameMode::BeginPlay()
{
	Super::BeginPlay();
	GameState = GetGameState<AAGSDGameStateBase>();
}

void AFarmingGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 원래 시간 로직
	//HandleTimeIncrement(DeltaSeconds * TimeScale);
	// x4 용 시간 로직
	HandleTimeIncrement(DeltaSeconds * TimeScale * 0.1f);
}

void AFarmingGameMode::NextDay(float time)
{
	GameState->SetCurrentTime(time);
	Reset = false;
	GameState->AdvanceDay();
}

void AFarmingGameMode::HandleTimeIncrement(float time)
{
	if (!GameState) return;
	GameState->AddCurrentTime(time);

	if (GameState->GetCurrentTime() >= GameHoursPerDay)
	{
		GameState->InitCurrentTime();
		Reset = true;
	}
	if (GameState->GetCurrentTime() > NextDayStartTime && Reset)
	{
		NextDay(GameState->GetCurrentTime());
	}
	GameState->OnSecondChangedDelegate.Broadcast(GameState->GetCurrentTime());
}


