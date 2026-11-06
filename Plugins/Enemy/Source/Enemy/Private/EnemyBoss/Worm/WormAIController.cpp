#include "EnemyBoss/Worm/WormAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyBoss/Worm/BossWorm.h"

void AWormAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	ControlledEnemy = Cast<ABossWorm>(GetPawn());
}

void AWormAIController::SetBlackboardKey()
{
	Super::SetBlackboardKey();

	if ( BlackboardComp == nullptr || ControlledEnemy == nullptr ) return;

	BlackboardComp->SetValueAsBool("CanAttack", true);
	BlackboardComp->SetValueAsFloat("AttackDelay", ControlledEnemy->AttackDelay);

	BlackboardComp->SetValueAsObject(TEXT("TargetCharacter"), TargetPawn);
	BlackboardComp->SetValueAsBool( TEXT("bIsBurrowing"), ControlledEnemy->bIsBurrowing);
	BlackboardComp->SetValueAsFloat(TEXT("W_NormalAttack"), ControlledEnemy->AttackStruct.NormalAttackWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_Burrow"), ControlledEnemy->AttackStruct.BurrowWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_Unburrow"), ControlledEnemy->AttackStruct.UnBurrowWeight);	
	BlackboardComp->SetValueAsFloat(TEXT("W_RangedAttack"), ControlledEnemy->AttackStruct.RangedAttackWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_LungeAttack"), ControlledEnemy->AttackStruct.LungeAttackWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_LinearFireBreath"), ControlledEnemy->AttackStruct.LinearFireBreathWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_FanFireBreath"), ControlledEnemy->AttackStruct.FanFireBreathWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_Suction"), ControlledEnemy->AttackStruct.SuctionAttackWeight);
}

void AWormAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 플레이어와의 거리에 따라 공격 가중치 실시간 조절 (근접: 평타/부채꼴브레스, 원거리: 런지/석션/직선브레스)
	if (BlackboardComp && ControlledEnemy && TargetPawn)
	{
		const float Dist = FVector::Dist2D(ControlledEnemy->GetActorLocation(), TargetPawn->GetActorLocation());
		const float MinLungeDist = ControlledEnemy->AttackStruct.MinLungeDistance;
		if (Dist < MinLungeDist)
		{
			// 근접 거리 (MinLungeDistance 미만): 런지와 석션, 곡사 원거리 공격 완전 봉인!
			BlackboardComp->SetValueAsFloat(TEXT("W_Suction"), 0.0f);
			BlackboardComp->SetValueAsFloat(TEXT("W_LungeAttack"), 0.0f);
			BlackboardComp->SetValueAsFloat(TEXT("W_RangedAttack"), 0.0f); // 근접 곡사 박격포 봉인
			BlackboardComp->SetValueAsFloat(TEXT("W_NormalAttack"), 4.0f);
			BlackboardComp->SetValueAsFloat(TEXT("W_FanFireBreath"), 4.0f); // 4타 구르기 캐치용 부채꼴 브레스
			BlackboardComp->SetValueAsFloat(TEXT("W_LinearFireBreath"), 2.0f);
		}
		else
		{
			// 중/원거리 (MinLungeDistance 이상): 도약 런지, 원거리 석션(끌어당기기), 직선 브레스, 곡사 박격포 공격 활성화
			BlackboardComp->SetValueAsFloat(TEXT("W_NormalAttack"), 0.0f);  // 원거리 평타 봉인
			BlackboardComp->SetValueAsFloat(TEXT("W_FanFireBreath"), 0.0f); // 근접 브레스 낭비 방지
			BlackboardComp->SetValueAsFloat(TEXT("W_Suction"), 4.0f);       // 원거리 플레이어 정상 끌어당기기
			BlackboardComp->SetValueAsFloat(TEXT("W_LungeAttack"), 4.0f);  // 원거리 도약 런지 가중치 상향 (적극적 거리 좁히기)
			BlackboardComp->SetValueAsFloat(TEXT("W_RangedAttack"), ControlledEnemy->AttackStruct.RangedAttackWeight); // 원거리 박격포 투사체 적극 시전
			BlackboardComp->SetValueAsFloat(TEXT("W_LinearFireBreath"), 3.0f);
		}
	}
}