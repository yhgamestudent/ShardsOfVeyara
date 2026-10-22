// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Struct_ItemData.h"
#include "PortalSpawner.generated.h"

class UBoxComponent;
class UAGSDInventoryComponent;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class AGSD_API APortalSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	APortalSpawner();

	// 포털 소환을 트리거하는 함수 (블루프린트 또는 C++에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Portal Spawner")
	void TriggerSpawnPortal();

	// 스폰 영역 박스 컴포넌트 getter
	FORCEINLINE UBoxComponent* GetSpawnAreaBox() const { return SpawnAreaBox; }

	// 보상 스폰 위치 컴포넌트 getter
	FORCEINLINE USceneComponent* GetRewardSpawnPoint() const { return RewardSpawnPoint; }

	// 보스 보상 아이템을 중앙 위치에 실제로 스폰하는 함수
	UFUNCTION(BlueprintCallable, Category = "Portal Spawner|Boss Reward")
	void SpawnRewardItem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 레벨에서 포털 스폰 범위를 지정하는 콜리전 박스 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Spawner")
	UBoxComponent* SpawnAreaBox;

	// 보스 처치 후 구슬 조각 등 보상 아이템이 스폰될 중앙 타겟 위치 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Spawner|Boss Reward")
	USceneComponent* RewardSpawnPoint;

	// 감시할 보스 액터 (인스턴스에서 지정 가능)
	UPROPERTY(EditInstanceOnly, Category = "Portal Spawner")
	AActor* TargetBoss;

	// 소환할 포털 클래스
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	TSubclassOf<AActor> PortalClass;

	// 포털 활성화에 필요한 보스 드랍 아이템 ID (비어있으면 기본적으로 EItemType::EIT_Quest 타입 획득 시 활성화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner")
	FString RequiredItemID;

	// 드랍 아이템 획득 후 포털을 활성화할지 여부 (false면 보스 사망 즉시 활성화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner")
	bool bRequireItemAcquisition = true;

	// 1회성 드롭 아이템 여부 확인 (이미 획득한 이력이 있으면 보스 사망 즉시 포털 활성화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner")
	bool bCheckAlreadyDropped = true;

	// 보스 처치 후 중앙(RewardSpawnPoint)에 스폰할 보상 아이템 클래스 (예: BP_DimensionMarble_0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward")
	TSubclassOf<AActor> RewardItemClass;

	// 보스 사망 후 보상 아이템이 스폰될 때까지의 연출 딜레이 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward", meta = (ClampMin = "0.0"))
	float RewardSpawnDelay = 1.5f;

	// 구슬 조각 스폰 시 상공 시작 높이 오프셋 (기본: +350.0 유닛)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward", meta = (ClampMin = "0.0"))
	float RewardSpawnHeightOffset = 350.0f;

	// 구슬 조각이 상공에서 목표 위치까지 천천히 하강하는 시간 (초, 기본: 3.0초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward", meta = (ClampMin = "0.5"))
	float RewardDescentDuration = 3.0f;

	// 구슬 조각의 최종 부유 높이 (바닥 기준 Z 오프셋, 기본: +70.0 유닛)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward")
	float RewardTargetFloatHeight = 70.0f;

	// 구슬 조각 하강 중 자전 회전 속도 (도/초, 기본: 60.0도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward")
	float RewardDescentRotationSpeed = 60.0f;

	// 보상 아이템 스폰 시 재생할 나이아가라 이펙트 (선택 사항)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward")
	UNiagaraSystem* RewardSpawnEffect = nullptr;

	// 보상 아이템 스폰 시 재생할 효과음 (선택 사항)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner|Boss Reward")
	USoundBase* RewardSpawnSound = nullptr;

	// 포털을 보상 아이템이 스폰되었던 중앙 위치(RewardSpawnPoint)에 소환할지 여부 (기본값: true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner")
	bool bSpawnPortalAtRewardLocation = true;

	// 포털 소환 후 오버랩(텔레포트)이 활성화될 때까지의 안전 대기 시간 (초, 기본: 1.5초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Spawner", meta = (ClampMin = "0.0"))
	float PortalActivationDelay = 1.5f;

	// 플레이어 앞쪽으로 소환할 거리 (bSpawnPortalAtRewardLocation이 false일 때 사용)
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	float SpawnOffsetDistance = 300.0f;

	// 포털 소환 시 플레이어와의 최소 안전 거리 (300 반경 내 플레이어가 있으면 다른 위치 탐색)
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	float MinPlayerDistance = 300.0f;

	// 소환된 포털이 플레이어를 바라보게 회전할지 여부
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	bool bFacePlayer = true;

	// 보스가 파괴되었을 때 호출될 콜백 함수
	UFUNCTION()
	void OnBossDestroyed(AActor* DestroyedActor);

	// 플레이어가 아이템을 획득했을 때 호출될 콜백 함수
	UFUNCTION()
	void OnPlayerItemAdded(int32 SlotIndex, const FStruct_ItemData& ItemData);

	// 구슬 조각 하강 실시간 갱신 함수 (60fps 타이머)
	UFUNCTION()
	void UpdateRewardDescent();

	// 포털 오버랩을 안전하게 활성화하는 함수
	UFUNCTION()
	void ActivatePortalOverlap();

private:
	// 플레이어와 최소 안전 거리를 유지하는 유효한 스폰 위치를 다각도로 탐색
	FVector FindSafeSpawnLocation(const FVector& PlayerLocation, const FVector& PlayerForward, const FVector& PlayerRight) const;

	// 목표 위치를 콜리전 박스 범위 내로 클램핑하고 바닥/NavMesh 위치로 보정
	FVector CalculateClampedSpawnLocation(const FVector& DesiredLocation) const;

	// 포털을 실제 월드에 소환하는 로직
	void SpawnPortal(const FVector& BaseLocation, const FRotator& BaseRotation);

	// 해당 아이템이 이미 드롭되었거나 인벤토리에 소지 중인지 확인
	bool IsItemAlreadyDroppedOrAcquired() const;

	// 1회성 드롭 고유 키 생성
	FString GetUniqueRewardKey() const;

	// 바인딩된 인벤토리 컴포넌트 캐싱
	UPROPERTY()
	UAGSDInventoryComponent* BoundInventoryComp = nullptr;

	// 스폰된 보상 구슬 액터 약참조
	UPROPERTY()
	TWeakObjectPtr<AActor> SpawnedRewardActor = nullptr;

	// 스폰된 포털 액터 약참조
	UPROPERTY()
	TWeakObjectPtr<AActor> SpawnedPortalActor = nullptr;

	// 구슬 하강 시작 위치 및 최종 목표 위치
	FVector RewardStartLocation = FVector::ZeroVector;
	FVector RewardTargetLocation = FVector::ZeroVector;
	float DescentElapsedTime = 0.0f;

	// 타이머 핸들들
	FTimerHandle RewardSpawnTimerHandle;
	FTimerHandle RewardDescentTimerHandle;
	FTimerHandle PortalActivationTimerHandle;

	// 보스 처치 여부
	bool bBossDefeated = false;

	// 보상 아이템이 이미 스폰되었는지 여부
	bool bRewardSpawned = false;

	// 포털이 이미 소환되었는지 여부
	bool bPortalSpawned = false;
};
