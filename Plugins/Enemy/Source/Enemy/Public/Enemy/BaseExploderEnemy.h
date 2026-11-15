#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "BaseExploderEnemy.generated.h"


UCLASS()
class ENEMY_API ABaseExploderEnemy : public ABaseEnemy
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* ExplosionRangeSphere;

protected:
	UPROPERTY(EditAnywhere, Category="자체설정")
	float ExplosionRange = 200.f;

	UPROPERTY(EditAnywhere, Category="자체설정")
	float ExplosionDamage = 50.f;

	UPROPERTY(EditDefaultsOnly, Category="자체설정")
	class UNiagaraSystem* ExplosionEffect;

	/**
	 * 점프 모션 이동 모드 (개구리 등)
	 * - true면 점프 애니메이션의 'Hop Move Window' 노티파이 구간에서만 실제로 이동합니다.
	 * - 구간 밖에서도 애니메이션용 속도(GetVelocity)는 유지하여 점프 루프가 끊기지 않습니다.
	 * - 점프 애니메이션에 Hop Move Window 노티파이 스테이트가 없으면 이동하지 않으니 주의!
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="자체설정|HopMovement")
	bool bUseHopMovement = false;

	virtual void BeginPlay() override;

public:
	ABaseExploderEnemy();
	virtual void Tick(float DeltaTime) override;
	virtual void InitializeStatsFromDataTable() override;

	/** 애님 블루프린트(Speed = VSize(GetVelocity()))가 이동 의도가 있는 동안 점프 루프를 유지하도록 보정 */
	virtual FVector GetVelocity() const override;

	/** Hop Move Window 노티파이 스테이트에서 호출 */
	void SetHopMoveWindowActive(bool bActive) { bHopMoveWindowActive = bActive; }
	
	UFUNCTION(BlueprintCallable)
	void Explode();

#if WITH_EDITOR
	// 에디터에서 프로퍼티가 변경될 때 호출됩니다.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/** 점프 공중 구간 여부 (노티파이로 갱신) */
	bool bHopMoveWindowActive = false;

	/** 점프 중 이동 속도 (BeginPlay 시점의 MaxWalkSpeed) */
	float HopMoveSpeed = 0.0f;

	/** AI가 이동 명령(MoveTo) 수행 중인지 */
	bool IsAIRequestingMove() const;
};
