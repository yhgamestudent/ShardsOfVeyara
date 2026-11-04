// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGSDBombSpawner.generated.h"

class UStaticMeshComponent;
class AAGSDBomb;
class UNiagaraSystem;
class USoundBase;

/**
 * 폭탄 자동 리젠 스포너 액터 (SOLID 원칙 준수)
 * - SRP: 폭탄의 스폰 위치 관리, 파괴(OnDestroyed) 감지, 리젠 타이머 카운트다운 및 새 폭탄 인스턴스 생성만을 전담.
 * - OCP: BombClass를 통해 다양한 폭탄 종류로 코드 수정 없이 확장 가능하며, OnBombSpawned/OnBombDestroyed 이벤트를 통해 블루프린트 연출 확장 지원.
 * - LSP: AAGSDBomb의 모든 파생 클래스(BP_Bomb 등)와 100% 호환.
 */
UCLASS(Blueprintable, ClassGroup = (Combat))
class AGSD_API AAGSDBombSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AAGSDBombSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 루트 씬 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 폭탄 거치대/받침대 메쉬 컴포넌트 (선택 사항, 메쉬를 지정하지 않으면 투명 스포너로 동작) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PedestalMesh;

	/** 폭탄이 생성될 정확한 스폰 위치 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SpawnPoint;

	/** 스폰할 폭탄 클래스 (BP_Bomb 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BombSpawner|Spawning")
	TSubclassOf<AAGSDBomb> BombClass;

	/** 폭탄이 파괴된 후 다시 생성될 때까지의 대기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BombSpawner|Spawning", meta = (ClampMin = "0.1", UIMin = "0.5"))
	float RespawnDelay = 4.0f;

	/** 게임 시작 시 첫 폭탄을 바로 스폰할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BombSpawner|Spawning")
	bool bSpawnOnBeginPlay = true;

	/** 게임 시작 후 첫 폭탄 생성까지의 지연 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BombSpawner|Spawning", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float InitialSpawnDelay = 0.0f;

	/** 폭탄 생성 시 재생할 나이아가라 이펙트 (옵션) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BombSpawner|Effects")
	TObjectPtr<UNiagaraSystem> SpawnVFX;

	/** 폭탄 생성 시 재생할 사운드 (옵션) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BombSpawner|Effects")
	TObjectPtr<USoundBase> SpawnSound;

	/** 현재 스포너가 관리 중인 활성 폭탄 인스턴스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BombSpawner|State")
	TWeakObjectPtr<AAGSDBomb> CurrentBomb;

public:
	/** 폭탄 스폰 함수 */
	UFUNCTION(BlueprintCallable, Category = "BombSpawner")
	virtual void SpawnBomb();

	/** 블루프린트 연출 훅: 새 폭탄이 스폰되었을 때 호출 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BombSpawner")
	void OnBombSpawned(AAGSDBomb* NewBomb);

	/** 블루프린트 연출 훅: 관리 중인 폭탄이 파괴되었을 때 호출 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BombSpawner")
	void OnBombDestroyed();

private:
	/** 폭탄 리젠 타이머 핸들 */
	FTimerHandle RespawnTimerHandle;

	/** 관리 중인 폭탄의 OnDestroyed 이벤트 바인딩 핸들러 */
	UFUNCTION()
	void OnCurrentBombDestroyed(AActor* DestroyedActor);

	/** 리젠 타이머 만료 시 호출 */
	void OnRespawnTimerExpired();
};

