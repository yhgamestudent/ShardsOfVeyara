#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Struct_ItemData.h"
#include "Struct_InventorySlotData.generated.h"

USTRUCT(BlueprintType)
struct FStruct_InventorySlotData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Data")
	bool IsEmpty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Data")
	FStruct_ItemData ItemData;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Data")
	int32 SlotIndex;
	
	FStruct_InventorySlotData()
	{
		IsEmpty = true;
		ItemData = FStruct_ItemData();
		SlotIndex = 0;
	}
};