// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UCropData.h"
#include "SeedDataTable.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FSeedData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SeedData")
	TSoftObjectPtr<UUCropData> CropData;
};
