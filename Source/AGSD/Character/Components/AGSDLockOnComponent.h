#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AGSDLockOnComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AGSD_API UAGSDLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAGSDLockOnComponent();

protected:
	virtual void BeginPlay() override;

public:
	// 매 프레임 캐릭터 Tick에서 호출되어 거리 및 장애물 시야 체크를 갱신합니다.
	void UpdateLockOnState(float DeltaSeconds);

	// 락온 토글 및 타겟 전환 함수
	void ToggleLockOn();
	void SwitchTargetLeft();
	void SwitchTargetRight();

	// 외부 조회 함수
	FORCEINLINE AActor* GetLockedTarget() const { return LockedTarget; }
	FORCEINLINE bool IsTargetLocked() const { return LockedTarget != nullptr; }

	// 락온 세팅
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float LockOnRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float MaxLockOnDistance = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float LineOfSightTimeoutDuration = 1.2f;

private:
	// 현재 락온 대상
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> LockedTarget = nullptr;

	// 시야 차단 여부
	bool bIsLineOfSightBlocked = false;

	// 시야 차단 시 자동 해제 대기 타이머
	FTimerHandle LineOfSightTimerHandle;

	// 헬퍼 함수
	AActor* FindNearestLockOnTarget();
	void SwitchTarget(bool bLookLeft);
	void OnLineOfSightTimeout();
	void SetLockOnMarkerState(AActor* TargetActor, bool bActive);

	// 소유주 캐릭터 캐싱
	UPROPERTY()
	TObjectPtr<class AAGSDCharacter> OwnerCharacter;
};
