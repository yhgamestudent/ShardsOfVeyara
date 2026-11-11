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

	// 하루 총 게임 시간(GameHoursPerDay=2400)을 DayLengthInSeconds(기본 500초 = 8분 20초) 동안 균일하게 진행
	const float EffectiveTimeScale = (DayLengthInSeconds > 0.0f) ? (GameHoursPerDay / DayLengthInSeconds) : TimeScale;
	HandleTimeIncrement(DeltaSeconds * EffectiveTimeScale);
}

void AFarmingGameMode::NextDay(float time)
{
	if (!GameState) return;
	GameState->SetCurrentTime(time);
	Reset = false;
	GameState->AdvanceDay();
	GameState->OnSecondChangedDelegate.Broadcast(time);
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


