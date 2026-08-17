// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AGSDGameStateBase.h"
#include "FarmingGameMode.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API AFarmingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFarmingGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	void NextDay(float time);
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Time")
	float TimeScale = 48.f;

	UPROPERTY(EditAnywhere, Category = "Time")
	float NextDayStartTime = 600.f;

	const float GameHoursPerDay = 2400.f;
	
	class AAGSDGameStateBase* GameState = nullptr;

	void HandleTimeIncrement(float time);

	bool Reset = false;
private:
	FTimerHandle TimeUpdateTimerHandle;
};
