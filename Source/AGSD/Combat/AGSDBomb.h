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
class AGSD_API AAGSDBomb : public AActor, public IInteraction
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

	/** 폭발 시 재생할 나이아가라 시스템 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Effects")
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

	/** 점화 시 재생할 도화선 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Audio")
	TObjectPtr<USoundBase> FuseSound;

	/** 폭발 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb|Audio")
	TObjectPtr<USoundBase> ExplosionSound;

	/** 점화 상태 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomb")
	bool bIsIgnited = false;

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

	/** 블루프린트 연출 훅 (점화 시) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb")
	void OnBombIgnited();

	/** 블루프린트 연출 훅 (폭발 시) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb")
	void OnBombExploded();

private:
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

