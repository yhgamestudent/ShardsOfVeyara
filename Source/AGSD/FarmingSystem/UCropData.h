// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PickUpItem.h"
#include "UCropData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FHarvestItem
{
	GENERATED_BODY()
	//수확 시 획득할 아이템(임시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
	FText ItemID;

	//수확 시 획득할 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
	int32 Quantity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
	TSubclassOf<APickUpItem> Harvest;
};

USTRUCT(BlueprintType)
struct FGrowthStageData
{
	GENERATED_BODY()
	//해당 단계의 시각적 메시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	class UStaticMesh* Mesh;

	//작물이 다음 단계로 진행되는 날짜
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	int32 TimeToGrow;
};

USTRUCT(BlueprintType)
struct FHarvestedStageData
{
	GENERATED_BODY()
	//해당 단계의 시각적 메시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	class UStaticMesh* Mesh;

	//작물이 다음 단계로 진행되는 날짜
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	int32 TimeToGrow;
};

UCLASS()
class AGSD_API UUCropData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//작물 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	FText CropName;
	
	//작물이 자라는 정보를 담을 배열
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	TArray<FGrowthStageData> GrowthStages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	TArray<FGrowthStageData> HarvestedStages;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
	TArray<FHarvestItem> HarvestRewards;
};
