// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Struct_ItemData.h"
#include "PortalSpawner.generated.h"

class UBoxComponent;
class UAGSDInventoryComponent;

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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 레벨에서 포털 스폰 범위를 지정하는 콜리전 박스 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Spawner")
	UBoxComponent* SpawnAreaBox;

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

	// 플레이어 앞쪽으로 소환할 거리
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

private:
	// 플레이어와 최소 안전 거리를 유지하는 유효한 스폰 위치를 다각도로 탐색
	FVector FindSafeSpawnLocation(const FVector& PlayerLocation, const FVector& PlayerForward, const FVector& PlayerRight) const;

	// 목표 위치를 콜리전 박스 범위 내로 클램핑하고 바닥/NavMesh 위치로 보정
	FVector CalculateClampedSpawnLocation(const FVector& DesiredLocation) const;

	// 포털을 실제 월드에 소환하는 로직
	void SpawnPortal(const FVector& BaseLocation, const FRotator& BaseRotation);

	// 해당 아이템이 이미 드롭되었거나 인벤토리에 소지 중인지 확인
	bool IsItemAlreadyDroppedOrAcquired() const;

	// 바인딩된 인벤토리 컴포넌트 캐싱
	UPROPERTY()
	UAGSDInventoryComponent* BoundInventoryComp = nullptr;

	// 보스 처치 여부
	bool bBossDefeated = false;

	// 포털이 이미 소환되었는지 여부
	bool bPortalSpawned = false;
};
