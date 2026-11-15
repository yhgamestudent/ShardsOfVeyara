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

class SHARDSOFVEYARA_API AFarmingGameMode : public AGameModeBase

{

	GENERATED_BODY()



public:

	AFarmingGameMode();



	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	

	void NextDay(float time);

	

protected:

	/** 하루의 실제 현실 시간 길이 (초 단위, 기본값: 500.0f = 8분 20초) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "10.0"))

	float DayLengthInSeconds = 500.0f;



	UPROPERTY(EditDefaultsOnly, Category = "Time")

	float TimeScale = 4.8f;



	UPROPERTY(EditAnywhere, Category = "Time")

	float NextDayStartTime = 600.f;



	const float GameHoursPerDay = 2400.f;

	

	class AAGSDGameStateBase* GameState = nullptr;



	void HandleTimeIncrement(float time);



	bool Reset = false;

private:

	FTimerHandle TimeUpdateTimerHandle;

};

