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
struct ENEMY_API FCombatStatRow : public FTableRowBase
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

	/** 평균 공격 주기 / 딜레이 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Timing", meta = (ClampMin = 0.1f))
	float AttackInterval = 2.5f;
};
