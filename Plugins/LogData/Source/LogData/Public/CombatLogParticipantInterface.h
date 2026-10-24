#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatLogParticipantInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCombatLogParticipantInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 전투 로그 참가자 인터페이스 (DIP 원칙 적용)
 * 플레이어와 보스가 상호 모듈 간 직접 참조 없이 체력 및 전투 상태를 로거에 제공할 수 있도록 추상화합니다.
 */
class LOGDATA_API ICombatLogParticipantInterface
{
	GENERATED_BODY()

public:
	virtual float GetParticipantHealth() const { return 0.0f; }
	virtual float GetParticipantMaxHealth() const { return 0.0f; }
	virtual FString GetParticipantCombatState() const { return TEXT("Idle"); }

	/** 중력 부유 등 외부 강제 패턴에 의한 액션 인터럽트 (구르기 중일 때는 실행되지 않음) */
	virtual void InterruptCombatAction(bool bInterruptAttack, bool bInterruptGuard) {}
};
