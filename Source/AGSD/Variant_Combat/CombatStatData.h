// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CombatStatData.generated.h"

UENUM(BlueprintType)
enum class ECombatRole : uint8
{
	Player          UMETA(DisplayName = "플레이어"),
	NormalEnemy     UMETA(DisplayName = "일반 몬스터"),
	EliteEnemy      UMETA(DisplayName = "정예 몬스터"),
	Boss            UMETA(DisplayName = "보스"),
	Dummy           UMETA(DisplayName = "훈련용 더미")
};

/**
 * Kalivra 밸런싱 시뮬레이션 및 데이터테이블(DT_CombatStats.csv)과 1:1 매핑되는 전투 스탯 구조체
 */
USTRUCT(BlueprintType)
struct AGSD_API FCombatStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Info")
	FText CharacterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Info")
	ECombatRole CombatRole = ECombatRole::NormalEnemy;

	/** 최대 생명력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats", meta = (ClampMin = 1.0f))
	float MaxHealth = 1000.0f;

	/** 기본 공격력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats", meta = (ClampMin = 0.0f))
	float BaseAttackDamage = 80.0f;

	/** 방어력 (피해 감소 계산용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats", meta = (ClampMin = 0.0f))
	float Defense = 10.0f;

	/** 치명타 확률 (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float CriticalChance = 0.05f;

	/** 치명타 피해 배율 (예: 1.5 = 150%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats", meta = (ClampMin = 1.0f))
	float CriticalMultiplier = 1.5f;

	/** 평균 공격 주기 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Timing", meta = (ClampMin = 0.1f))
	float AttackInterval = 2.5f;

	/** 넉백 저항력 (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Physics", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float KnockbackResistance = 0.0f;

	/** 목표 처치 시간 (TTK) 가이드라인 (초 단위, 기획/시뮬레이션 참고용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DesignGuideline")
	float TargetTTK = 5.0f;
};
