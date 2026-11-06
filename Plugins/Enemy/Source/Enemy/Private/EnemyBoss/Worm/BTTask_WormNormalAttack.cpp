#include "EnemyBoss/Worm/BTTask_WormNormalAttack.h"

#include "AIController.h"
#include "EnemyBoss/Worm/BossWorm.h"
#include "Kismet/GameplayStatics.h"

UBTTask_WormNormalAttack::UBTTask_WormNormalAttack()
{
	NodeName = "WormNormalAttack";

	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_WormNormalAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ABossWorm* Boss = Cast<ABossWorm>(AIController ? AIController->GetPawn() : nullptr);

	if (Boss == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// 이중 안전장치: 플레이어와의 거리가 MinLungeDistance 이상이면 원거리 평타 실행 방지 (헛손질 원천 차단)
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Boss->GetWorld(), 0);
	if (PlayerPawn)
	{
		const float Dist = FVector::Dist2D(Boss->GetActorLocation(), PlayerPawn->GetActorLocation());
		if (Dist >= Boss->AttackStruct.MinLungeDistance)
		{
			UE_LOG(LogTemp, Log, TEXT("[BossWorm AI] Player too far (%.1fcm >= %.1fcm). Normal attack cancelled."),
				Dist, Boss->AttackStruct.MinLungeDistance);
			return EBTNodeResult::Failed;
		}
	}
	
	// 1. 공격 함수 실행 (몽타주 재생 시작)
	Boss->NormalAttack();
	bIsAttacking = true;

	// 2. 중요: "성공(Succeeded)"을 바로 리턴하면 안 됩니다!
	// 몽타주가 끝날 때까지 기다려야 하므로 "진행 중(InProgress)"을 리턴합니다.
	return EBTNodeResult::InProgress;
}

void UBTTask_WormNormalAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 공격 중이 아니면 틱을 돌 필요가 없음
	if (!bIsAttacking) return;
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	ABossWorm* Boss = Cast<ABossWorm>(AIController->GetPawn());

	// Boss가 유효하고, 몽타주 재생이 끝났는지 확인
	// (Montage_IsPlaying은 지정한 몽타주가 재생 중이면 true, 아니면 false)
	if (Boss && !Boss->GetMesh()->GetAnimInstance()->Montage_IsPlaying(Boss->NormalAttackMontage))
	{
		// 3. 몽타주가 끝났으므로 태스크 종료를 선언합니다.
		bIsAttacking = false;
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
