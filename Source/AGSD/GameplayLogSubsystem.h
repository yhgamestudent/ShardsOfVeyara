#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Struct_GameplayLogData.h"
#include "GameplayLogSubsystem.generated.h"

/**
 * 게임 플레이 중 발생하는 각종 이벤트 로그를 중앙에서 관리하는 서브시스템입니다.
 */
UCLASS()
class AGSD_API UGameplayLogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 세이브 및 로드에 사용될 실제 로그 데이터 구조체
	UPROPERTY(BlueprintReadWrite, Category = "GameplayLog")
	FGameplayLogData LogData;

	// 전체 로그 데이터 반환/덮어쓰기 (세이브 및 로드용)
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	FGameplayLogData GetLogData() const { return LogData; }

	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void SetLogData(const FGameplayLogData& InLogData) { LogData = InLogData; }

	// ==========================================
	// 로깅 헬퍼 함수들 (블루프린트에서 쉽게 호출 가능)
	// ==========================================

	// 1. 잡초 방지로 인한 작물 성장 지연 기간 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddCropGrowthDelayDueToWeeds(float DelaySeconds);

	// 2. 날짜 전환 시점의 플레이어 위치 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordPlayerPositionAtDateChange(int32 Day, FVector Position);

	// 3. 실패한 물약 제조 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementFailedPotionCrafting();

	// 4. 공격 시 사용한 콤보 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordUsedCombo(const FString& ComboName);

	// 5. 끝까지 진행한 콤보 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordCompletedCombo(const FString& ComboName);

	// 6. 작물 별 풍요/성장 비약 사용 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordElixirUsageOnCrop(const FString& CropName);

	// 7. 획득한 코인 갯수 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddAcquiredCoins(int32 Amount);

	// 8. 소모한 골드량 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddConsumedCoins(int32 Amount);

	// 9. 맵 별 클리어 시간 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordMapClearTime(const FString& MapName, float TimeSeconds);

	// 10. 가드로 경감한 피해량 누적
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddDamageMitigatedByGuard(float MitigatedDamage);

	// 11. 게임 총 플레이 시간 누적
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddPlayTime(float TimeSeconds);

	// 12. 일시정지 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementPauseCount();

	// 13. 아이템 획득 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementItemsAcquiredCount();

	// 14. 아이템 버리기 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementItemsDiscardedCount();

	// 15. 인벤토리 가득 참 발생 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementInventoryFullOccurrence();

	// 16. NPC별 대화 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordNPCDialogue(const FString& NPCName);

	// 17. 몬스터 종류별 처치 수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordMonsterKill(const FString& MonsterName);

	// 18. 작물 종류별 수확 수량 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordCropHarvest(const FString& CropName, int32 Amount);

	// 19. 가드 사용 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementGuardUsageCount();
};
