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

	// 공격 시 사용할 타겟 반환 (하드 락온이 있으면 우선 반환, 없으면 소프트 타겟 탐색 후 반환)
	AActor* GetTargetForAttack(bool& bOutIsHardLocked);

	// 비락온 시 카메라 정면 범위 내의 적을 1회성으로 탐색
	AActor* FindSoftLockTarget();

	// 락온 세팅
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float LockOnRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float MaxLockOnDistance = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float LineOfSightTimeoutDuration = 1.2f;

	// 소프트 락온 세팅
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|SoftLock")
	bool bEnableSoftLockOn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|SoftLock")
	float SoftLockRadius = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|SoftLock")
	float SoftLockMaxAngle = 30.0f;

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
