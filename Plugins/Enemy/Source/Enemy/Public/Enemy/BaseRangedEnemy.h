#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "BaseRangedEnemy.generated.h"


UCLASS()
class ENEMY_API ABaseRangedEnemy : public ABaseEnemy
{
	GENERATED_BODY()

protected:
	
	// 원거리 공격 타입일 때 발사체 클래스
	UPROPERTY(EditDefaultsOnly, Category="자체설정")
	TSubclassOf<class ABaseEnemyProjectile> ProjectileClass;

	// 원거리 공격 지점 컴포넌트
	UPROPERTY(VisibleAnywhere)
	class USceneComponent* RangedAttackPoint;

	// 원거리 공격 액션 이름 (기본값: Arrow)
	UPROPERTY(EditDefaultsOnly, Category="자체설정|전투로그")
	FString ProjectileActionName = TEXT("Arrow");
	
public:
	ABaseRangedEnemy();

	UFUNCTION(BlueprintCallable)
	void ShootProjectile();
};
