#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestHealActor.generated.h"

UCLASS()
class AGSD_API ATestHealActor : public AActor
{
	GENERATED_BODY()

protected:
	// 게임 시작 시 호출
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category="자체설정")
	bool bIsKillMode = false;
	
public:	
	ATestHealActor();
	// 매 프레임 호출
	virtual void Tick(float DeltaTime) override;

	/** 플레이어 감지용 스피어 콜리전 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	/** 충돌 시작 시 호출될 함수 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
