#pragma once

#include "CoreMinimal.h"
#include "CSVLogType.generated.h"



USTRUCT(BlueprintType)
struct FEnemyLogData
{
	GENERATED_BODY()

	// --- 1. CSV Columns (열 정의) ---

	/** 개체 식별을 위한 고유 ID (예: "Rat_102") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	FString EnemyID;
	/** 몬스터 클래스/개체 이름 (예: "Melee") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	FString EnemyType;
	// 전투 결과 - 사망 승리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	FString Result;
	/** 플레이어가 인지되어 전투가 시작된 월드 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float StartWorldTime = 0.0f;
	/** 적이 죽거나 전투가 완전히 끝난 월드 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float EndWorldTime = 0.0f;
	/** 실제 교전 소요 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float ElapsedTime = 0.0f;
	/** 이 적이 한 세션 동안 받은 총 대미지 양 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float TotalDamageReceived = 0.f;
	// 이 적이 플레이어에게 입힌 총 대미지 양
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float TotalDamageDealt = 0.f;
	/** 적이 플레이어 공격을 성공적으로 가드한 대미지 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float TotalDamageGuarded = 0.f;
	// 적이 가드 임계치를 뚫어 가드 반격을 한 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	int32 CounterAttackCount = 0;
	// 적이 부활한 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	int32 ReviveCount = 0;
	// 스포너가 몬스터를 얼마나 스폰하였는지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	int32 SpawnCount = 0;
	
	// --- 2. Serialization (행 텍스트 변환 구현) ---
	FString ToCSVRow() const
	{
		return FString::Printf(TEXT("%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d"), 
			*EnemyID, 
			*EnemyType, 
			*Result, 
			StartWorldTime, 
			EndWorldTime, 
			ElapsedTime, 
			TotalDamageReceived, 
			TotalDamageDealt, 
			TotalDamageGuarded, 
			CounterAttackCount, 
			ReviveCount, 
			SpawnCount);
	}

	/** CSV 파일 맨 첫 행에 기입될 고유 열(Header) 칼럼 이름들을 정의합니다. */
	static FString GetCSVHeader()
	{
		return TEXT("EnemyID,EnemyType,Result,StartWorldTime,EndWorldTime,ElapsedTime,TotalDamageReceived,TotalDamageDealt,TotalDamageGuarded,CounterAttackCount,ReviveCount,SpawnCount");
	}
	
};

USTRUCT(BlueprintType)
struct FCommonBossLogData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	FString BossID;
	// 전투 결과 - 사망 승리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	FString Result;
	/** 플레이어가 인지되어 전투가 시작된 월드 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float StartWorldTime = 0.0f;
	/** 적이 죽거나 전투가 완전히 끝난 월드 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float EndWorldTime = 0.0f;
	/** 실제 교전 소요 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float ElapsedTime = 0.0f;
	/** 이 적이 한 세션 동안 받은 총 대미지 양 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float TotalDamageReceived = 0.f;
	// 이 적이 플레이어에게 입힌 총 대미지 양
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	float TotalDamageDealt = 0.f;
	// 근거리 패턴 결정 회수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	int32 ShortRangePatternCount = 0;
	// 중거리 패턴 정한 회수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	int32 MidRangePatternCount = 0;
	// 원거리 패턴 정한 회수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CSV Log")
	int32 LongRangePatternCount = 0;
	
	// 공통 데이터 Row 문자열 반환
	FString ToCSVRow() const
	{
		return FString::Printf(TEXT("%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d"), 
			*BossID, *Result, StartWorldTime, EndWorldTime, ElapsedTime, TotalDamageReceived, TotalDamageDealt, 
			ShortRangePatternCount, MidRangePatternCount, LongRangePatternCount);
	}

	// 공통 데이터 Header 문자열 반환
	static FString GetCSVHeader()
	{
		return TEXT("BossID,Result,StartWorldTime,EndWorldTime,ElapsedTime,TotalDamageReceived,TotalDamageDealt,ShortRangePattern,MidRangePattern,LongRangePattern");
	}
};

USTRUCT(BlueprintType)
struct FBossSkeletonMageLogData
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FCommonBossLogData Base; // 공통 데이터 포함
    
    // 각 보스마다 다른 패턴 사용 횟수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    int32 TeleportCount = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    int32 FireBallCount = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    float FireBallDamage = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    int32 SummonCount = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    int32 GroundAttackCount = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    float GroundAttackDamage = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    int32 PushTargetCount = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    int32 GravityAttackCount = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    float GravityAttackDamage = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    float SecondPhaseThunderAttackDamage = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
    float SecondPhaseMeteorAttackDamage = 0.0f;
    
    FString ToCSVRow() const
    {
       // 맨 뒤에 %.3f, %.3f 서식 지정자 2개 추가 및 데이터 변수 매칭
       return FString::Printf(TEXT("%s,%d,%d,%.3f,%d,%d,%.3f,%d,%d,%.3f,%.3f,%.3f"), 
          *Base.ToCSVRow(),
          TeleportCount, FireBallCount, FireBallDamage, SummonCount, 
          GroundAttackCount, GroundAttackDamage, PushTargetCount, 
          GravityAttackCount, GravityAttackDamage,
          SecondPhaseThunderAttackDamage, SecondPhaseMeteorAttackDamage);
    }

    static FString GetCSVHeader()
    {
       // 헤더 맨 뒤에도 추가된 컬럼명 명시
       return FString::Printf(TEXT("%s,TeleportCount,FireBallCount,FireBallDamage,SummonCount,GroundAttackCount,GroundAttackDamage,PushTargetCount,GravityAttackCount,GravityAttackDamage,SecondPhaseThunderAttackDamage,SecondPhaseMeteorAttackDamage"), 
          *FCommonBossLogData::GetCSVHeader());
    }
};

USTRUCT(BlueprintType)
struct FBossMagicSwordManLogData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCommonBossLogData Base; // 공통 데이터 포함
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 CloseAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float CloseAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 DashAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float DashAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 CloseJumpUpAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float CloseJumpUpAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 DashJumpUpAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float DashJumpUpAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 JumpUpAttackSuccessCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float AirAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")	
	int32 JumpAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float JumpAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 GuardCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float TotalDamageGuarded = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 GuardCounterAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float GuardCounterAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 BladeWaveAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float BladeWaveAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	int32 BackDashCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float PowerAttackDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float RushStrikeAttackDamage = 0.f;
	
	// CSV 행 변환 구현 (순수 텍스트 2개 + 정수/실수 21개 = 총 23개 데이터)
	FString ToCSVRow() const
	{
		return FString::Printf(TEXT("%s,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%.3f"), 
		   *Base.ToCSVRow(), 
		   CloseAttackCount, CloseAttackDamage,
		   DashAttackCount, DashAttackDamage,
		   CloseJumpUpAttackCount, CloseJumpUpAttackDamage,
		   DashJumpUpAttackCount, DashJumpUpAttackDamage,
		   JumpUpAttackSuccessCount, AirAttackDamage,
		   JumpAttackCount, JumpAttackDamage,
		   GuardCount, TotalDamageGuarded,
		   GuardCounterAttackCount, GuardCounterAttackDamage,
		   BladeWaveAttackCount, BladeWaveAttackDamage,
		   BackDashCount,
		   PowerAttackDamage,
		   RushStrikeAttackDamage); 
	}

	// CSV 헤더 컬럼명 정의 (위의 ToCSVRow 서식 지정자 콤마 개수와 1:1 완벽 매칭)
	static FString GetCSVHeader()
	{
		return FString::Printf(TEXT("%s,CloseAttackCount,CloseAttackDamage,DashAttackCount,DashAttackDamage,CloseJumpUpAttackCount,CloseJumpUpAttackDamage,DashJumpUpAttackCount,DashJumpUpAttackDamage,JumpUpAttackSuccessCount,AirAttackDamage,JumpAttackCount,JumpAttackDamage,GuardCount,TotalDamageGuarded,GuardCounterAttackCount,GuardCounterAttackDamage,BladeWaveAttackCount,BladeWaveAttackDamage,BackDashCount,PowerAttackDamage,RushStrikeAttackDamage"), 
		   *FCommonBossLogData::GetCSVHeader());
	}
};

USTRUCT(BlueprintType)
struct FHechiLogData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCommonBossLogData Base; // 공통 데이터 포함
	
	// CSV 행 변환 구현 (공통 데이터 Row 문자열을 그대로 반환)
	FString ToCSVRow() const
	{
		return Base.ToCSVRow();
	}

	// CSV 헤더 컬럼명 정의 (공통 데이터 Header 문자열을 그대로 반환)
	static FString GetCSVHeader()
	{
		return FCommonBossLogData::GetCSVHeader();
	}
};

UENUM(BlueprintType)
enum class ECombatEventType : uint8
{
	None				UMETA(DisplayName = "None"),
	BattleStart			UMETA(DisplayName = "BattleStart"),
	SkillWindup			UMETA(DisplayName = "SkillWindup"),
	SkillRelease		UMETA(DisplayName = "SkillRelease"),
	SkillRecovery		UMETA(DisplayName = "SkillRecovery"),
	HitSuccess			UMETA(DisplayName = "HitSuccess"),
	HitMissed			UMETA(DisplayName = "HitMissed"),
	HitGuarded			UMETA(DisplayName = "HitGuarded"),
	HitJustGuard		UMETA(DisplayName = "HitJustGuard"),
	HitEvaded			UMETA(DisplayName = "HitEvaded"),
	BossDamaged			UMETA(DisplayName = "BossDamaged"),
	PhaseTransition		UMETA(DisplayName = "PhaseTransition"),
	BattleEnd			UMETA(DisplayName = "BattleEnd")
};

USTRUCT(BlueprintType)
struct FCombatTimelineEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString Timestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float CombatTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString ActionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	ECombatEventType EventType = ECombatEventType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString SourceActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString ActionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float Damage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float Distance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString PlayerState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float BossHP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float BossHPPct = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float PlayerHP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float PlayerHPPct = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString Details;

	FString GetEventTypeString() const
	{
		switch (EventType)
		{
		case ECombatEventType::BattleStart: return TEXT("BATTLE_START");
		case ECombatEventType::SkillWindup: return TEXT("SKILL_WINDUP");
		case ECombatEventType::SkillRelease: return TEXT("SKILL_RELEASE");
		case ECombatEventType::SkillRecovery: return TEXT("SKILL_RECOVERY");
		case ECombatEventType::HitSuccess: return TEXT("HIT_SUCCESS");
		case ECombatEventType::HitMissed: return TEXT("HIT_MISSED");
		case ECombatEventType::HitGuarded: return TEXT("HIT_GUARDED");
		case ECombatEventType::HitJustGuard: return TEXT("HIT_JUST_GUARD");
		case ECombatEventType::HitEvaded: return TEXT("HIT_EVADED");
		case ECombatEventType::BossDamaged: return TEXT("BOSS_DAMAGED");
		case ECombatEventType::PhaseTransition: return TEXT("PHASE_TRANSITION");
		case ECombatEventType::BattleEnd: return TEXT("BATTLE_END");
		default: return TEXT("UNKNOWN");
		}
	}

	FString ToCSVRow() const
	{
		return FString::Printf(TEXT("%s,%.3f,%s,%s,%s,%s,%s,%.1f,%.1f,%s,%.1f,%.1f%%,%.1f,%.1f%%,%s"),
			*Timestamp,
			CombatTime,
			*ActionID,
			*GetEventTypeString(),
			*SourceActor,
			*TargetActor,
			*ActionName,
			Damage,
			Distance,
			*PlayerState,
			BossHP,
			BossHPPct,
			PlayerHP,
			PlayerHPPct,
			*Details.Replace(TEXT(","), TEXT(";")));
	}

	static FString GetCSVHeader()
	{
		return TEXT("Timestamp,CombatTime,ActionID,EventType,SourceActor,TargetActor,ActionName,Damage,Distance,PlayerState,BossHP,BossHPPct,PlayerHP,PlayerHPPct,Details");
	}
};

USTRUCT(BlueprintType)
struct FSkillAggregateStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	FString SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	int32 CastCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	int32 HitCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	int32 MissCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	int32 GuardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	int32 JustGuardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float TotalWindupTime = 0.0f;
	int32 WindupSamples = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float TotalTravelTime = 0.0f;
	int32 TravelSamples = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float TotalRecoveryTime = 0.0f;
	int32 RecoverySamples = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog")
	float TotalDamageDealt = 0.0f;

	TMap<FString, int32> VictimStateCounts;
};