#pragma once

#include "CoreMinimal.h"
#include "HoldingWeapon.generated.h" // UENUM을 사용하려면 반드시 필요합니다.

/**
 * 플레이어가 현재 들고 있는 아이템의 상태를 정의하는 UENUM 타입입니다. 
 */
UENUM(BlueprintType)
enum class EHoldingWeapon : uint8 // 클래스 이름이 아닌, ENUM 타입 이름은 'E'로 시작합니다.
{
	// ENUM 멤버 정의
	None        UMETA(DisplayName = "None"),
	Spear        UMETA(DisplayName = "Spear"),
	Sickle        UMETA(DisplayName = "Sickle"),
	Potion        UMETA(DisplayName = "Potion"),
	Torch         UMETA(DisplayName = "Torch")
};