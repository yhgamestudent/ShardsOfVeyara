// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PotionDataTable.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FPotionData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PotionData")
	FString ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PotionData")
	FString IngredientA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PotionData")
	FString IngredientB;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PotionData")
	FLinearColor LiquidColor;

	FPotionData()
	{
		LiquidColor = FColor::FromHex("A5DDFFFF");
	}
};