// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalSpawner.generated.h"

UCLASS()
class AGSD_API APortalSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	APortalSpawner();

protected:
	virtual void BeginPlay() override;

	// 감시할 보스 액터 (인스턴스에서 지정 가능)
	UPROPERTY(EditInstanceOnly, Category = "Portal Spawner")
	AActor* TargetBoss;

	// 소환할 포털 클래스
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	TSubclassOf<AActor> PortalClass;

	// 보스 뒤쪽으로 소환할 거리
	UPROPERTY(EditAnywhere, Category = "Portal Spawner")
	float SpawnOffsetDistance = 300.0f;

	// 보스가 파괴되었을 때 호출될 콜백 함수
	UFUNCTION()
	void OnBossDestroyed(AActor* DestroyedActor);

private:
	// 포털을 실제 월드에 소환하는 로직
	void SpawnPortal(const FVector& BaseLocation, const FRotator& BaseRotation);
};
