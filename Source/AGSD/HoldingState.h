// HoldingState.h 파일 내용

#pragma once

#include "CoreMinimal.h"
#include "HoldingState.generated.h" // UENUM을 사용하려면 반드시 필요합니다.

/**
 * 플레이어가 현재 들고 있는 아이템의 상태를 정의하는 UENUM 타입입니다. 
 */
UENUM(BlueprintType)
enum class EHoldingState : uint8 // 클래스 이름이 아닌, ENUM 타입 이름은 'E'로 시작합니다.
{
    // ENUM 멤버 정의
    EHS_None        UMETA(DisplayName = "None (아무것도 들고 있지 않음)"),
    EHS_Consumable  UMETA(DisplayName = "Consumable (소모품)"),
    EHS_Tool        UMETA(DisplayName = "Tool (도구)"),
    EHS_Seed        UMETA(DisplayName = "Seed (씨앗)"),
    EHS_Crop        UMETA(DisplayName = "Crop (작물)"),
    EHS_Alchemy     UMETA(DisplayName = "Alchemy (연금술)"),
    EHS_Shard       UMETA(DisplayName = "Shard (차원 조각)"),
    EHS_OtherItem   UMETA(DisplayName = "Other Item (기타 아이템)"),
    EHS_Potion      UMETA(DisplayName = "Potion (포션)")
};