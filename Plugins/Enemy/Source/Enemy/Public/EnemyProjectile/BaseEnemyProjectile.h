#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEnemyProjectile.generated.h"

UENUM(BlueprintType)
enum class EKnockdownType : uint8
{
	None          UMETA(DisplayName = "None (일반 피격)"),
	Kneel         UMETA(DisplayName = "Kneel (무릎 꿇기)"),
	AirborneFloor UMETA(DisplayName = "Airborne to Floor (에어본 ➔ 바닥 엎어짐)")
};

UCLASS()
class ENEMY_API ABaseEnemyProjectile : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	// 발사체 파괴시 나오는 이펙트 처리는 자식에서 각도 조정 팔요
	virtual void CreateHitEffect();
	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UPROPERTY(EditAnywhere, Category="자체설정")
	bool bOnlyNiagaraEffect = false; // 나이아가라 이펙트만 사용할지 여부
	// 발사체 속도
	UPROPERTY(EditAnywhere, Category="자체설정")
	float ProjectileSpeed = 3000.f; // 초기 속도
	UPROPERTY(EditAnywhere, Category="자체설정")
	float Damage = 20.f; // 데미지
	// 충돌 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* CollisionComp;
	// 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* MeshComp;
	// 투사체 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UProjectileMovementComponent* ProjectileMovement;
	// 나이아가라 이펙트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UNiagaraComponent* NiagaraEffectComp;
	// trail 이펙트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UNiagaraComponent* TrailEffectComp;
	// 충돌 시 이펙트
	UPROPERTY(EditAnywhere, Category="자체설정")
	class UNiagaraSystem* HitEffect;
	// 이펙트 생성 위치
	FVector EffectCreateLocation;
	
public:
	ABaseEnemyProjectile();
	virtual void Tick(float DeltaTime) override;

	/** 피격 연출 강도 (0.0 = 움찔, 1.0 = 비틀거림) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="자체설정", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitIntensity = 1.0f;

	/** 피격 시 유발할 넉다운 타입 (None = 없음, Kneel = 무릎 꿇기, AirborneFloor = 에어본 후 바닥 엎어짐) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="자체설정|HitReaction")
	EKnockdownType KnockdownType = EKnockdownType::None;

	/** 넉다운 지속 시간 (바닥/무릎 상태 유지 시간, 초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="자체설정|HitReaction")
	float KnockdownDuration = 1.5f;

	FORCEINLINE class UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }
	UFUNCTION(BlueprintCallable, Category="자체설정")
	void SetDamage(float InDamage) { Damage = InDamage; }

	UPROPERTY(BlueprintReadWrite, Category="CombatLog")
	FString ActionID;

	UPROPERTY(BlueprintReadWrite, Category="CombatLog")
	FString ActionName = TEXT("FireBall");

	bool bHasHitPlayer = false;

	virtual void Destroyed() override;

#if WITH_EDITOR
	// 에디터에서 프로퍼티가 변경될 때 호출됩니다.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
