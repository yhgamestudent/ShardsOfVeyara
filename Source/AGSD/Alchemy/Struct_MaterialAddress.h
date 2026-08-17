#pragma once

#include "CoreMinimal.h"
#include "Struct_SlotAddress.h"
#include "Struct_MaterialAddress.generated.h"

USTRUCT(BlueprintType)
struct FStruct_MaterialAddress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	TArray<FStruct_SlotAddress> Address;
};
