#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyProjectile/BaseEnemyProjectile.h"
#include "GroundAttackProjectile.generated.h"

UCLASS()
class ENEMY_API AGroundAttackProjectile : public AActor
{
	GENERATED_BODY()

	FVector InitialLocation;
	FVector PeakLocation;
	float RiseElapsedTime = 0.f;
	float StayElapsedTime = 0.f;
	float LowerElapsedTime = 0.f;

	bool bIsRising = false;
	bool bIsStaying = false;
	bool bIsLowering = false;

	void UpperMesh(float DeltaTime);
	void LowerMesh(float DeltaTime);
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category="자체설정")
	float Damage = 20.f; // 데미지

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* RootComp;
	
	// 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComp;
	
public:	
	AGroundAttackProjectile();
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
	
	void HandleDamage();

	UPROPERTY(BlueprintReadWrite, Category="CombatLog")
	FString ActionID;

	bool bHasHitPlayer = false;

	virtual void Destroyed() override;
	
	// 생성 후 상승할 높이
	UPROPERTY(EditAnywhere, Category="자체설정")
	float RiseHeight = 20.f;
	// 상승하는 데 걸리는 시간
	UPROPERTY(EditAnywhere, Category="자체설정")
	float RiseDuration = 0.2f;
	// 하강에 걸리는 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LowerDuration = 1.f; 
	// 장판 유지 시간
	UPROPERTY(EditAnywhere, Category="자체설정")
	float DurationTime = 5.f;
};
