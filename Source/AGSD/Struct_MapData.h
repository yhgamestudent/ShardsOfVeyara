#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Struct_MapData.generated.h"

USTRUCT(BlueprintType)
struct FStruct_MapData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText MapName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* MapImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TeleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> Level;
};
