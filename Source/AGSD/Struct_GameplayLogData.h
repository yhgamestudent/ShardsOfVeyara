#pragma once

#include "CoreMinimal.h"
#include "Struct_GameplayLogData.generated.h"

// 📌 23번 중첩 TMap 해결을 위한 래핑 구조체
USTRUCT(BlueprintType)
struct FInteractionCountMap
{
	GENERATED_BODY()

	// 오브젝트(Actor) 이름 -> 상호작용 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, int32> ActorCounts;
};

// 📌 NPC 코인 거래 정밀 기록 구조체
USTRUCT(BlueprintType)
struct FNPCTransactionRecord
{
	GENERATED_BODY()

	// 거래가 이루어진 맵 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	FString StageName;

	// 거래 대상 NPC 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	FString NPCName;

	// 구매/거래한 아이템 이름 또는 상세 내용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	FString ItemOrDetail;

	// 소모한 총 코인량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	int32 Cost = 0;

	// 거래 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	int32 Quantity = 1;
};
USTRUCT(BlueprintType)
struct FGameplayLogData
{
	GENERATED_BODY()

	// 플레이어 세션 고유 ID (새 게임 시작 시 UUID로 생성되어 세이브 파일과 함께 유지됩니다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	FString PlayerSessionID;

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

	// 게임 세부 총 플레이 시간 (탐험, 보스전, 기타/거점)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	float TotalExplorationTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	float TotalBossTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	float TotalEtcTime = 0.f;

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

	// 수확 행동 시행 횟수 (총 상호작용 횟수)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	int32 TotalCropHarvestActionCount = 0;

	// 작물 심기 시행 횟수 및 최초 심기 시각
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	int32 TotalCropPlantCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	float FirstCropPlantTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	TArray<float> CropPlantTimestamps;

	// 19. 가드 사용 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	int32 GuardUsageCount = 0;

	// 체력 물약 사용 횟수 (Total 및 맵별)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	int32 TotalHealthPotionUsageCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> StageHealthPotionUsageCounts;

	// ==========================================
	// 📌 9대 가설 & 18종 차트 검증용 추가 수집 항목
	// ==========================================

	// 20. 맵별 플레이 체류 시간 (MapName -> Seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, float> StagePlayTimes;

	// 맵별 세부 플레이 시간 (탐험, 보스전, 기타/거점)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, float> StageExplorationTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, float> StageBossTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, float> StageEtcTimes;

	// 21. 맵별 포탈/클리어 도달 소요 시간 (MapName -> Seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, float> StageClearPortalTimes;

	// 22. 맵별 플레이어 이동거리 (MapName -> Meters)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, float> StageMovementDistances;

	// 23. 맵별 + 오브젝트별 상호작용 키 입력 횟수 (MapName -> FInteractionCountMap)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, FInteractionCountMap> StageInteractionCounts;

	// 24. 맵별 낙사/낙하 및 체크포인트 리스폰 횟수 (MapName -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, int32> StageFallRespawnCounts;

	// 25. F키 튜토리얼 통스킵 횟수 & 대화 전체 스킵 버튼 횟수 & 지문 개별 넘김 횟수 & 지문 정상 완독 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Tutorial")
	int32 TutorialFullSkipCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Tutorial")
	int32 DialogueFullSkipCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Tutorial")
	int32 DialogueLineSkipCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Tutorial")
	int32 DialogueLineReadCount = 0;

	// 26. 옵션 변경 횟수 및 대화 로그 재확인 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|UI")
	int32 OptionChangeCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|UI")
	int32 DialogueLogRecheckCount = 0;

	// 27. 사망 원인 분포 (Reason -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> DeathReasonCounts;

	// 28. 맵별 조작 입력 분포 (Jump / Dash)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Controls")
	TMap<FString, int32> StageJumpCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Controls")
	TMap<FString, int32> StageDashCounts;

	// 29. 맵별 환경 기믹 / 기믹 종류별 파훼 / 보스 패턴 대응 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Gimmick")
	TMap<FString, float> GimmickStartTimes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Gimmick")
	TMap<FString, float> GimmickClearTimes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Gimmick")
	TMap<FString, int32> StageGimmickClearCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Gimmick")
	TMap<FString, int32> GimmickClearCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Gimmick")
	TMap<FString, int32> BossPatternDodgeCounts;

	// 30. 맵별 보스전 입장 / 사망 / 클리어 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Boss")
	TMap<FString, int32> BossEnterCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Boss")
	TMap<FString, int32> BossDeathCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Boss")
	TMap<FString, int32> BossClearCounts;

	// 31. 맵별 보스전 소요 시간 & 드롭 전리품/재화 획득량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Boss")
	TMap<FString, float> BossBattleTimes;

	// 보스전 시작 시각 (순수 전투 시간 자동 산출용 런타임 캐시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Boss")
	TMap<FString, float> ActiveBossBattleStartTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Boss")
	TMap<FString, int32> BossLootAcquiredCounts;

	// 보스별 최저 잔여 체력 비율 (0.0f ~ 1.0f, 체력 감소율 산출용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Boss")
	TMap<FString, float> BossMinHealthRatios;

	// 32. 연금술 포션 맵별 소모 횟수 & 종류(ItemID)별 소모 횟수 & 최신 체력 잔량 (%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Reward")
	TMap<FString, int32> PotionUsagePerMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Reward")
	TMap<FString, int32> PotionUsageByType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Reward")
	float LatestPlayerHealthPercent = 100.f;

	// 33. 봉헌 제단 상호작용 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Reward")
	int32 TributeAltarUsageCount = 0;

	// 34. 맵별 몬스터 가해 딜량 vs 피격 딜량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, float> StageDamageDealt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, float> StageDamageTaken;

	// ==========================================
	// 📌 맵(Stage)별 세부 지표 확장 필드
	// ==========================================

	// 맵별 가드 사용 횟수 & 가드 경감 피해량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> StageGuardCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, float> StageDamageMitigatedByGuard;

	// 맵별 획득 / 소모 코인
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	TMap<FString, int32> StageAcquiredCoins;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	TMap<FString, int32> StageConsumedCoins;

	// NPC 거래 상세 이력 (개별 거래 건별 시계열 기록)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	TArray<FNPCTransactionRecord> NPCTransactions;

	// 맵별 + NPC별 + 아이템별 코인 소모 집계 (Map_NPC_Item -> TotalCoinsSpent)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Economy")
	TMap<FString, int32> StageNPCCoinConsumption;

	// 맵별 아이템 획득 / 버리기 / 인벤토리 풀 발생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Inventory")
	TMap<FString, int32> StageItemsAcquired;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Inventory")
	TMap<FString, int32> StageItemsDiscarded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Inventory")
	TMap<FString, int32> StageInventoryFullCounts;

	// 맵별 인벤토리(가방) 열람 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Inventory")
	TMap<FString, int32> StageInventoryOpenCounts;

	// 맵별 일시정지 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|UI")
	TMap<FString, int32> StagePauseCounts;

	// 맵별 + 콤보명 콤보 사용 및 완주 횟수 (Map_Combo -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> StageUsedComboCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> StageCompletedComboCounts;

	// 맵별 + 몬스터명 처치 수 (Map_Monster -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> StageMonsterKillCounts;

	// 맵별 + 작물명 수확 수량 및 비약 사용 (Map_Crop -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	TMap<FString, int32> StageCropHarvestCounts;

	// 맵별 수확 행동 시행 횟수 (Map -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	TMap<FString, int32> StageCropHarvestActionCounts;

	// 맵별 작물 심기 시행 횟수 (Map -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	TMap<FString, int32> StageCropPlantCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	TMap<FString, int32> StageElixirUsageCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	TMap<FString, float> StageCropGrowthDelayDueToWeeds;

	// 맵별 + 상호작용 행동 시행 횟수 (Map_ActionType -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Interaction")
	TMap<FString, int32> StageInteractionActionCounts;

	// 맵별 + NPC명 대화 횟수 (Map_NPC -> Count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|NPC")
	TMap<FString, int32> StageNPCDialogueCounts;

	// 맵별 봉헌 제단 이용 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Reward")
	TMap<FString, int32> StageTributeAltarUsageCounts;

	// 맵별 옵션 변경 및 대화 지문 넘기기/로그 확인
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|UI")
	TMap<FString, int32> StageOptionChangeCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Tutorial")
	TMap<FString, int32> StageDialogueFullSkipCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Tutorial")
	TMap<FString, int32> StageDialogueLineSkipCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Tutorial")
	TMap<FString, int32> StageDialogueLineReadCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|UI")
	TMap<FString, int32> StageDialogueLogRecheckCounts;

	// ==========================================
	// 📌 5대 정밀 수집 체계 (공간 히트맵 좌표)
	// ==========================================

	// 35. 공간 좌표 사망/낙사 위치 트래킹 (히트맵 시각화용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Spatial")
	TArray<FVector> DeathPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Spatial")
	TArray<FVector> FallRespawnPositions;

	// ==========================================
	// 📌 누적 막대 그래프(Stacked Bar Chart) 정밀 시간 체계
	// ==========================================

	// 36. 숲 맵 등 체크포인트 구간별 순수 소요 시간 (Map_CPName -> SectionSeconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	TMap<FString, float> StageCheckpointSectionTimes;

	// 직전 체크포인트 도달 시각 (구간 시간 자동 산출용 런타임 캐시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	TMap<FString, float> LastCheckpointTimes;

	// 숲 맵 등 체크포인트 구간별 순수 이동거리 (Map_CPName -> SectionMeters)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Spatial")
	TMap<FString, float> StageCheckpointSectionDistances;

	// 직전 체크포인트 도달 시점의 이동거리 (구간 이동거리 자동 산출용 런타임 캐시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Spatial")
	TMap<FString, float> LastCheckpointDistances;

	// 맵별 도달한 최대 체크포인트 인덱스 (역주행 시 시간 왜곡 방지 필터)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	TMap<FString, int32> MaxReachedCheckpointIndex;

	// 37. 던전 맵 등 기믹 방별 순수 소요 시간 (Map_RoomName -> RoomSeconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	TMap<FString, float> StageGimmickRoomTimes;

	// 기믹 방 진입 시각 (소요 시간 자동 산출용 런타임 캐시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	TMap<FString, float> ActiveGimmickRoomStartTimes;

	// 던전 맵 등 기믹 방별 순수 이동거리 (Map_RoomName -> RoomMeters)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Spatial")
	TMap<FString, float> StageGimmickRoomDistances;

	// 기믹 방 진입 시점의 이동거리 (소요 거리 자동 산출용 런타임 캐시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Spatial")
	TMap<FString, float> ActiveGimmickRoomStartDistances;

	// 기믹 방 클리어 완료 여부 (클리어 후 재진입 시 시간 왜곡 방지 필터)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	TMap<FString, bool> ClearedGimmickRooms;

	// 38. 맵별 6대 행동 소요 시간 (Map_ActionCategory -> Seconds)
	// ActionCategory: Movement, Airborne, Combat, Interaction, PuzzleOrGimmick, UI_Pause
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Time")
	TMap<FString, float> StageActionDurations;

	// 39. 성공한 물약 제작 수 & 취침 횟수 & 세션 완료 플래그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Reward")
	int32 SuccessfulPotionCraftingCount = 0;

	// 데이터 테이블 기반 동적 기준 수치 및 제작/봉헌 추적
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Milestone")
	int32 TotalTargetCropTypes = 9;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Milestone")
	int32 TotalTargetPotionRecipes = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Milestone")
	int32 MaxTargetTributeSteps = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Milestone")
	int32 CurrentTributeLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Milestone")
	TMap<FString, int32> CraftedPotionTypeCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Farming")
	TMap<FString, int32> StageSleepCounts;

	// ==========================================
	// 📌 40. 최초 1회차 완주 스냅샷 (First Clear Snapshot - 영구 보존)
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearExplorationTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearBossTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearEtcTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearDistances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearDamageTaken;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, int32> StageFirstClearFallRespawns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, int32> StageFirstClearDeaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, int32> StageFirstClearMonsterKills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> FirstBossBattleTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearCheckpointSectionTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearCheckpointSectionDistances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearGimmickRoomTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|FirstClear")
	TMap<FString, float> StageFirstClearGimmickRoomDistances;

	// ==========================================
	// 📌 41. 맵별 단일 총합 지표 (MonsterKills, Deaths, CheckpointRetries)
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> StageTotalMonsterKillCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Combat")
	TMap<FString, int32> StageTotalDeathCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, int32> StageCheckpointRetryCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog|Stage")
	TMap<FString, int32> CheckpointSectionRetryCounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayLog")
	bool bIsGameCompleted = false;
};
