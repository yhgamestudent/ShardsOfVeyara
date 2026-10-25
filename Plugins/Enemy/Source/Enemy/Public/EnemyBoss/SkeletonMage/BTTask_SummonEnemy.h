#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_SummonEnemy.generated.h"


UCLASS()
class ENEMY_API UBTTask_SummonEnemy : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

	bool bIsAttacking = false;
	
public:
	UBTTask_SummonEnemy();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
