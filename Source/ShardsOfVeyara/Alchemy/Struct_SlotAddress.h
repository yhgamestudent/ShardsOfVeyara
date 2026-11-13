#pragma once

#include "CoreMinimal.h"
#include "Struct_SlotAddress.generated.h"

USTRUCT(BlueprintType)
struct FStruct_SlotAddress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	int32 Index = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	int32 Amount = 0;
};
