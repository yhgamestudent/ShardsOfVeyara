#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SpearComboData.generated.h"

UENUM(BlueprintType)
enum class ESpearAttackDirection : uint8
{
	Neutral,
	Forward,
	Backward,
	Sprint,
	Parry
};

UENUM(BlueprintType)
enum class ESpearAttackInput : uint8
{
	LMB,           // 마우스 좌클릭 (기본 공격)
	RMB,           // 마우스 우클릭 (보조/특수 공격)
	Both_LMB_RMB   // 마우스 좌+우클릭 동시 입력 (스킬 콤보)
};

USTRUCT(BlueprintType)
struct FSpearStageData // 각 단계별 개별 애니메이션 데이터
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* AttackMontage; // 공격 동작 (Stage)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FText AttackName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bBreakComboOnHit = false;

	// 이 공격 몽타주 실행 시 뒤로 멀어지는 후퇴 워핑 적용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bUseBackwardWarp = false;

	// 각 스테이지(타수)별 요구 입력 버튼 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	ESpearAttackInput InputRequirement = ESpearAttackInput::LMB;
};

USTRUCT(BlueprintType)
struct FSpearComboData : public FTableRowBase
{
	GENERATED_BODY()

	// 콤보 하나(1~10번) 내의 모든 스테이지를 리스트로 관리
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSpearStageData> Stages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESpearAttackDirection DirectionRequirement = ESpearAttackDirection::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	ESpearAttackInput InputRequirement = ESpearAttackInput::LMB;
};