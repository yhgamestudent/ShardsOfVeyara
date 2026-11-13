#pragma once

#include "CoreMinimal.h"
#include "Struct_InventorySlotData.h"
#include "Struct_WorldChestData.generated.h"

USTRUCT(BlueprintType)
struct FStruct_WorldChestData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// ItemID: 스트링 -> FString
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest Data")
	TArray<FStruct_InventorySlotData> ChestSlots;
};
