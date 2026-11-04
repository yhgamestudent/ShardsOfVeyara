// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Combat/Interfaces/CombatDamageable.h"
#include "AGSDDestructibleRock.generated.h"

class UStaticMeshComponent;
class UGeometryCollectionComponent;

/**
 * 파괴 가능한 바위 액터 클래스 (SOLID 원칙 준수)
 * - 단일 책임 원칙(SRP): 평상시 지형 고정 및 피격 시 물리 파편화 전이만 담당.
 * - 개방-폐쇄 원칙(OCP): OnRockDestroyed 이벤트를 통해 블루프린트에서 폭파 VFX/사운드 등 연출 확장 가능.
 * - 의존 역전 원칙(DIP): 특정 에셋 하드코딩 배제, 인터페이스 및 언리얼 표준 컴포넌트에 의존.
 */
UCLASS(Blueprintable, ClassGroup = (Combat))
class AGSD_API AAGSDDestructibleRock : public AActor, public ICombatDamageable
{
	GENERATED_BODY()
	
public:	
	AAGSDDestructibleRock();

protected:
	virtual void BeginPlay() override;

	/** 루트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 바위 원본 스태틱 메시 컴포넌트 (바위 형태 그대로 정밀 충돌을 제공하는 콜리전 프록시) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RockStaticMesh;

	/** 카오스 지오메트리 컬렉션 컴포넌트 (바위 메쉬 및 파괴 시뮬레이션) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGeometryCollectionComponent> GeometryCollectionComponent;

	/** 바위 파괴에 필요한 최소 피해량 (기본값 25.0f: 일반 평타 1.0f 무시, 폭탄 30~100 방사 피해에 반응) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible")
	float DestructionDamageThreshold = 25.0f;

	/** 파괴 시 파편들을 사방으로 날려버리는 물리 속도 충격량 (cm/s, bVelChange=true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|Physics")
	float DestructionImpulse = 2500.0f;

	/** 파괴 물리 폭발 영향 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|Physics")
	float DestructionImpulseRadius = 900.0f;

	/** 파괴 후 잔해 파편 액터가 정리될 때까지의 지연 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible")
	float CleanupDelaySeconds = 6.0f;

	/** 이미 파괴되었는지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destructible")
	bool bIsDestroyed = false;

	/** 파편 충돌 채널 최적화 설정 (지형에만 충돌, 파편 간 및 캐릭터/카메라 충돌 무시하여 물리 부하 90% 이상 절감) */
	virtual void ConfigureFractureCollision();

public:
	/** 바위 원본 스태틱 메시 컴포넌트 접근자 */
	UStaticMeshComponent* GetRockStaticMesh() const { return RockStaticMesh; }

	/** 언리얼 엔진 표준 데미지 수신 핸들러 (방사형 데미지 등) */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/** ICombatDamageable 인터페이스 구현 */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	/** 바위 파괴 실행 함수 */
	UFUNCTION(BlueprintCallable, Category = "Destructible")
	virtual void TriggerDestruction(const FVector& HitLocation, const FVector& Impulse = FVector::ZeroVector);

	/** 블루프린트에서 사운드/이펙트/추가 연출을 바인딩할 수 있는 이벤트 훅 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Destructible")
	void OnRockDestroyed(const FVector& HitLocation);

private:
	/** 파편 정리 타이머 핸들 */
	FTimerHandle CleanupTimerHandle;

	/** 잔해 정리 함수 */
	void OnCleanupTimerExpired();
};
