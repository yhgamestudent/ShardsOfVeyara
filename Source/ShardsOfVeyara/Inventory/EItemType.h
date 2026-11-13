// EItemType.h - 아이템 유형 분류 에넘
// 기획서 기반: 아이템의 "종류"를 정의합니다. (장비, 소모품, 재료, 퀘스트)
// EHoldingState와는 분리된 개념으로, 인벤토리 필터링 및 UI 탭 분류에 사용됩니다.

#pragma once

#include "CoreMinimal.h"
#include "EItemType.generated.h"

/**
 * 아이템의 유형을 정의하는 에넘입니다.
 * 인벤토리 탭 필터링 및 데이터 테이블 분류에 사용됩니다.
 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	EIT_None		UMETA(DisplayName = "None (미분류)"),
	EIT_Equipment	UMETA(DisplayName = "Equipment (장비)"),
	EIT_Consumable	UMETA(DisplayName = "Consumable (소모품)"),
	EIT_Material	UMETA(DisplayName = "Material (재료)"),
	EIT_Quest		UMETA(DisplayName = "Quest (퀘스트)")
};
