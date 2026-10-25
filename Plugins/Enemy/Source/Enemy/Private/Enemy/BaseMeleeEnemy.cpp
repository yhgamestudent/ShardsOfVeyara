#include "Enemy/BaseMeleeEnemy.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "CombatTimelineLogger.h"
#include "CombatLogParticipantInterface.h"

ABaseMeleeEnemy::ABaseMeleeEnemy()
{
	MeleeAttackPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AttackPoint"));
	MeleeAttackPoint->SetupAttachment(RootComponent); // 루트 컴포넌트
	
	AttackRangePointSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangePointSphere"));
	AttackRangePointSphere->SetupAttachment(MeleeAttackPoint); // AttackPoint에 부착
	AttackRangePointSphere->ShapeColor = FColor::Purple;
	AttackRangePointSphere->SetVisibility(false);
	AttackRangePointSphere->SetHiddenInGame(false); 


	EnemyType = EEnemyType::EET_Melee;
}

void ABaseMeleeEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckMeleeAttackHit(DeltaTime);
}

void ABaseMeleeEnemy::CheckMeleeAttackHit(float DeltaTime)
{
	if (bIsMeleeAttacking == true ) // 공격 중일 때
	{
		TArray<AActor*> OverlappingActors;
		// AttackRangePointSphere와 겹치는 모든 액터를 가져옵니다.
		AttackRangePointSphere->GetOverlappingActors(OverlappingActors);
		
		for (AActor* OverlappingActor : OverlappingActors)
		{
			// 액터가 유효하고 "Player" 태그를 가지고 있으며, 아직 공격한 목록에 없는지 확인합니다.
			if (OverlappingActor && OverlappingActor->ActorHasTag(FName("Player")) && !HittedActors.Contains(OverlappingActor))
			{
				// 공격 로그를 출력합니다.
				UE_LOG(LogTemp, Warning, TEXT("Attack Hit Detected on: %s"), *OverlappingActor->GetName());

				// 로그 기록 로직
				if (GetMesh()) 
				{
					// 스켈레탈 메쉬 에셋 이름 가져오기
					FString MeshName = GetMesh()->GetSkeletalMeshAsset() ? GetMesh()->GetSkeletalMeshAsset()->GetName() : TEXT("NoMeshAsset");
				}
				
				NotifyDamageDealtToPlayer(MeleeAttackDamage); // 로그 데이터 및 소환자 보스에게 입힌 대미지 누적
				
				// ─── [고정밀 전투 타임라인 로깅: 근접 미니언 공격 적중/가드] ───
				if (UCombatTimelineLogger::IsSessionActive())
				{
					float PlayerHP = 0.0f;
					float PlayerMaxHP = 0.0f;
					FString TargetCombatState = TEXT("Idle");
					if (const ICombatLogParticipantInterface* Participant = Cast<ICombatLogParticipantInterface>(OverlappingActor))
					{
						PlayerHP = Participant->GetParticipantHealth();
						PlayerMaxHP = Participant->GetParticipantMaxHealth();
						TargetCombatState = Participant->GetParticipantCombatState();
					}

					const float Dist = FVector::Dist(GetActorLocation(), OverlappingActor->GetActorLocation());
					const float CurrentTime = GetWorld()->GetTimeSeconds();
					const FString ActionName = TEXT("Minion_MeleeAttack");

					if (TargetCombatState == TEXT("JustGuarding"))
					{
						UCombatTimelineLogger::LogHitJustGuard(ActionName, ActionName, CurrentTime, Dist, 0.0f, 0.0f, PlayerHP, PlayerMaxHP);
					}
					else if (TargetCombatState == TEXT("Guarding"))
					{
						UCombatTimelineLogger::LogHitGuarded(ActionName, ActionName, MeleeAttackDamage, CurrentTime, Dist, 0.0f, 0.0f, PlayerHP, PlayerMaxHP);
					}
					else
					{
						const float EstimatedAfterHP = FMath::Max(0.0f, PlayerHP - MeleeAttackDamage);
						UCombatTimelineLogger::LogHitSuccess(ActionName, ActionName, MeleeAttackDamage, CurrentTime, Dist, TargetCombatState, 0.0f, 0.0f, EstimatedAfterHP, PlayerMaxHP);
					}
				}

				// 플레이어에게 대미지를 적용합니다.
				UGameplayStatics::ApplyDamage(
					OverlappingActor,
					MeleeAttackDamage, // 헤더 파일에 선언된 대미지 변수
					GetController(),
					this,
					UDamageType::StaticClass()
				);
				
				// 공격한 목록에 추가하여 중복 피해를 방지합니다.
				HittedActors.Add(OverlappingActor);

				bIsMeleeAttacking = false; // 공격 상태를 종료합니다.
			}
		}
	}
}

#if WITH_EDITOR
void ABaseMeleeEnemy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 변경된 프로퍼티의 이름을 가져옵니다.
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	// 디버그 모드에 따라 어택, 디텍트, 체이스 범위 구체의 가시성을 설정합니다.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseMeleeEnemy, bDebugMode))
	{
		if ( bDebugMode == true )
		{
			if ( AttackRangePointSphere ) AttackRangePointSphere->SetVisibility(true);
		}
		else
		{
			if ( AttackRangePointSphere ) AttackRangePointSphere->SetVisibility(false);
		}
	}
}
#endif

void ABaseMeleeEnemy::InitializeStatsFromDataTable()
{
	Super::InitializeStatsFromDataTable();

	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())
	{
		MeleeAttackDamage = AttackDamage;
	}
}