#pragma once

#include "CoreMinimal.h"
#include "Struct_GameplayLogData.generated.h"

USTRUCT(BlueprintType)
struct FGameplayLogData
{
	GENERATED_BODY()

	// 1. 잡초 방지로 인한 작물 성장 지연 기간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	float TotalCropGrowthDelayDueToWeeds = 0.f;

	// 2. 날짜 전환 시점의 플레이어 위치 (Day -> Position)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<int32, FVector> PlayerPositionsAtDateChange;

	// 3. 실패한 물약 제조 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 FailedPotionCraftingCount = 0;

	// 4. 공격 시 사용한 콤보 별 횟수 (ComboName -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<FString, int32> UsedComboCounts;

	// 5. 끝까지 진행한 콤보 별 횟수 (ComboName -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<FString, int32> CompletedComboCounts;

	// 6. 작물 별 풍요/성장 비약 사용 횟수 (CropName -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<FString, int32> ElixirUsagePerCrop;

	// 7. 획득한 코인 갯수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 TotalAcquiredCoins = 0;

	// 8. 소모한 골드량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 TotalConsumedCoins = 0;

	// 9. 맵 별 처음부터 클리어까지 걸린 시간 (MapName -> Seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<FString, float> MapClearTimes;

	// 10. 가드로 경감한 피해량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	float TotalDamageMitigatedByGuard = 0.f;

	// 11. 게임 총 플레이 시간 (Seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	float TotalPlayTime = 0.f;

	// 12. 일시정지 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 PauseCount = 0;

	// 13. 아이템 획득 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 TotalItemsAcquired = 0;

	// 14. 아이템 버리기 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 TotalItemsDiscarded = 0;

	// 15. 인벤토리 가득 참 발생 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 InventoryFullOccurrenceCount = 0;

	// 16. NPC별 대화 횟수 (NPC Name -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<FString, int32> NPCDialogueCounts;

	// 17. 몬스터 종류별 처치 수 (Monster Name -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<FString, int32> MonsterKillCounts;

	// 18. 작물 종류별 수확 수량 (Crop Name -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	TMap<FString, int32> CropHarvestCounts;

	// 19. 가드 사용 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 GuardUsageCount = 0;

    // 플레이어 사망원인은 제외됨
};
