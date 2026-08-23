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
	virtual void Deinitialize() override;

	// 세이브 및 로드에 사용될 실제 로그 데이터 구조체
	UPROPERTY(BlueprintReadWrite, Category = "GameplayLog")
	FGameplayLogData LogData;

	// 전체 로그 데이터 반환/덮어쓰기 (세이브 및 로드용)
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	FGameplayLogData GetLogData() const { return LogData; }

	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void SetLogData(const FGameplayLogData& InLogData) { LogData = InLogData; }

	// 새 게임 시작 시 호출 — 새 UUID로 세션 ID를 초기화합니다
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void InitNewSession();

	// 현재 세션 ID 조회
	UFUNCTION(BlueprintPure, Category = "GameplayLog")
	FString GetPlayerSessionID() const { return LogData.PlayerSessionID; }
	
	// 현재 맵(Stage) 이름 조회 (지정되지 않았을 시 현재 월드에서 자동 추출)
	UFUNCTION(BlueprintPure, Category = "GameplayLog")
	FString GetTargetStageName(const FString& InStageName = TEXT("")) const;

	// ==========================================
	// 로깅 헬퍼 함수들 (블루프린트 및 C++에서 호출 가능)
	// ==========================================

	// 1. 잡초 방지로 인한 작물 성장 지연 기간 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddCropGrowthDelayDueToWeeds(float DelaySeconds, const FString& StageName = TEXT(""));

	// 2. 날짜 전환 시점의 플레이어 위치 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordPlayerPositionAtDateChange(int32 Day, FVector Position);

	// 3. 실패한 물약 제조 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementFailedPotionCrafting();

	// 4. 공격 시 사용한 콤보 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordUsedCombo(const FString& ComboName, const FString& StageName = TEXT(""));

	// 5. 끝까지 진행한 콤보 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordCompletedCombo(const FString& ComboName, const FString& StageName = TEXT(""));

	// 6. 작물 별 풍요/성장 비약 사용 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordElixirUsageOnCrop(const FString& CropName, const FString& StageName = TEXT(""));

	// 7. 획득한 코인 갯수 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddAcquiredCoins(int32 Amount, const FString& StageName = TEXT(""));

	// 8. 소모한 골드량 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddConsumedCoins(int32 Amount, const FString& StageName = TEXT(""));

	// 8-1. NPC 거래로 인한 코인 소모 상세 기록 (어디서, 어떤 NPC와, 어떤 거래/아이템, 얼마)
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Economy")
	void RecordNPCTransaction(const FString& NPCName, const FString& ItemOrDetail, int32 CoinCost, int32 Quantity = 1, const FString& StageName = TEXT(""));

	// 9. 맵 별 클리어 시간 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordMapClearTime(const FString& MapName, float TimeSeconds);

	// 10. 가드로 경감한 피해량 누적
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddDamageMitigatedByGuard(float MitigatedDamage, const FString& StageName = TEXT(""));

	// 11. 게임 총 플레이 시간 누적
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void AddPlayTime(float TimeSeconds);

	// 12. 일시정지 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementPauseCount(const FString& StageName = TEXT(""));

	// 13. 아이템 획득 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementItemsAcquiredCount(const FString& StageName = TEXT(""));

	// 14. 아이템 버리기 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementItemsDiscardedCount(const FString& StageName = TEXT(""));

	// 15. 인벤토리 가득 참 발생 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void IncrementInventoryFullOccurrence(const FString& StageName = TEXT(""));

	// 16. NPC별 대화 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordNPCDialogue(const FString& NPCName, const FString& StageName = TEXT(""));

	// 17. 몬스터 종류별 처치 수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordMonsterKill(const FString& MonsterName, const FString& StageName = TEXT(""));

	// 18. 작물 종류별 수확 수량 및 수확 행동 횟수 추가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Farming")
	void RecordCropHarvest(const FString& CropName, int32 Amount, const FString& StageName = TEXT(""));

	// 작물 심기 행동 횟수 및 시각 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Farming")
	void RecordCropPlant(const FString& CropOrSeedName = TEXT(""), const FString& StageName = TEXT(""));

	// 상호작용 행동 시행 횟수 기록 (IInteraction 연동)
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Interaction")
	void RecordInteractionAction(const FString& ActionType, const FString& StageName = TEXT(""));

	// 19. 가드 사용 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Combat")
	void IncrementGuardUsageCount(const FString& StageName = TEXT(""));

	// 체력 물약 사용 횟수 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Combat")
	void RecordHealthPotionUsage(const FString& PotionName = TEXT("HealthPotion"), const FString& StageName = TEXT(""));

	// ==========================================
	// 📌 9대 가설 & 18종 차트 로깅 헬퍼 함수
	// ==========================================

	// 20. 맵별 플레이 체류 시간 누적
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Stage")
	void AddStagePlayTime(const FString& StageName, float DeltaSeconds);

	// 21. 맵별 포탈 도달 소요 시간 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Stage")
	void RecordStageClearPortalTime(const FString& StageName, float TimeSeconds);

	// 22. 맵별 플레이어 이동거리 누적
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Stage")
	void AddStageMovementDistance(const FString& StageName, float DistanceMeters);

	// 23. 맵별 + 오브젝트별 상호작용 키 입력 횟수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Stage")
	void IncrementStageInteraction(const FString& StageName, const FString& ActorName);

	// 24. 맵별 낙사/낙하 발생 및 체크포인트 리스폰 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Stage")
	void RecordStageFallRespawn(const FString& StageName, FVector FallLocation);

	// 25. 튜토리얼 스킵 및 대화 지문 넘기기 기록
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Tutorial")
	void IncrementTutorialFullSkip();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Tutorial")
	void IncrementDialogueLineSkip(const FString& StageName = TEXT(""));

	// 26. UI 옵션 변경 및 대화 로그 재확인
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|UI")
	void IncrementOptionChange(const FString& StageName = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|UI")
	void IncrementDialogueLogRecheck(const FString& StageName = TEXT(""));

	// 27. 사망 기록 (사망 원인 & 위치)
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Combat")
	void RecordPlayerDeath(const FString& StageName, const FString& DeathReason, FVector DeathLocation);

	// 28. 점프 / 대시 입력 수 증가
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Controls")
	void IncrementStageJump(const FString& StageName);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Controls")
	void IncrementStageDash(const FString& StageName);

	// 29. 기믹 파훼 및 보스 패턴 회피
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Gimmick")
	void IncrementStageGimmickClear(const FString& StageName, const FString& GimmickType = TEXT(""));
	
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Gimmick")
	void RecordGimmickStart(const FString& GimmickName, const FString& StageName = TEXT(""), float TimeSeconds = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Gimmick")
	void RecordGimmickClear(const FString& GimmickName, const FString& StageName = TEXT(""), float TimeSeconds = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Gimmick")
	void IncrementBossPatternDodge(const FString& StageName);

	// 30. 보스전 입장 / 사망 / 클리어
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Boss")
	void RecordBossEnter(const FString& BossName);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Boss")
	void RecordBossDeath(const FString& BossName);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Boss")
	void RecordBossClear(const FString& BossName, float BattleTimeSeconds, int32 LootAcquiredCount);

	// 31. 포션 소모 & 체력 잔량
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Reward")
	void RecordPotionUsage(const FString& StageName, const FString& PotionType);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Reward")
	void UpdateLatestHealthPercent(float HealthPercent);

	// 32. 봉헌 제단 이용
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Reward")
	void IncrementTributeAltarUsage(const FString& StageName = TEXT(""));

	// 33. 가해 딜량 vs 피격 딜량 누적
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Combat")
	void AddStageDamageDealt(const FString& StageName, float Damage);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Combat")
	void AddStageDamageTaken(const FString& StageName, float Damage);

	// ==========================================
	// 📌 누적 막대 그래프(Stacked Bar Chart) 정밀 로깅 함수
	// ==========================================

	// 36. 숲 맵 체크포인트 도달 및 구간별 소요 시간 & 이동거리 자동 계산
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Time")
	void RecordCheckpointReach(int32 CheckpointIndex, const FString& StageName = TEXT(""), float CustomDeltaSeconds = 0.0f, float CustomDeltaDistance = 0.0f);

	// 37. 던전 맵 기믹 방 진입 / 퇴장(클리어) 타이머 & 이동거리 측정
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Time")
	void StartGimmickRoom(const FString& RoomName, const FString& StageName = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Time")
	void EndGimmickRoom(const FString& RoomName, const FString& StageName = TEXT(""), bool bMarkAsCleared = true, float CustomDurationSeconds = 0.0f, float CustomDistanceMeters = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Time")
	void RecordGimmickRoomTime(const FString& RoomName, float DurationSeconds, const FString& StageName = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Spatial")
	void RecordGimmickRoomDistance(const FString& RoomName, float DistanceMeters, const FString& StageName = TEXT(""));

	UFUNCTION(BlueprintPure, Category = "GameplayLog|Time")
	bool IsGimmickRoomCleared(const FString& RoomName, const FString& StageName = TEXT("")) const;

	// 38. 맵별 6대 행동 소요 시간 누적 (Movement, Airborne, Combat, Interaction, PuzzleOrGimmick, UI_Pause)
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Time")
	void AddStageActionDuration(const FString& ActionCategory, float DeltaSeconds, const FString& StageName = TEXT(""));

	// 39. 성공한 물약 제작 수 & 취침 횟수 & 클리어 플래그
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Reward")
	void IncrementSuccessfulPotionCrafting(int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Farming")
	void IncrementSleepCount(const FString& StageName = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void SetGameCompleted(bool bCompleted = true);

	// ==========================================
	// 📌 데이터 내보내기 (구글 시트 / CSV 파이프라인)
	// ==========================================

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Export")
	FString GenerateCSVString() const;

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Export")
	bool ExportLogsToCSVFile(const FString& FilePath);
};
