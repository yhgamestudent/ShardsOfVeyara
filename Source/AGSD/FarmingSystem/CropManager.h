// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACultivationPlot.h"
#include "CropManager.generated.h"

USTRUCT(BlueprintType)
struct FCropArrayWrapper
{
	GENERATED_BODY()

	// TMap의 값으로 사용하기 위해 구조체 내부에 TArray를 선언합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	TArray<class AACultivationPlot*> CropsToAdvance;
};

UCLASS()
class AGSD_API ACropManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACropManager();

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void RegisterCrop(AACultivationPlot* plot, int32 NextAdvanceDay);

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void HandleDayPassed(int32 CurrentDay);
	
	void UnregisterCrop(AACultivationPlot* plot, int32 ScheduledDay);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	TMap<int32, FCropArrayWrapper> ResisterCropArray;
};
