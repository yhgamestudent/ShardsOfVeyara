// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalSpawner.generated.h"

class UBoxComponent;

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

	// 레벨에서 포털 스폰 범위를 지정하는 콜리전 박스 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Spawner")
	UBoxComponent* SpawnAreaBox;

	// 감시할 보스 액터 (인스턴스에서 지정 가능)
	UPROPERTY(EditInstanceOnly, Category = "Portal Spawner")
	AActor* TargetBoss;

	// 소환할 포털 클래스
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	TSubclassOf<AActor> PortalClass;

	// 플레이어 앞쪽으로 소환할 거리
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	float SpawnOffsetDistance = 300.0f;

	// 소환된 포털이 플레이어를 바라보게 회전할지 여부
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	bool bFacePlayer = true;

	// 보스가 파괴되었을 때 호출될 콜백 함수
	UFUNCTION()
	void OnBossDestroyed(AActor* DestroyedActor);

private:
	// 목표 위치를 콜리전 박스 범위 내로 클램핑하고 바닥/NavMesh 위치로 보정
	FVector CalculateClampedSpawnLocation(const FVector& DesiredLocation) const;

	// 포털을 실제 월드에 소환하는 로직
	void SpawnPortal(const FVector& BaseLocation, const FRotator& BaseRotation);
};
