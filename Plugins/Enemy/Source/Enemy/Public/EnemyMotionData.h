// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyMotionData.generated.h"

class UAnimMontage;

/**
 * 몬스터 공격의 형태 및 패턴 분류
 */
UENUM(BlueprintType)
enum class EEnemyAttackType : uint8
{
	MeleeBasic       UMETA(DisplayName = "근접 기본 공격"),
	MeleeHeavy       UMETA(DisplayName = "근접 강공격/도약"),
	RangedProjectile UMETA(DisplayName = "원거리 투사체"),
	ChargeRush       UMETA(DisplayName = "돌진 꿰뚫기"),
	AreaOfEffect     UMETA(DisplayName = "광역 장판/지진"),
	DefensiveCounter UMETA(DisplayName = "반격/가드 밀치기")
};

/**
 * [Shards of Veyara] 몬스터 모션밸런스 명세서 데이터테이블 구조체
 * 플레이어의 SpearComboData와 대칭되는 몬스터 전용 데이터 주도 액션 명세서입니다.
 */
USTRUCT(BlueprintType)
struct ENEMY_API FEnemyMotionData : public FTableRowBase
{
	GENERATED_BODY()

	// ─── [1. 몬스터 및 모션 식별] ───

	/** 몬스터 ID (예: Enemy_BaseMelee, Boss_BlackKnight 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identification")
	FName MonsterID;

	/** 모션/스킬 이름 (기획/디버깅 표시용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identification")
	FText AttackName;

	/** 공격 패턴 유형 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identification")
	EEnemyAttackType AttackType = EEnemyAttackType::MeleeBasic;

	/** 재생할 공격 애니메이션 몽타주 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// ─── [2. 밸런스 및 데미지 수치] ───

	/** 기본 공격력 대비 배율 (예: 약공 1.0x, 강공 1.5x, 필살기 2.0x 이상) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Balance", meta = (ClampMin = 0.1f))
	float DamageMultiplier = 1.0f;

	/** 공격 발동이 가능한 최대 유효 사거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Balance", meta = (ClampMin = 50.0f))
	float AttackRange = 250.0f;

	/** 해당 패턴 재사용 대기시간 (쿨타임, 초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Balance", meta = (ClampMin = 0.0f))
	float Cooldown = 3.0f;

	// ─── [3. 소울라이크 공방 타이밍 (프레임/시간 윈도우)] ───

	/** 선딜레이 (초): 모션 시작부터 실제 타격 판정 직전까지의 시간. 플레이어가 패링/가드/회피 반응할 수 있는 윈도우 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Timing", meta = (ClampMin = 0.05f))
	float WindupTime = 0.5f;

	/** 활성 타격 판정 지속 시간 (초): 실제 히트 판정 지속 구간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Timing", meta = (ClampMin = 0.05f))
	float ActiveHitDuration = 0.2f;

	/** 후딜레이 (초): 타격 종료 후 다음 행동이 가능할 때까지의 경직 시간. 플레이어의 딜타임 구간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Timing", meta = (ClampMin = 0.1f))
	float RecoveryTime = 1.0f;

	// ─── [4. 특수 액션 및 물리 판정] ───

	/** 일반 가드 불가 여부 (true면 저스트 패링 또는 구르기 순수 회피만 허용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Mechanics")
	bool bIsUnblockable = false;

	/** 모션 실행 중 피격 경직 무시 (슈퍼아머 활성화 여부) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Mechanics")
	bool bHasSuperArmor = false;

	/** 플레이어 적중 시 밀쳐내는 넉백 세기 (0이면 일반 경직) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Mechanics", meta = (ClampMin = 0.0f))
	float KnockbackPower = 0.0f;

	// ─── [5. 모션 워핑 (추적 돌진)] ───

	/** 타겟을 향해 모션 워핑(위치 보정/돌진)을 사용할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|MotionWarping")
	bool bUseMotionWarp = false;

	/** 전진 모션 워핑 최대 추적 거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|MotionWarping", meta = (ClampMin = 0.0f, EditCondition = "bUseMotionWarp"))
	float WarpDistance = 0.0f;

	// ─── [6. 보스 페이즈 조건] ───

	/** 해당 공격이 발동 가능한 보스 페이즈 (0=일반몹/항상, 1=1페이즈, 2=2페이즈 광폭화 전용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Phase", meta = (ClampMin = 0, ClampMax = 3))
	int32 RequiredPhase = 0;
};
