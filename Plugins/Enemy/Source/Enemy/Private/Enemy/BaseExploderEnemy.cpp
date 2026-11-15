#include "Enemy/BaseExploderEnemy.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseExploderEnemy::ABaseExploderEnemy()
{
	ExplosionRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionRangeSphere"));
	ExplosionRangeSphere->SetupAttachment(RootComponent); // 루트 컴포넌트
	ExplosionRangeSphere->ShapeColor = FColor::Yellow;
	ExplosionRangeSphere->SetVisibility(false);
	ExplosionRangeSphere->SetHiddenInGame(false);


	EnemyType = EEnemyType::EET_Exploder;
}

void ABaseExploderEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		HopMoveSpeed = MoveComp->MaxWalkSpeed > 0.0f ? MoveComp->MaxWalkSpeed : MoveSpeed;
	}
}

void ABaseExploderEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bUseHopMovement)
	{
		return;
	}

	// 점프 공중 구간에서만 이동 허용, 착지/대기 구간에서는 제자리 (MaxWalkSpeed 0 → 감속 정지)
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	MoveComp->MaxWalkSpeed = bHopMoveWindowActive ? HopMoveSpeed : 0.0f;

	// 공중 구간은 짧아서 가속(MaxAcceleration)으로는 목표 속도에 도달하기 전에 끝남
	// → 구간 동안 경로 방향으로 수평 속도를 즉시 HopMoveSpeed로 설정 (톡 튀어나가는 점프)
	if (bHopMoveWindowActive && IsAIRequestingMove())
	{
		const AAIController* AIController = Cast<AAIController>(GetController());
		const UPathFollowingComponent* PathComp = AIController ? AIController->GetPathFollowingComponent() : nullptr;

		FVector HopDir = PathComp ? PathComp->GetCurrentDirection() : FVector::ZeroVector;
		HopDir.Z = 0.0f;
		if (HopDir.Normalize())
		{
			MoveComp->Velocity.X = HopDir.X * HopMoveSpeed;
			MoveComp->Velocity.Y = HopDir.Y * HopMoveSpeed;
		}
	}
}

FVector ABaseExploderEnemy::GetVelocity() const
{
	const FVector RealVelocity = Super::GetVelocity();

	// 실제 속도가 0이어도 AI가 이동 중이면 애님(Speed)에 이동 속도를 전달해 점프 루프 유지
	if (bUseHopMovement && IsAIRequestingMove() && RealVelocity.SizeSquared2D() < FMath::Square(HopMoveSpeed))
	{
		return GetActorForwardVector() * HopMoveSpeed + FVector(0.0f, 0.0f, RealVelocity.Z);
	}

	return RealVelocity;
}

bool ABaseExploderEnemy::IsAIRequestingMove() const
{
	if (const AAIController* AIController = Cast<AAIController>(GetController()))
	{
		return AIController->GetMoveStatus() == EPathFollowingStatus::Moving;
	}
	return false;
}

void ABaseExploderEnemy::Explode()
{
	// 이미 플레이어에게 처치된 상태면 폭발/중복 사망 처리하지 않음 (처치 드롭 유지)
	if (Health <= 0.f)
	{
		return;
	}

	// 살아있는 상태에서의 자폭 → 플레이어가 직접 잡은 게 아니므로 아이템 드롭 없음
	bSuppressItemDrop = true;

	// 한 프레임 오버랩 검사: ExplosionRangeSphere와 겹치는 액터들을 가져와 태그로 필터링
	if (ExplosionRangeSphere)
	{
		TArray<AActor*> OverlappingActors;
		ExplosionRangeSphere->GetOverlappingActors(OverlappingActors);

		for (AActor* Actor : OverlappingActors)
		{
			if (Actor && Actor->ActorHasTag(FName("Player")))
			{
				UE_LOG(LogTemp, Warning, TEXT("Explode overlap with Player: %s"), *Actor->GetName());

				// 로그 기록 로직
				if (GetMesh()) 
				{
					// 스켈레탈 메쉬 에셋 이름 가져오기
					FString MeshName = GetMesh()->GetSkeletalMeshAsset() ? GetMesh()->GetSkeletalMeshAsset()->GetName() : TEXT("NoMeshAsset");
				}
				
				EnemyLogData.TotalDamageDealt += ExplosionDamage; // 로그 데이터에 입힌 대미지 누적
				
				//액터에게 데미지 적용
				UGameplayStatics::ApplyDamage(
					Actor,
					ExplosionDamage,
					GetController(),
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
	
	if (ExplosionEffect && ExplosionRangeSphere && GetWorld() )
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionEffect,
			ExplosionRangeSphere->GetComponentLocation(),
			ExplosionRangeSphere->GetComponentRotation()
		);
	}
	Die(); // 폭발 후 죽음 처리
}

#if WITH_EDITOR
void ABaseExploderEnemy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 변경된 프로퍼티의 이름을 가져옵니다.
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
									? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseExploderEnemy, bDebugMode))
	{
		if ( bDebugMode == true )
		{
			if ( ExplosionRangeSphere ) ExplosionRangeSphere->SetVisibility(true);
		}
		else
		{
			if ( ExplosionRangeSphere ) ExplosionRangeSphere->SetVisibility(false);
		}
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseExploderEnemy, ExplosionRange))
	{
		if (ExplosionRangeSphere)
		{
			ExplosionRangeSphere->SetSphereRadius(ExplosionRange);
		}
	}
}
#endif

void ABaseExploderEnemy::InitializeStatsFromDataTable()
{
	Super::InitializeStatsFromDataTable();

	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())
	{
		ExplosionDamage = AttackDamage;
	}
}