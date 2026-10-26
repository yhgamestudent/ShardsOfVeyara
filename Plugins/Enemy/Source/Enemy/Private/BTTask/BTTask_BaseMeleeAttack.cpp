#include "BTTask/BTTask_BaseMeleeAttack.h"
#include "AIController.h"
#include "BaseEnemy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

UBTTask_BaseMeleeAttack::UBTTask_BaseMeleeAttack()
{
	NodeName = "BaseMeleeAttack"; 
}

EBTNodeResult::Type UBTTask_BaseMeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	if ( OwnerComp.GetAIOwner() == nullptr ) return EBTNodeResult::Failed;

	ABaseEnemy* Enemy = Cast<ABaseEnemy>( OwnerComp.GetAIOwner()->GetPawn() );
	if ( Enemy == nullptr ) return EBTNodeResult::Failed;

	// 공격 대상 확인 (블랙보드 TargetCharacter 우선, 없으면 Enemy->TargetCharacter 혹은 플레이어 폰)
	AActor* TargetActor = nullptr;
	if ( UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent() )
	{
		TargetActor = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetCharacter")));
	}
	if ( !TargetActor )
	{
		TargetActor = Enemy->TargetCharacter;
	}
	if ( !TargetActor )
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(Enemy->GetWorld(), 0);
	}

	// 대상과의 거리 검사: 공격 유효 사거리를 초과하여 멀리 도망친 경우 헛공격 실행 방지
	if ( TargetActor )
	{
		const float Dist = FVector::Dist2D(Enemy->GetActorLocation(), TargetActor->GetActorLocation());
		const float MaxAllowedAttackRange = FMath::Max(Enemy->AttackRange * 1.5f, 250.0f);
		if ( Dist > MaxAllowedAttackRange )
		{
			return EBTNodeResult::Failed;
		}
	}

	Enemy->Attack();// 적의 공격 함수 호출
	
	return EBTNodeResult::Succeeded;
}


