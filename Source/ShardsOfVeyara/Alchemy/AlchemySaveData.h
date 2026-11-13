#pragma once

#include "CoreMinimal.h"
#include "AlchemySaveData.generated.h"

USTRUCT(BlueprintType)
struct FAlchemySaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString TableName;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FString> ItemID;
};