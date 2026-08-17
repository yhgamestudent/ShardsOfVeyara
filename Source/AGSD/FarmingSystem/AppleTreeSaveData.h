#pragma once

#include "CoreMinimal.h"
#include "AppleTreeSaveData.generated.h"

USTRUCT(BlueprintType)
struct FAppleTreeSaveData
{
	GENERATED_BODY()

	// 나무 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TreeName;

	// 사과가 달렸는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasApple = false;

	// 마지막으로 확인/갱신된 날짜
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LastCheckedDay = 1;
};
