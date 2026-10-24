#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CSVLogType.h"
#include "CombatTimelineLogger.generated.h"

struct FActiveSkillTrackingInfo
{
	FString ActionID;
	FString ActionName;
	float WindupStartTime = 0.0f;
	float ReleaseTime = 0.0f;
	float InitialPlayerDistance = 0.0f;
};

/**
 * 전투 타임라인 고정밀 이벤트 로거 (Combat Timeline Logger)
 * 보스 전투의 모든 사건(스킬 시전 선딜/발사/후딜, 적중/빗나감/가드/저스트가드, 피격 상태, HP 변화)을
 * 밀리초 단위 타임스탬프와 함께 독립된 세션 CSV 파일로 기록하고,
 * 전투 종료 시 하단에 스킬별 밸런스 종합 요약표(KPI Summary)를 자동 생성합니다.
 */
UCLASS()
class LOGDATA_API UCombatTimelineLogger : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 세션 시작: 폴더 생성, 고유 타임스탬프 파일명 결정, CSV 헤더 작성, BATTLE_START 이벤트 기록 */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void StartCombatSession(
		const FString& BossID,
		float StartWorldTime,
		float BossMaxHP,
		float PlayerMaxHP,
		float InitialDistance = 0.0f
	);

	/** 스킬 시전 시작 (선딜레이 시작): 고유 ActionID(예: FireBall_#1)를 생성하여 반환 */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static FString LogSkillWindup(
		const FString& ActionName,
		float CurrentWorldTime,
		float Distance,
		const FString& PlayerState,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP
	);

	/** 스킬 발사/판정 발생 (선딜레이 종료, 투사체 비행/장판 전개 시작) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogSkillRelease(
		const FString& ActionID,
		float CurrentWorldTime,
		float Distance,
		const FString& PlayerState,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP,
		const FString& AdditionalDetails = TEXT("")
	);

	/** 스킬 후딜레이 종료 (보스가 다음 행동 가능한 상태로 복귀) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogSkillRecovery(
		const FString& ActionID,
		float CurrentWorldTime,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP
	);

	/** 플레이어 피격 적중 (HIT_SUCCESS) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogHitSuccess(
		const FString& ActionID,
		const FString& ActionName,
		float Damage,
		float CurrentWorldTime,
		float Distance,
		const FString& PlayerState,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP
	);

	/** 스킬 빗나감 (HIT_MISSED - 벽/바닥 충돌 또는 플레이어 탈출 소멸) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogHitMissed(
		const FString& ActionID,
		const FString& ActionName,
		float CurrentWorldTime,
		float Distance,
		const FString& Reason = TEXT("Escaped/Obstacle")
	);

	/** 일반 가드로 방어 성공 (HIT_GUARDED) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogHitGuarded(
		const FString& ActionID,
		const FString& ActionName,
		float MitigatedDamage,
		float CurrentWorldTime,
		float Distance,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP
	);

	/** 저스트 가드로 완벽 방어 성공 (HIT_JUST_GUARD) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogHitJustGuard(
		const FString& ActionID,
		const FString& ActionName,
		float CurrentWorldTime,
		float Distance,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP
	);

	/** 보스가 플레이어 공격에 피격됨 (BOSS_DAMAGED) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogBossDamaged(
		const FString& AttackName,
		float Damage,
		float CurrentWorldTime,
		float Distance,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP,
		const FString& PlayerState = TEXT("Attacking")
	);

	/** 페이즈 전환 (PHASE_TRANSITION) */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void LogPhaseTransition(
		const FString& PhaseName,
		float CurrentWorldTime,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP,
		const FString& Details = TEXT("")
	);

	/** 세션 종료: BATTLE_END 기록, 스킬별 KPI 종합 통계 요약표 하단 작성 및 파일 닫기 */
	UFUNCTION(BlueprintCallable, Category = "CombatLog|Timeline")
	static void EndCombatSession(
		const FString& Result,
		float EndWorldTime,
		float BossHP,
		float BossMaxHP,
		float PlayerHP,
		float PlayerMaxHP
	);

	/** 현재 세션이 활성 상태인지 확인 */
	UFUNCTION(BlueprintPure, Category = "CombatLog|Timeline")
	static bool IsSessionActive();

	/** 가장 최근 시전된 특정 스킬의 ActionID 반환 (투사체 충돌 매칭용) */
	static FString GetLatestActionID(const FString& ActionName);

private:
	static bool bSessionActive;
	static FString CurrentSessionFilePath;
	static FString CurrentBossID;
	static float SessionStartWorldTime;
	static int32 ActionCounter;

	static TMap<FString, FActiveSkillTrackingInfo> ActiveSkills;
	static TMap<FString, FSkillAggregateStats> SkillStats;

	static void AppendRowToFile(const FString& RowText);
	static void WriteEvent(const FCombatTimelineEvent& Event);
};
