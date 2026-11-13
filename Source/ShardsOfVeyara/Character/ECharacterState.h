#pragma once

#include "CoreMinimal.h"
#include "ECharacterState.generated.h"

/**
 * 캐릭터의 전반적인 행동 및 전투 상태를 정의하는 UENUM 타입입니다.
 */
UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle    UMETA(DisplayName = "Idle"),
	Combat  UMETA(DisplayName = "Combat"),
	Block   UMETA(DisplayName = "Block")
};
