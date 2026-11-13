// Fill out your copyright notice in the Description page of Project Settings.



#pragma once



#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

#include "Interaction.h"

#include "AGSDBomb.generated.h"



class UStaticMeshComponent;

class USphereComponent;

class UPointLightComponent;

class UNiagaraSystem;

class USoundBase;



/**

 * 상호작용 가능한 폭탄/화약통 액터 클래스 (SOLID 원칙 준수)

 * - IInteraction 인터페이스를 구현하여 플레이어가 다가가서 E키로 점화시킵니다.

 * - 일정 시간(FuseTime) 카운트다운 후 폭발합니다.

 * - 폭발 시 방사형 데미지(RadialDamage)를 가해 범위 내 부서지는 바위(AAGSDDestructibleRock)를 폭파하고 플레이어에게도 피해를 줍니다.

 */

UCLASS(Blueprintable, ClassGroup = (Combat))

class SHARDSOFVEYARA_API AAGSDBomb : public AActor, public IInteraction

{

	GENERATED_BODY()

	

public:	

	AAGSDBomb();



protected:

	virtual void BeginPlay() override;



	/** 폭탄 스태틱 메시 컴포넌트 (루트 컴포넌트, 물리 시뮬레이션 적용) */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")

	TObjectPtr<UStaticMeshComponent> BombMesh;



	/** 플레이어 상호작용 감지용 구체 콜리전 */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")

	TObjectPtr<USphereComponent> InteractionSphere;



	/** 점화 시 깜빡이는 도화선 불빛 */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")

	TObjectPtr<UPointLightComponent> FuseLight;



	/** 점화 후 폭발까지 걸리는 시간 (초) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")

	float FuseTime = 3.0f;



	/** 폭발 피해 및 충격 반경 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")

	float ExplosionRadius = 450.0f;



	/** 폭발 중심부 최대 데미지 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")

	float BaseDamage = 100.0f;



	/** 폭발 외곽 최소 데미지 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")

	float MinimumDamage = 30.0f;



	/** 거리별 데미지 감쇄율 (1.0 = 선형 감쇄) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")

	float DamageFalloff = 1.0f;



	/** 폭발 물리 충격량 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")

	float ExplosionImpulse = 3000.0f;



	/** 플레이어 피격 반응: 이 반경 이내면 에어본 넉다운(날아감), 바깥이면 일반 피격 (강도 1 → 0 보간) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|PlayerReaction", meta = (ClampMin = "0.0"))
	float KnockdownRadius = 180.0f;

	/** 에어본 넉다운 시 바닥에 누워 있는 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|PlayerReaction", meta = (ClampMin = "0.0"))
	float KnockdownDuration = 1.2f;

	/** 에어본 넉다운 시 폭발 반대 방향으로 날려보내는 수평 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|PlayerReaction")
	float KnockdownLaunchHorizontalSpeed = 700.0f;

	/** 에어본 넉다운 시 위로 띄우는 수직 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|PlayerReaction")
	float KnockdownLaunchVerticalSpeed = 650.0f;

	/** 폭발 시 재생할 나이아가라 시스템 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Effects")

	TObjectPtr<UNiagaraSystem> ExplosionVFX;



	/** 폭발 시 월드에 재생할 카메라 쉐이크 클래스 (미지정 시 재생 안 함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Effects")
	TSubclassOf<class UCameraShakeBase> ExplosionCameraShakeClass;

	/** 카메라 쉐이크 최대 세기 유지 내부 반경 (카메라 붐 고려 기본 600cm, 0 이하 설정 시 ExplosionRadius 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Effects")
	float CameraShakeInnerRadius = 600.0f;

	/** 카메라 쉐이크 거리 감쇄 외부 반경 (기본 1500cm, 약 15m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Effects")
	float CameraShakeOuterRadius = 1500.0f;

	/** 카메라 쉐이크 거리 감쇄 지수 (1.0 = 선형 감쇄) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Effects")
	float CameraShakeFalloff = 1.0f;

	/** 점화 시 재생할 도화선 사운드 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Audio")

	TObjectPtr<USoundBase> FuseSound;



	/** 폭발 사운드 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Audio")

	TObjectPtr<USoundBase> ExplosionSound;



	/** 점화 상태 여부 */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomb")

	bool bIsIgnited = false;



	/** 상호작용 위젯에 표시될 안내 텍스트 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Interaction")

	FText InteractActionText = FText::FromString(TEXT("폭탄 점화하기"));



public:

	// --- IInteraction 인터페이스 구현 ---

	virtual void Interact_Implementation(AAGSDCharacter* player) override;

	virtual void ShowWidget_Implementation(ACharacter* player) override;

	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;

	virtual FString GetInteractionActionType_Implementation(AAGSDCharacter* player) override { return TEXT("Ignite"); }



	/** 점화 시작 함수 */

	UFUNCTION(BlueprintCallable, Category = "Bomb")

	virtual void Ignite();



	/** 폭발 실행 함수 */

	UFUNCTION(BlueprintCallable, Category = "Bomb")

	virtual void Explode();



	/**
	 * 폭발 지점 기준 대상 위치의 피격 반응을 계산합니다.
	 * @param bOutKnockdown      KnockdownRadius 이내면 true (에어본 넉다운)
	 * @param OutHitIntensity    일반 피격 강도 (KnockdownRadius 경계 1.0 → ExplosionRadius 경계 0.0 선형 보간)
	 * @param OutLaunchVelocity  넉다운 시 날려보낼 속도 (일반 피격이면 ZeroVector)
	 * @return 폭발 반경 안이면 true
	 */
	bool GetExplosionReaction(const FVector& TargetLocation, bool& bOutKnockdown, float& OutHitIntensity, FVector& OutLaunchVelocity) const;

	float GetKnockdownDuration() const { return KnockdownDuration; }

	/** 블루프린트 연출 훅 (점화 시) */

	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb")

	void OnBombIgnited();



	/** 블루프린트 연출 훅 (폭발 시) */

	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb")

	void OnBombExploded();



private:

	/** 실제 폭발 판정 중심 (Explode 시점에 기록) */
	FVector ExplosionCenter = FVector::ZeroVector;

	/** 폭발 타이머 핸들 */

	FTimerHandle FuseTimerHandle;



	/** 도화선 깜빡임 타이머 핸들 */

	FTimerHandle FlickerTimerHandle;



	/** 도화선 깜빡임 로직 */

	void ToggleFuseFlicker();



	/** 상호작용 오버랩 이벤트 핸들러 */

	UFUNCTION()

	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);



	UFUNCTION()

	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};



