// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AGSDGameStateBase.generated.h"

/**
 * 
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDayChanged, int32);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSecondChanged, float, time);

UCLASS()
class AGSD_API AAGSDGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	FOnDayChanged OnDayChangedDelegate;
	FOnSecondChanged OnSecondChangedDelegate;
	
	void AdvanceDay();

	FORCEINLINE int32 GetCurrentDay() const { return CurrentDay; }
	FORCEINLINE float GetCurrentTime() const { return CurrentTime; }
	FORCEINLINE void SetCurrentTime(float time) { CurrentTime = time; }
	FORCEINLINE void InitCurrentTime() { CurrentTime = 0.f; }
	FORCEINLINE void AddCurrentTime(float Time) { CurrentTime += Time; }
	
	UFUNCTION()
	void OnRep_CurrentDay();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 CurrentDay = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float CurrentTime = 360.f;
};
