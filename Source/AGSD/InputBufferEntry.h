#pragma once

#include "CoreMinimal.h"
#include "InputBufferEntry.generated.h"

USTRUCT(BlueprintType)
struct FInputBufferEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Buffer")
	FName InputName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Buffer")
	float TimeStamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Buffer")
	int32 FrameCount;

	FInputBufferEntry() 
		: TimeStamp(0.0f)
		, FrameCount(0)
	{}

	FInputBufferEntry(FName Name, float Time, int32 Frame)
		: InputName(Name)
		, TimeStamp(Time)
		, FrameCount(Frame)
	{}
};
