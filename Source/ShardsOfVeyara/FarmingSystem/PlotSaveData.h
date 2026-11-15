#pragma once

#include "CoreMinimal.h"
#include "PlotSaveData.generated.h"

USTRUCT(BlueprintType)
struct FPlotSaveData
{
	GENERATED_BODY()

	// 1. 누구인가?
	UPROPERTY(BlueprintReadWrite)
	FString PlotName;
	// 2. 무엇을 심었는가? (빈 땅이면 None)
	UPROPERTY(BlueprintReadWrite)
	FName SeedName;
	//다음 성장 날짜
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	int32 ScheduledDay = 0;
	//잡초로 인한 누적 지연 날짜
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	int32 DelayedDays = 0;
	//현재 단계
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	int32 CurrentGrowStageIndex = 0;
	//최종 단계
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	int32 FinishGrowStageIndex = 0;
	//다음 단계까지 남은 시간
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	int32 GrowthTimeCounter = 0;
	//작물이 다 자랐는지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	bool FullyGrown = false;
	//잡초가 자랐는지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	bool bHasWeeds = false;

	// 추가 보너스 수확량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	int32 BonusYield = 0;
};
