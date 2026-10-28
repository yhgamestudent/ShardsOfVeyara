#include "EnemyBoss/SkeletonMage/BossSkeletonMage.h"
#include "BaseEnemy.h"
#include "Enemy/BaseMeleeEnemy.h"
#include "CSVLog.h"
#include "CombatTimelineLogger.h"
#include "CombatLogParticipantInterface.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "EnemyProjectile/BaseEnemyProjectile.h"
#include "EnemyProjectile/GroundAttackProjectile.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h" // 헤더 파일 추가
#include "Components/CapsuleComponent.h" 
#include "Engine/OverlapResult.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"


ABossSkeletonMage::ABossSkeletonMage()
{
	SummonPointOne = CreateDefaultSubobject<USceneComponent>(TEXT("SummonPointOne"));
	SummonPointOne->SetupAttachment(GetRootComponent());

	SummonPointTwo = CreateDefaultSubobject<USceneComponent>(TEXT("SummonPointTwo"));
	SummonPointTwo->SetupAttachment(GetRootComponent());

	PushAreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PushAreaSphere"));
	PushAreaSphere->SetupAttachment(GetRootComponent());
}

void ABossSkeletonMage::BeginPlay()
{
	Super::BeginPlay();
	
	CommonBossLogData.BossID = BossLogId; // 로그 데이터에 보스 ID 기록
	
	// 이 액터에 속한 모든 USceneComponent를 가져옵니다.
	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	// 이름으로 FireBallSpawnPoint 컴포넌트를 찾습니다.
	for (USceneComponent* Component : SceneComponents)
	{
		if (Component->GetFName() == TEXT("FireBallSpawnPoint"))
		{
			FireBallSpawnPoint = Component;
			break; // 찾았으면 반복 종료
		}
	}
}

void ABossSkeletonMage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 스킬 쿨타임 만료 검사 및 가중치 복원
	UpdateSkillCooldowns();

	TraceTargetCharacterForGroundAttackEffect(DeltaTime);
	
	HandleGravityAttack(DeltaTime);
	
	if (bIsAttacking == true ) // 공격 중일 때
	{
		TArray<AActor*> OverlappingActors;
		// 겹치는 모든 액터를 가져옵니다.
		PushAreaSphere->GetOverlappingActors(OverlappingActors);
		
		for (AActor* OverlappingActor : OverlappingActors)
		{
			// 액터가 유효하고 "Player" 태그를 가지고 있으며, 아직 공격한 목록에 없는지 확인합니다.
			if (OverlappingActor && OverlappingActor->ActorHasTag(FName("Player"))
				&& !HittedActors.Contains(OverlappingActor))
			{
				// 공격 로그를 출력합니다.
				UE_LOG(LogTemp, Warning, TEXT("Attack Hit Detected on: %s"), *OverlappingActor->GetName());

				// 공격한 목록에 추가하여 중복 피해를 방지합니다.
				HittedActors.Add(OverlappingActor);
				
				bIsAttacking = false; // 공격 상태를 종료합니다.

				CurrentSkillHitIntensity = AttackStruct.PushTargetHitIntensity;
				CurrentSkillKnockdownType = AttackStruct.PushTargetKnockdownType;
				CurrentSkillKnockdownDuration = AttackStruct.PushTargetKnockdownDuration; bHasExplicitSkillReaction = true;

				// --- 플레이어 밀치기 효과 시작 ---
				ACharacter* PlayerCharacter = Cast<ACharacter>(OverlappingActor);
				if (PlayerCharacter)
				{
					PlayerCharacter->StopAnimMontage(); // 현재 재생 중인 몽타주 중지
					
					// 2. 컨트롤러를 가져와서 이동 입력 무시 상태 해제
					if (APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController()))
					{
						PC->SetIgnoreMoveInput(false);
						// 필요하다면 시점 회전도 함께 풉니다.
						// PC->SetIgnoreLookInput(false);
					}
					
					// 1. 밀어낼 방향 계산 (보스 -> 플레이어)
					const FVector BossLoc = GetActorLocation();
					const FVector PlayerLoc = PlayerCharacter->GetActorLocation();
					FVector PushDirection = PlayerLoc - BossLoc;
					const float Dist2D = PushDirection.Size2D();
					PushDirection.Z = 0.f; // 수평 방향으로만 밀도록 Z값을 0으로 설정
					
					bool bDirValid = PushDirection.Normalize();
					if (!bDirValid || Dist2D < 10.f)
					{
						// 보스와 플레이어가 너무 가까워 방향이 0인 경우 보스의 정면으로 밀어냄
						PushDirection = GetActorForwardVector();
						PushDirection.Z = 0.f;
						PushDirection.Normalize();
					}

					// 2. 밀어낼 속도 계산 (방향 * 힘 + 위로 띄우는 힘)
					const FVector LaunchVelocity = PushDirection * PushForce + FVector(0.f, 0.f, PushUpwardForce);

					// 3. 지면 마찰력(Ground Friction)에 의한 감속 간섭을 차단하기 위해 공중 모드로 즉시 전환 후 발사
					if (UCharacterMovementComponent* MoveComp = PlayerCharacter->GetCharacterMovement())
					{
						MoveComp->SetMovementMode(MOVE_Falling);
					}

					// bXYOverride와 bZOverride를 true로 설정하여 현재 속도를 무시하고 새로운 속도를 적용합니다.
					PlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);

					// ─── [PushTarget 타임라인 로깅 (Release 및 Hit)] ───
					float PlayerHP = 0.0f;
					float PlayerMaxHP = 0.0f;
					FString TargetCombatState = TEXT("Attacking");
					float Dist = 0.0f;
					QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

					UCombatTimelineLogger::LogSkillRelease(
						CurrentPushTargetActionID,
						GetWorld()->GetTimeSeconds(),
						Dist,
						TargetCombatState,
						Health,
						MaxHealth,
						PlayerHP,
						PlayerMaxHP,
						TEXT("Shockwave Hit Player")
					);
					UCombatTimelineLogger::LogHitSuccess(
						CurrentPushTargetActionID,
						TEXT("PushTarget"),
						0.0f,
						GetWorld()->GetTimeSeconds(),
						Dist,
						TargetCombatState,
						Health,
						MaxHealth,
						PlayerHP,
						PlayerMaxHP
					);
				}
				// --- 플레이어 밀치기 효과 끝 ---

				// 이 아래에 이제 대미지 넣는거 추가 가능
			}
		}
	}

	// ─── [미니언 생존 중 매직 실드 결계 수명주기 체크] ───
	if (bIsMagicShieldActive)
	{
		CheckMinionSurvivors();
	}
}

void ABossSkeletonMage::Die()
{
	Super::Die();
	
	// 1. 모든 중력 관련 타이머 즉시 완전 취소
	GetWorldTimerManager().ClearTimer(GravityActivationTimerHandle);
	GetWorldTimerManager().ClearTimer(GravityImpactEffectTimerHandle);
	GetWorldTimerManager().ClearTimer(GravityEndTimerHandle);
	bIsGravityAttackActive = false;

	// 2. 월드 플레이어 완전 탐색 및 표준 수치 강제 주입 (오염된 백업 변수 대신 1.0f, 400.f 직접 복원)
	TArray<AActor*> FoundPlayers;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Player"), FoundPlayers);
	if (ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		FoundPlayers.AddUnique(PlayerPawn);
	}
	if (TargetCharacter)
	{
		FoundPlayers.AddUnique(TargetCharacter);
	}

	for (AActor* PlayerActor : FoundPlayers)
	{
		if (ACharacter* Char = Cast<ACharacter>(PlayerActor))
		{
			if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
			{
				MoveComp->GravityScale = 1.0f;
				MoveComp->AirControl = 0.05f;
				MoveComp->MaxWalkSpeed = 400.0f;

				// 공중에 붕 뜨며 끌려 올라가던 상향 관성을 제거하고 낙하 모드로 강제 전환
				if (MoveComp->IsFalling() || MoveComp->Velocity.Z > 0.f)
				{
					MoveComp->Velocity.Z = FMath::Min(MoveComp->Velocity.Z, 0.f);
					MoveComp->SetMovementMode(MOVE_Falling);
				}
			}
		}
	}

	// 타겟팅 이펙트가 존재하면 제거합니다.
	if (GroundTargetingComponent && GroundTargetingComponent->IsValidLowLevel())
	{
		GroundTargetingComponent->DestroyComponent();
		GroundTargetingComponent = nullptr;
	}

	// 마법 결계 이펙트 제거
	if (ActiveMagicShieldComponent)
	{
		ActiveMagicShieldComponent->DestroyComponent();
		ActiveMagicShieldComponent = nullptr;
	}
	bIsMagicShieldActive = false;
}

void ABossSkeletonMage::SpawnDeadEffectAndDestroy()
{
	// 주변 "Enemy" 태그를 가진 액터들에게 대미지 적용
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape SearchSphere = FCollisionShape::MakeSphere(3000.f); // 범위 3000 유닛
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // 자기 자신은 제외
	
	if (GetWorld()->OverlapMultiByChannel(OverlapResults, GetActorLocation(), FQuat::Identity, ECC_Pawn, SearchSphere, QueryParams))
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* HitActor = Result.GetActor();
			
			// 액터가 유효하고 "Enemy" 태그를 가지고 있는지 확인
			if (HitActor && HitActor->ActorHasTag(FName("Enemy")))
			{
				UGameplayStatics::ApplyDamage(
					HitActor, 
					1000.f,           // 대미지 1000
					GetController(),  // 대미지 원인 컨트롤러
					this,             // 대미지 가해자
					UDamageType::StaticClass()
				);
			}
		}
	}

	Super::SpawnDeadEffectAndDestroy();
}

float ABossSkeletonMage::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsMagicShieldActive)
	{
		// 소환수 생존 여부 재검증
		CheckMinionSurvivors();

		if (bIsMagicShieldActive)
		{
			// 공격자 캐릭터 추출 (직접 공격자, 투사체의 소유자 또는 가해자)
			ACharacter* AttackerCharacter = nullptr;
			if (DamageCauser)
			{
				if (ACharacter* Char = Cast<ACharacter>(DamageCauser))
				{
					AttackerCharacter = Char;
				}
				else if (AActor* Inst = DamageCauser->GetInstigator())
				{
					AttackerCharacter = Cast<ACharacter>(Inst);
				}
				else if (AActor* OwnerActor = DamageCauser->GetOwner())
				{
					AttackerCharacter = Cast<ACharacter>(OwnerActor);
				}
			}
			if (!AttackerCharacter)
			{
				AttackerCharacter = TargetCharacter.Get() ? TargetCharacter.Get() : UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
			}

			if (AttackerCharacter)
			{
				const FVector BossLoc = GetActorLocation();
				const FVector AttackerLoc = AttackerCharacter->GetActorLocation();
				FVector PushDirection = AttackerLoc - BossLoc;
				PushDirection.Z = 0.f;
				if (!PushDirection.Normalize() || PushDirection.IsNearlyZero())
				{
					PushDirection = GetActorForwardVector();
					PushDirection.Z = 0.f;
					PushDirection.Normalize();
				}

				// 공격자에게 즉시 반사 넉백 적용
				const FVector LaunchVelocity = PushDirection * (PushForce * 1.1f) + FVector(0.f, 0.f, PushUpwardForce * 0.9f);
				if (UCharacterMovementComponent* MoveComp = AttackerCharacter->GetCharacterMovement())
				{
					MoveComp->SetMovementMode(MOVE_Falling);
				}
				AttackerCharacter->LaunchCharacter(LaunchVelocity, true, true);

				// 피격 위치 및 방향으로 반사 충격파 나이아가라 이펙트 스폰
				if (MagicShieldEffect)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						MagicShieldEffect,
						BossLoc + FVector(0.f, 0.f, 10.f),
						PushDirection.Rotation(),
						FVector(1.0f)
					);
				}

				UE_LOG(LogTemp, Log, TEXT("[BossSkeletonMage] Attack reflected by Magic Shield! Attacker %s knocked back."), *AttackerCharacter->GetName());
			}

			// 소환수 생존 중 완전 무적 (데미지 0 차단)
			return 0.0f;
		}
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 1. 체력이 0보다 크고, 아직 2페이즈가 시작되지 않았는지 확인
	if (Health > 0.f && bIsSecondPhaseStarted == false  )
	{
		// 2. 현재 체력 비율 계산
		float CurrentHealthRatio = Health / MaxHealth;

		// 3. 설정된 임계값보다 낮아졌는지 체크
		if (CurrentHealthRatio <= SecondPhaseHealthThreshold)
		{
			bIsSecondPhaseStarted = true; // 중복 실행 방지 플래그 설정

			// ─── [2페이즈 전환 타임라인 기록] ───
			float PlayerHP = 0.0f;
			float PlayerMaxHP = 0.0f;
			FString TargetCombatState = TEXT("Idle");
			float Dist = 0.0f;
			QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

			UCombatTimelineLogger::LogPhaseTransition(
				TEXT("Phase2_Start"),
				GetWorld()->GetTimeSeconds(),
				Health,
				MaxHealth,
				PlayerHP,
				PlayerMaxHP,
				TEXT("Health dropped below 50% Threshold")
			);

			StartSecondPhase();          // 특수 패턴 함수 실행
		}
	}
	
	return ActualDamage;
}

void ABossSkeletonMage::PlayTeleportMontage(const FVector& Destination)
{
	if ( BlackboardComp == nullptr ) return;
	
	StartSkillCooldown(TEXT("Teleport"));

	TeleportDestination = Destination;
	
	BossSkeletonMageLogData.TeleportCount++;

	// ─── [텔레포트 선딜레이 시작 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	CurrentTeleportActionID = UCombatTimelineLogger::LogSkillWindup(
		TEXT("Teleport"),
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP
	);
	
	if ( TeleportMontage )
	{
		PlayAnimMontage(TeleportMontage);
		
		// 현재 위치에 텔레포트 입장 이펙트 생성
		if ( TeleportInEffect ) SpawnTeleportEffectAtLocation(GetActorLocation(), TeleportInEffect); 
	}
}

// 애님 노티파이에서 호출할 함수
void ABossSkeletonMage::TeleportMoveToNextPoint()
{
	if (TeleportDestination != FVector::ZeroVector)
	{
		// ─── [텔레포트 이동 완료 (Release / Recovery) 로깅] ───
		float PlayerHP = 0.0f;
		float PlayerMaxHP = 0.0f;
		FString TargetCombatState = TEXT("Idle");
		float Dist = 0.0f;
		QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

		UCombatTimelineLogger::LogSkillRelease(
			CurrentTeleportActionID,
			GetWorld()->GetTimeSeconds(),
			Dist,
			TargetCombatState,
			Health,
			MaxHealth,
			PlayerHP,
			PlayerMaxHP,
			TEXT("Teleport to Target Point")
		);
		UCombatTimelineLogger::LogSkillRecovery(
			CurrentTeleportActionID,
			GetWorld()->GetTimeSeconds(),
			Health,
			MaxHealth,
			PlayerHP,
			PlayerMaxHP
		);

		// --- 수정된 부분: 캡슐 절반 높이만큼 위로 오프셋을 주어 바닥 위에 정상적으로 서도록 함 ---
		float SafeZOffset = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		FVector AdjustedDestination = TeleportDestination + FVector(0.f, 0.f, SafeZOffset);
		
		SetActorLocation(AdjustedDestination); // 조정된 위치로 텔레포트 이동
		// --------------------------------------------------------

		// 이펙트는 원래 위치나 필요에 따라 AdjustedDestination 사용
		if ( TeleportOutEffect ) SpawnTeleportEffectAtLocation(TeleportDestination, TeleportOutEffect); 
		
		// TargetCharacter를 찾아서 바라보도록 회전
		if (TargetCharacter)
		{
			const FVector TargetLocation = TargetCharacter->GetActorLocation();
			// 현재 위치에서 타겟 위치를 바라보는 회전값 계산
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetLocation);

			// Z축(Yaw) 회전만 적용하여 수평으로 바라보게 함
			SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));
		}

		// 텔레포트 도착 후 플레이어를 향해 파이어볼 확정 연계 (0.5초 딜레이)
		FTimerHandle FireBallComboTimer;
		GetWorldTimerManager().SetTimer(FireBallComboTimer, [this]()
		{
			if (IsValid(this) && TargetCharacter && (Health > 0.0f))
			{
				PlayFireBallMontage();
			}
		}, 0.5f, false);
		
	}
}

void ABossSkeletonMage::SpawnTeleportEffectAtLocation(const FVector& Location, class UNiagaraSystem* EffectToSpawn)
{
	if (TeleportOutEffect == nullptr) return;

	FVector SpawnLocation = Location;

	// 바닥을 찾기 위해 라인 트레이스를 수행합니다.
	FHitResult HitResult;
	const FVector StartTrace = Location + FVector(0.f, 0.f, 100.f); // 약간 위에서 시작
	const FVector EndTrace = Location - FVector(0.f, 0.f, 1000.f); // 아래로 1000 유닛
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if (TargetCharacter) TraceParams.AddIgnoredActor(TargetCharacter);

	// 2. 특정 태그를 가진 모든 액터 무시
	TArray<AActor*> PetActors;
	TArray<AActor*> EnemyActors;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Pet"), PetActors);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), EnemyActors);

	// TraceParams에 각각 추가
	TraceParams.AddIgnoredActors(PetActors);
	TraceParams.AddIgnoredActors(EnemyActors);


	// 라인 트레이스로 바닥 위치를 찾습니다.
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_WorldStatic, TraceParams))
	{
		// 충돌 지점을 스폰 위치로 설정합니다.
		SpawnLocation = HitResult.Location;
	}

	// 나이아가라 이펙트를 스폰합니다.
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), EffectToSpawn, SpawnLocation,
		FRotator::ZeroRotator, FVector(1.f));
}

void ABossSkeletonMage::PlayFireBallMontage()
{
	if ( BlackboardComp == nullptr ) return;
	
	StartSkillCooldown(TEXT("FireBall"));
	
	BossSkeletonMageLogData.FireBallCount++;

	// ─── [파이어볼 선딜레이 시작 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	CurrentFireBallActionID = UCombatTimelineLogger::LogSkillWindup(
		TEXT("FireBall"),
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP
	);
	
	if ( FireBallMontage )
	{
		PlayAnimMontage(FireBallMontage);
	}
}

// 애님 노티파이에서 호출할 함수
void ABossSkeletonMage::ShootFireBall()
{
	// 필요한 모든 컴포넌트와 변수가 유효한지 확인합니다.
	if (!IsValid(TargetCharacter) || !IsValid(FireBallSpawnPoint) || FireBallClass == nullptr)
	{
		return;
	}

	// ─── [파이어볼 발사/Release 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	UCombatTimelineLogger::LogSkillRelease(
		CurrentFireBallActionID,
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP,
		TEXT("FireBall Projectile Fired")
	);

	// FireBallSpawnPoint의 월드 위치와 회전값을 가져옵니다.
	const FVector SpawnLocation = FireBallSpawnPoint->GetComponentLocation();

	// 발사 위치에서 타겟을 향하는 방향을 계산합니다.
	const FVector TargetLocation = TargetCharacter->GetActorLocation();
	const FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, TargetLocation);

	// 스폰 파라미터를 설정합니다.
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 월드에 발사체를 스폰하고 ActionID를 주입합니다.
	if (ABaseEnemyProjectile* SpawnedProj = GetWorld()->SpawnActor<ABaseEnemyProjectile>(FireBallClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		SpawnedProj->ActionID = CurrentFireBallActionID;
		SpawnedProj->ActionName = TEXT("FireBall");
		SpawnedProj->SetDamage(AttackStruct.FireBallDamage > 0.0f ? AttackStruct.FireBallDamage : (BaseAttackDamage * 0.8f));
		SpawnedProj->HitIntensity = AttackStruct.FireBallHitIntensity;
		SpawnedProj->KnockdownType = AttackStruct.FireBallKnockdownType;
		SpawnedProj->KnockdownDuration = AttackStruct.FireBallKnockdownDuration;
	}
}

void ABossSkeletonMage::StartSummoning(const FVector& Location1, const FVector& Location2)
{
	if ( BlackboardComp == nullptr ) return;
	
	SpawnSummonEffectAtLocation(GetActorLocation()); // 마법사가 소환 시작 이펙트
	
	StartSkillCooldown(TEXT("Summon"));
	
	SummonLocations.Empty();
	SummonLocations.Add(Location1);
	SummonLocations.Add(Location2);
	
	BossSkeletonMageLogData.SummonCount++;

	// ─── [소환 패턴 선딜레이 시작 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	CurrentSummonActionID = UCombatTimelineLogger::LogSkillWindup(
		TEXT("Summon"),
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP
	);
	
	if ( SummonEnemyMontage )
	{
		PlayAnimMontage(SummonEnemyMontage);
	}
}

// 애님 노티파이에서 호출할 함수
void ABossSkeletonMage::SummonEnemy()
{
	//UE_LOG(LogTemp, Warning, TEXT("SummonEnemy called") );
	// 소환할 적 클래스가 없거나 소환 위치가 없으면 함수를 종료합니다.
	if (SummonableEnemyClasses.Num() == 0 || SummonLocations.Num() == 0) return;
	
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<FString> SpawnedMinionNames;
	
	// 저장된 각 소환 위치에 대해 반복합니다.
	for (const FVector& SpawnLocation : SummonLocations)
	{
		// 소환할 적 클래스 배열에서 무작위 인덱스를 선택합니다.
		const int32 RandomIndex = FMath::RandRange(0, SummonableEnemyClasses.Num() - 1);
		TSubclassOf<ABaseEnemy> EnemyClassToSummon = SummonableEnemyClasses[RandomIndex];

		if (EnemyClassToSummon)
		{
			// 스폰 위치에서 타겟을 바라보는 회전값을 계산합니다.
			const FRotator LookAtRotation =
				UKismetMathLibrary::FindLookAtRotation(SpawnLocation, TargetCharacter->GetActorLocation());
			// 수평으로만 바라보도록 Yaw 값만 사용합니다.
			const FRotator SpawnRotation = FRotator(0.f, LookAtRotation.Yaw, 0.f);

			// 적의 캡슐 컴포넌트 높이만큼 Z 오프셋을 적용하여 땅에 정확히 스폰되도록 합니다.
			float SafeZOffset = 0.f;
			ACharacter* EnemyCDO = Cast<ACharacter>(EnemyClassToSummon->GetDefaultObject());
			if (EnemyCDO && EnemyCDO->GetCapsuleComponent())
			{
				SafeZOffset = EnemyCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			}
			
			ABaseEnemy* SpawnedEnemy = 
				World->SpawnActor<ABaseEnemy>(
				EnemyClassToSummon, 
				SpawnLocation + FVector(0.f, 0.f, SafeZOffset), 
				SpawnRotation, 
				SpawnParams);

			if ( SummonEffectFromEnemy )
			{
				// 나이아가라 이펙트를 스폰합니다.
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SummonEffectFromEnemy,
					SpawnLocation + FVector(0.f, 0.f, 2.f),
					FRotator::ZeroRotator, FVector(20.f));
			}
			
			if ( SpawnedEnemy )
			{
				SpawnedEnemy->SummonerBoss = this;
				SpawnedEnemy->EnemyLogID = FString::Printf(TEXT("%s_SummonedEnemy"), *BossLogId);
				SpawnedEnemy->Tags.AddUnique(FName("Minion"));

				// 결계 추적 목록에 등록 및 사망 바인딩
				ActiveSummonedMinions.Add(SpawnedEnemy);
				SpawnedEnemy->OnDestroyed.AddDynamic(this, &ABossSkeletonMage::OnSummonedMinionDestroyed);

				// 소환수 클래스에 알맞은 전용 데이터테이블 행 결정
				FName UniversalMinionKey = TEXT("Minion_Melee");
				FName FallbackRowName = SummonMinionStatRowName;
				if (EnemyClassToSummon)
				{
					const FString ClassName = EnemyClassToSummon->GetName();
					if (ClassName.Contains(TEXT("Rogue")))
					{
						UniversalMinionKey = TEXT("Minion_Rogue");
						FallbackRowName = SummonRogueStatRowName;
					}
					else if (ClassName.Contains(TEXT("Warrior")))
					{
						UniversalMinionKey = TEXT("Minion_Warrior");
						FallbackRowName = SummonWarriorStatRowName;
					}
					else
					{
						UniversalMinionKey = TEXT("Minion_Melee");
						FallbackRowName = SummonMinionStatRowName;
					}
				}

				SpawnedMinionNames.Add(UniversalMinionKey.ToString());

				// 1. 범용 통합 전투 밸런스 테이블(DT_BossCombatBalance) 우선 적용
				bool bInitializedFromUniversal = false;
				if (const FBossCombatBalanceRow* MinionRow = FindSkillBalance(UniversalMinionKey))
				{
					if (MinionRow->MaxHealth > 0.0f)
					{
						SpawnedEnemy->MaxHealth = MinionRow->MaxHealth;
						SpawnedEnemy->Health = MinionRow->MaxHealth;
					}
					if (MinionRow->BaseAttackDamage > 0.0f)
					{
						SpawnedEnemy->AttackDamage = MinionRow->BaseAttackDamage;
						if (ABaseMeleeEnemy* MeleeEnemy = Cast<ABaseMeleeEnemy>(SpawnedEnemy))
						{
							MeleeEnemy->InitializeStatsFromDataTable();
						}
					}
					bInitializedFromUniversal = true;
					UE_LOG(LogTemp, Log, TEXT("[BossSkeletonMage] Summoned %s initialized from Universal Balance Table (HP: %.1f, DMG: %.1f)"),
						*UniversalMinionKey.ToString(), SpawnedEnemy->MaxHealth, SpawnedEnemy->AttackDamage);
				}

				// 2. 기존 CombatStatHandle (하위 호환성 유지)
				if (!bInitializedFromUniversal && !FallbackRowName.IsNone() && CombatStatHandle.DataTable)
				{
					SpawnedEnemy->CombatStatHandle.DataTable = CombatStatHandle.DataTable;
					SpawnedEnemy->CombatStatHandle.RowName = FallbackRowName;
					SpawnedEnemy->InitializeStatsFromDataTable();
				}
			}
		}
	}

	// ─── [소환 실행 (Release / Recovery) 로깅 (미니언 세부 목록 포함)] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	const FString SummonDetails = SpawnedMinionNames.Num() > 0 
		? FString::Printf(TEXT("Minions Spawned: %s"), *FString::Join(SpawnedMinionNames, TEXT(", ")))
		: TEXT("Minions Spawned");

	UCombatTimelineLogger::LogSkillRelease(
		CurrentSummonActionID,
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP,
		SummonDetails
	);
	UCombatTimelineLogger::LogSkillRecovery(
		CurrentSummonActionID,
		GetWorld()->GetTimeSeconds(),
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP
	);

	// 소환수가 존재하면 밀치기 원(매직 실드 결계) 즉시 가동!
	if (ActiveSummonedMinions.Num() > 0)
	{
		ActivateMagicShieldBarrier();
	}
}

void ABossSkeletonMage::SpawnSummonEffectAtLocation(const FVector& Location)
{
	if (SummonEffectFromMage == nullptr) return;

	FVector SpawnLocation = Location;

	// 바닥을 찾기 위해 라인 트레이스를 수행합니다.
	FHitResult HitResult;
	const FVector StartTrace = Location + FVector(0.f, 0.f, 100.f); // 약간 위에서 시작
	const FVector EndTrace = Location - FVector(0.f, 0.f, 1000.f); // 아래로 1000 유닛
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if ( TargetCharacter ) TraceParams.AddIgnoredActor(TargetCharacter);
	
	TArray<AActor*> PetActors;
	TArray<AActor*> EnemyActors;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Pet"), PetActors);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), EnemyActors);

	// TraceParams에 각각 추가
	TraceParams.AddIgnoredActors(PetActors);
	TraceParams.AddIgnoredActors(EnemyActors);

	// 라인 트레이스로 바닥 위치를 찾습니다.
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_WorldStatic, TraceParams))
	{
		// 충돌 지점보다 약간 위에 스폰 위치를 설정합니다.
		SpawnLocation = HitResult.Location + FVector(0.f, 0.f, 5.f);
	}

	// 나이아가라 이펙트를 스폰합니다.
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SummonEffectFromMage, SpawnLocation,
		FRotator::ZeroRotator, FVector(10.f));
}

void ABossSkeletonMage::PlayGroundAreaAttackMontage()
{
	if ( BlackboardComp == nullptr ) return;
	
	StartSkillCooldown(TEXT("GroundAttack"));

	BossSkeletonMageLogData.GroundAttackCount++;

	// ─── [장판 공격 선딜레이 시작 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	CurrentGroundAttackActionID = UCombatTimelineLogger::LogSkillWindup(
		TEXT("GroundAttack"),
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP
	);
	
	GroundTargetingComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		GroundTargetingEffect,
		TargetCharacter->GetActorLocation(), // 초기 위치
		FRotator::ZeroRotator,
		FVector(1.f),
		true
	);
	
	if ( GroundAreaAttackMontage )
	{
		PlayAnimMontage(GroundAreaAttackMontage);
	}
}

void ABossSkeletonMage::TraceTargetCharacterForGroundAttackEffect(float DeltaTime)
{
	// 그라운드 타겟팅 이펙트 위치 업데이트
	if (GroundTargetingComponent && GroundTargetingComponent->IsValidLowLevel() && IsValid(TargetCharacter))
	{
		FVector CharacterLocation = TargetCharacter->GetActorLocation();
		FVector TargetLocation = CharacterLocation;

		// 캐릭터 위치에서 아래로 라인 트레이스를 실행하여 바닥을 찾습니다.
		FHitResult HitResult;
		FVector StartTrace = CharacterLocation;
		FVector EndTrace = CharacterLocation - FVector(0.f, 0.f, 1000.f); // 아래로 1000 유닛
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);
		if ( TargetCharacter )TraceParams.AddIgnoredActor(TargetCharacter);
		

		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_WorldStatic, TraceParams))
		{
			// 충돌 지점을 타겟 위치로 설정합니다.
			TargetLocation = HitResult.Location;
		}

		// 이펙트의 월드 위치를 업데이트합니다. (Z축으로 2만큼 상승)
		GroundTargetingComponent->SetWorldLocation(TargetLocation + FVector(0.f, 0.f, 5.f));
	}
}

// 애님 노티파이에서 호출할 함수
void ABossSkeletonMage::GroundAreaAttack()
{
	if (!IsValid(TargetCharacter) || GroundAttackClass == nullptr) return;

	// 타겟팅 이펙트가 존재하면 제거합니다.
	if (GroundTargetingComponent && GroundTargetingComponent->IsValidLowLevel())
	{
		GroundTargetingComponent->DestroyComponent();
		GroundTargetingComponent = nullptr;
	}
	
	// ─── [장판 공격 발사/전개 Release 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	UCombatTimelineLogger::LogSkillRelease(
		CurrentGroundAttackActionID,
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP,
		TEXT("Ground Circle Deployed")
	);

	const FVector CharacterLocation = TargetCharacter->GetActorLocation();
	FVector SpawnLocation = CharacterLocation; // 기본 스폰 위치

	// 바닥을 찾기 위해 라인 트레이스를 수행합니다.
	FHitResult HitResult;
	const FVector StartTrace = CharacterLocation;
	const FVector EndTrace = CharacterLocation - FVector(0.f, 0.f, 1000.f); // 아래로 1000 유닛
	FCollisionQueryParams TraceParams;
	if ( TargetCharacter )TraceParams.AddIgnoredActor(TargetCharacter);
	TraceParams.AddIgnoredActor(this);
	
	TArray<AActor*> PetActors;
	TArray<AActor*> EnemyActors;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Pet"), PetActors);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), EnemyActors);

	// TraceParams에 각각 추가
	TraceParams.AddIgnoredActors(PetActors);
	TraceParams.AddIgnoredActors(EnemyActors);
	

	// 라인 트레이스로 바닥 위치를 찾습니다.
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_WorldStatic, TraceParams))
	{
		// 충돌 지점을 스폰 위치로 설정합니다.
		SpawnLocation = HitResult.Location;
	}
	// 장판은 보통 바닥에 평평하게 놓이므로 회전은 필요 없습니다.
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	// 항상 스폰되도록 설정합니다.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 월드에 장판 프로젝타일을 스폰합니다.
	AGroundAttackProjectile* SpawnedProjectile = GetWorld()->SpawnActor<AGroundAttackProjectile>(GroundAttackClass, SpawnLocation, SpawnRotation, SpawnParams);

	// 스폰된 액터가 유효하고, 유지 시간이 0보다 크면 LifeSpan 및 ActionID를 설정합니다.
	if (SpawnedProjectile)
	{
		SpawnedProjectile->ActionID = CurrentGroundAttackActionID;
		SpawnedProjectile->DurationTime = AttackStruct.GroundAttackDuration; // 장판 유지 시간 설정
		SpawnedProjectile->HitIntensity = AttackStruct.GroundAttackHitIntensity;
		SpawnedProjectile->KnockdownType = AttackStruct.GroundAttackKnockdownType;
		SpawnedProjectile->KnockdownDuration = AttackStruct.GroundAttackKnockdownDuration;
	}
	if ( GroundAttackEffect ) 
	{
		// 나이아가라 이펙트를 스폰합니다.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GroundAttackEffect,
			SpawnLocation + FVector(0.f, 0.f, 2.f),
			FRotator::ZeroRotator, FVector(5.f));
	}
}

bool ABossSkeletonMage::IsPlayerInPushArea() const
{
	if (!PushAreaSphere) return false;

	// 1. 오버랩된 액터들 중 "Player" 태그 확인
	TArray<AActor*> OverlappingActors;
	PushAreaSphere->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && Actor->ActorHasTag(FName("Player")))
		{
			return true;
		}
	}

	// 2. 거리 기반 안전 검사 (Sphere Radius 내에 타겟 플레이어가 있는지 확인)
	if (TargetCharacter && TargetCharacter->ActorHasTag(FName("Player")))
	{
		const float SphereRadius = PushAreaSphere->GetScaledSphereRadius();
		const float Dist = FVector::Dist(PushAreaSphere->GetComponentLocation(), TargetCharacter->GetActorLocation());
		if (Dist <= SphereRadius + 50.f)
		{
			return true;
		}
	}

	return false;
}

void ABossSkeletonMage::PlayPushTargetMontage()
{
	if ( BlackboardComp == nullptr ) return;
	if ( !IsPlayerInPushArea() ) return; // 플레이어가 밀치기 영역에 없으면 몽타주 미실행
	
	StartSkillCooldown(TEXT("PushTarget"));
	
	BossSkeletonMageLogData.PushTargetCount++;

	// ─── [PushTarget 선딜레이 시작 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	CurrentPushTargetActionID = UCombatTimelineLogger::LogSkillWindup(
		TEXT("PushTarget"),
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP
	);
	
	CurrentSkillHitIntensity = AttackStruct.PushTargetHitIntensity;
	CurrentSkillKnockdownType = AttackStruct.PushTargetKnockdownType;
	CurrentSkillKnockdownDuration = AttackStruct.PushTargetKnockdownDuration; bHasExplicitSkillReaction = true;

	if ( PushTargetMontage )
	{
		PlayAnimMontage(PushTargetMontage);
	}
}

void ABossSkeletonMage::CreateMagicShield()
{
	if ( MagicShieldEffect )
	{
		// 나이아가라 이펙트를 스폰합니다.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MagicShieldEffect,
			GetActorLocation() + FVector(0.f, 0.f, 10.f),
			FRotator::ZeroRotator, FVector(1.f));
	}
}

void ABossSkeletonMage::ActivateMagicShieldBarrier()
{
	bIsMagicShieldActive = true;
	LastShieldPushTime = -999.0f; // 즉시 첫 밀치기 가능하도록 초기화

	if (ActiveMagicShieldComponent)
	{
		ActiveMagicShieldComponent->DestroyComponent();
		ActiveMagicShieldComponent = nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BossSkeletonMage] Magic Shield Barrier Activated! Active Minions: %d"), ActiveSummonedMinions.Num());
}

void ABossSkeletonMage::DeactivateMagicShieldBarrier()
{
	bIsMagicShieldActive = false;

	if (ActiveMagicShieldComponent)
	{
		ActiveMagicShieldComponent->Deactivate();
		ActiveMagicShieldComponent->DestroyComponent();
		ActiveMagicShieldComponent = nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BossSkeletonMage] Magic Shield Barrier Shattered! Boss is now vulnerable."));
}

void ABossSkeletonMage::CheckMinionSurvivors()
{
	if (!bIsMagicShieldActive) return;

	ActiveSummonedMinions.RemoveAll([](const TWeakObjectPtr<ABaseEnemy>& MinionPtr)
	{
		return !MinionPtr.IsValid() || MinionPtr->Health <= 0.0f;
	});

	if (ActiveSummonedMinions.IsEmpty())
	{
		DeactivateMagicShieldBarrier();
	}
}

void ABossSkeletonMage::OnSummonedMinionDestroyed(AActor* DestroyedActor)
{
	CheckMinionSurvivors();
}

void ABossSkeletonMage::StartSkillCooldown(FName SkillName)
{
	if (FBossSkillCooldownInfo* Info = SkillCooldownMap.Find(SkillName))
	{
		Info->LastCastTime = GetWorld()->GetTimeSeconds();
		Info->bIsOnCooldown = true;

		if (BlackboardComp && !Info->BlackboardKey.IsNone())
		{
			// 쿨다운 시작: 가중치를 0으로 설정하여 AI 추첨에서 제외
			BlackboardComp->SetValueAsFloat(Info->BlackboardKey, 0.0f);
		}

		// 행동 딜레이는 스킬별 고유 ActionDelay 주입 (소환 6.0s 등 모션 보장)
		const float DelayToSet = Info->ActionDelay > 0.0f ? Info->ActionDelay : CommonAttackDelay;
		if (BlackboardComp)
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", DelayToSet);
		}

		UE_LOG(LogTemp, Log, TEXT("[BossSkeletonMage] Skill [%s] on Cooldown for %.1fs (ActionDelay: %.1fs)"),
			*SkillName.ToString(), Info->Cooldown, DelayToSet);
	}
	else
	{
		// 맵에 등록되지 않은 경우 폴백
		if (BlackboardComp)
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", CommonAttackDelay);
		}
	}
}

void ABossSkeletonMage::UpdateSkillCooldowns()
{
	if (!BlackboardComp || SkillCooldownMap.IsEmpty()) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (auto& Pair : SkillCooldownMap)
	{
		FBossSkillCooldownInfo& Info = Pair.Value;
		if (!Info.bIsOnCooldown || Info.BaseWeight <= 0.0f || Info.BlackboardKey.IsNone()) continue;

		// 쿨타임이 지났는지 검사
		if (CurrentTime - Info.LastCastTime >= Info.Cooldown)
		{
			Info.bIsOnCooldown = false;
			const float CurrentW = BlackboardComp->GetValueAsFloat(Info.BlackboardKey);
			if (CurrentW <= 0.0f)
			{
				// 쿨다운 만료: 원래 가중치로 복구
				BlackboardComp->SetValueAsFloat(Info.BlackboardKey, Info.BaseWeight);
				UE_LOG(LogTemp, Log, TEXT("[BossSkeletonMage] Skill [%s] Cooldown Finished -> Restored Weight to %.1f"),
					*Pair.Key.ToString(), Info.BaseWeight);
			}
		}
	}
}

bool ABossSkeletonMage::IsSkillOnCooldown(FName SkillOrBlackboardKey) const
{
	if (const FBossSkillCooldownInfo* Info = SkillCooldownMap.Find(SkillOrBlackboardKey))
	{
		return Info->bIsOnCooldown;
	}
	for (const auto& Pair : SkillCooldownMap)
	{
		if (Pair.Value.BlackboardKey == SkillOrBlackboardKey)
		{
			return Pair.Value.bIsOnCooldown;
		}
	}
	return false;
}

void ABossSkeletonMage::GravityAttack()
{
	if ( BlackboardComp == nullptr ) return;
	
	StartSkillCooldown(TEXT("GravityAttack"));
	
	BossSkeletonMageLogData.GravityAttackCount++;

	// ─── [GravityAttack 선딜레이 시작 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	CurrentGravityAttackActionID = UCombatTimelineLogger::LogSkillWindup(
		TEXT("GravityAttack"),
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP
	);
	
	if ( GravityAttackMontage )
	{
		PlayAnimMontage(GravityAttackMontage);
	}
}

void ABossSkeletonMage::StartGravityAttack()
{
	if (!TargetCharacter || !GravityGroundEffect) return;

	// ─── [GravityAttack 공중 부양 Release 로깅] ───
	float PlayerHP = 0.0f;
	float PlayerMaxHP = 0.0f;
	FString TargetCombatState = TEXT("Idle");
	float Dist = 0.0f;
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

	UCombatTimelineLogger::LogSkillRelease(
		CurrentGravityAttackActionID,
		GetWorld()->GetTimeSeconds(),
		Dist,
		TargetCombatState,
		Health,
		MaxHealth,
		PlayerHP,
		PlayerMaxHP,
		TEXT("Gravity Field Deployed")
	);

	UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement();
	if (MoveComp)
	{
		// 이미 중력이 낮아져 있다면 오염된 값을 저장하지 않고 표준 정상 수치(1.0f, 400.f)로 안전 백업
		DefaultGravityScale = (MoveComp->GravityScale >= 0.5f) ? MoveComp->GravityScale : 1.0f;
		DefaultAirControl = 0.05f;
		DefaultMaxWalkSpeed = (MoveComp->MaxWalkSpeed > 250.f) ? MoveComp->MaxWalkSpeed : 400.0f;
	}
	
	// 바닥 위치 계산 로직 (기존과 동일)
	const FVector CharacterLocation = TargetCharacter->GetActorLocation();
	FVector SpawnLocation = CharacterLocation;
	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	if ( TargetCharacter )TraceParams.AddIgnoredActor(TargetCharacter);
	TraceParams.AddIgnoredActor(this);

	TArray<AActor*> PetActors;
	TArray<AActor*> EnemyActors;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Pet"), PetActors);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), EnemyActors);

	// TraceParams에 각각 추가
	TraceParams.AddIgnoredActors(PetActors);
	TraceParams.AddIgnoredActors(EnemyActors);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, CharacterLocation, CharacterLocation - FVector(0.f, 0.f, 1000.f), ECC_WorldStatic, TraceParams))
	{
		SpawnLocation = HitResult.Location;
	}

	// 상태값 설정
	GravityAttackCenter = SpawnLocation + FVector(0.f, 0.f, GravityHalfHeight - 200.f);
	GravityTimer = 0.0f; // 타이머 초기화

	// 이펙트 스폰
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GravityGroundEffect, SpawnLocation + FVector(0.f, 0.f, 2.f), FRotator::ZeroRotator, FVector(1.f));

	// 시전 시간 뒤에 실제 캡슐 판정 활성화 (멤버 타이머 핸들로 관리하여 사망 시 취소 가능하도록 보장)
	GetWorldTimerManager().SetTimer(GravityActivationTimerHandle, [this]()
	{
		if (Health > 0.0f)
		{
			bIsGravityAttackActive = true; 
		}
	}, 1.0f, false);
	
	// 람다 캡처에 SpawnLocation을 추가하여 시작 시점의 바닥 위치를 기억하게 합니다.
	GetWorldTimerManager().SetTimer(GravityImpactEffectTimerHandle, [this, SpawnLocation]()
	{
		if (GravityImpactEffect && Health > 0.0f)
		{
			// 시작 시 장판(GravityGroundEffect) 위치와 동일한 위치에서 Z축으로 -100만큼 조정
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GravityImpactEffect,
				SpawnLocation + FVector(0.f, 0.f, -100.f), 
				FRotator::ZeroRotator, FVector(1.f));
		}
	}, 4.7f, false);
	
	// 일정 시간 뒤 종료 예약
	GetWorldTimerManager().SetTimer(GravityEndTimerHandle, this, &ABossSkeletonMage::EndGravityAttack, GravityDuration, false);
}

void ABossSkeletonMage::EndGravityAttack()
{
	bIsGravityAttackActive = false;
	
	// 1. 범위 내의 모든 캐릭터를 찾기 위한 설정
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape GravityCapsule = FCollisionShape::MakeCapsule(GravityRadius, GravityHalfHeight);
	FCollisionQueryParams OverlapParams;
	OverlapParams.AddIgnoredActor(this);

	TSet<ACharacter*> AffectedCharacters;

	// 2. 공격 종료 시 캡슐 범위 내에 있는 캐릭터들 체크
	if (GetWorld()->OverlapMultiByChannel(OverlapResults, GravityAttackCenter, FQuat::Identity, ECC_Pawn, GravityCapsule, OverlapParams))
	{
		for (auto& Result : OverlapResults)
		{
			ACharacter* OverlappedChar = Cast<ACharacter>(Result.GetActor());
			if (OverlappedChar && OverlappedChar->ActorHasTag(TEXT("Player")))
			{
				AffectedCharacters.Add(OverlappedChar);
			}
		}
	}

	// 3. 캡슐 오차 누락 방지: TargetCharacter가 무중력 상태(GravityScale < 1.0f)였거나 공중에 떠 있다면 반드시 포함!
	if (TargetCharacter)
	{
		UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement();
		if (MoveComp && (MoveComp->GravityScale < 1.0f || MoveComp->IsFalling()))
		{
			AffectedCharacters.Add(TargetCharacter);
		}
	}

	// 4. 대상 캐릭터들에게 슬램(하향 내리찍기) 및 넉다운 대미지 적용
	if (AffectedCharacters.IsEmpty())
	{
		// ─── [GravityAttack 회피 성공(Miss) 및 Recovery 로깅] ───
		float PlayerHP = 0.0f;
		float PlayerMaxHP = 0.0f;
		FString TargetCombatState = TEXT("Idle");
		float Dist = 0.0f;
		QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

		UCombatTimelineLogger::LogHitMissed(
			CurrentGravityAttackActionID,
			TEXT("GravityAttack"),
			GetWorld()->GetTimeSeconds(),
			Dist,
			TEXT("Player Escaped Gravity Field")
		);

		UCombatTimelineLogger::LogSkillRecovery(
			CurrentGravityAttackActionID,
			GetWorld()->GetTimeSeconds(),
			Health,
			MaxHealth,
			PlayerHP,
			PlayerMaxHP
		);
	}
	else
	{
		for (ACharacter* CharToSlam : AffectedCharacters)
		{
			if (!CharToSlam) continue;

			UCharacterMovementComponent* MoveComp = CharToSlam->GetCharacterMovement();
			if (MoveComp)
			{
				// 1. 대미지 주기 및 넉다운 피격 설정 (피격 처리를 먼저 완료)
				CurrentSkillHitIntensity = AttackStruct.GravityAttackHitIntensity;
				CurrentSkillKnockdownType = AttackStruct.GravityAttackKnockdownType;
				CurrentSkillKnockdownDuration = AttackStruct.GravityAttackKnockdownDuration; bHasExplicitSkillReaction = true;
				UGameplayStatics::ApplyDamage(CharToSlam, 
					AttackStruct.GravityAttackDamage, GetController(), this, UDamageType::StaticClass());

				// 2. 원래 중력 상태 복구 및 낙하(Falling) 모드 강제 보장
				MoveComp->GravityScale = DefaultGravityScale;
				MoveComp->AirControl = DefaultAirControl;
				MoveComp->MaxWalkSpeed = DefaultMaxWalkSpeed;
				MoveComp->SetMovementMode(MOVE_Falling);

				// 3. 바닥으로 강력하게 수직 발사 (물리 속도 강제 주입 + LaunchCharacter Z-Override)
				const FVector SlamVelocity = FVector(0.f, 0.f, -4000.f);
				MoveComp->Velocity.Z = -4000.f;
				CharToSlam->LaunchCharacter(SlamVelocity, false, true);
				
				CommonBossLogData.TotalDamageDealt += AttackStruct.GravityAttackDamage;
				BossSkeletonMageLogData.GravityAttackDamage += AttackStruct.GravityAttackDamage;

				// ─── [GravityAttack 바닥 충돌 Hit 및 Recovery 로깅] ───
				float PlayerHP = 0.0f;
				float PlayerMaxHP = 0.0f;
				FString TargetCombatState = TEXT("InAir");
				float Dist = 0.0f;
				QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

				UCombatTimelineLogger::LogHitSuccess(
					CurrentGravityAttackActionID,
					TEXT("GravityAttack"),
					AttackStruct.GravityAttackDamage,
					GetWorld()->GetTimeSeconds(),
					Dist,
					TargetCombatState,
					Health,
					MaxHealth,
					PlayerHP,
					PlayerMaxHP
				);
				UCombatTimelineLogger::LogSkillRecovery(
					CurrentGravityAttackActionID,
					GetWorld()->GetTimeSeconds(),
					Health,
					MaxHealth,
					PlayerHP,
					PlayerMaxHP
				);
			}
		}
	}
	
	// 5. (혹시 그 외의 경우라도) 타겟팅된 메인 플레이어 중력은 확실하게 복구
	if (TargetCharacter)
	{
		UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement();
		if (MoveComp && MoveComp->GravityScale < 1.0f)
		{
			MoveComp->GravityScale = DefaultGravityScale;
			MoveComp->AirControl = DefaultAirControl;
			MoveComp->MaxWalkSpeed = DefaultMaxWalkSpeed;
		}
	}
}

void ABossSkeletonMage::HandleGravityAttack(float DeltaTime)
{
	// 보스가 이미 사망했거나 체력이 0 이하라면 중력 틱 즉시 차단
	if (!bIsGravityAttackActive || Health <= 0.0f)
	{
		return;
	}
		// 디버그 캡슐 그리기는 매 프레임 실행 (시각화 유지용)
		//DrawDebugCapsule(GetWorld(), GravityAttackCenter, GravityHalfHeight, GravityRadius, 
		//FQuat::Identity, FColor::Purple, false, DeltaTime * 2.f, 0, 1.0f);

		// 범위 체크
		TArray<FOverlapResult> OverlapResults;
		FCollisionShape GravityCapsule = FCollisionShape::MakeCapsule(GravityRadius, GravityHalfHeight);
		FCollisionQueryParams OverlapParams;
		OverlapParams.AddIgnoredActor(this);

		bool bIsPlayerInArea = false;
		if (GetWorld()->OverlapMultiByChannel(OverlapResults, GravityAttackCenter, FQuat::Identity, ECC_Pawn, GravityCapsule, OverlapParams))
		{
			for (auto& Result : OverlapResults)
			{
				ACharacter* OverlappedChar = Cast<ACharacter>(Result.GetActor());
				if (OverlappedChar && OverlappedChar->ActorHasTag(TEXT("Player")))
				{
					bIsPlayerInArea = true;

					// 구르기 중(bIsRolling)이 아닐 때만 공격 몽타주 종료 및 막기(가드) 상태 즉시 해제
					if (ICombatLogParticipantInterface* Participant = Cast<ICombatLogParticipantInterface>(OverlappedChar))
					{
						Participant->InterruptCombatAction(true, true);
					}

					UCharacterMovementComponent* MoveComp = OverlappedChar->GetCharacterMovement();
					if (MoveComp)
					{
						// 영역 안에 있는 동안 지속적으로 무중력 적용
						MoveComp->GravityScale = 0.05f; 
						MoveComp->AirControl = 0.7f;  // 공중 제어력 증가
						MoveComp->MaxWalkSpeed = 200.f; // 좀 느리게
						
						// Z축으로 살짝 뜨게 하는 힘 (둥둥 뜨는 느낌)
						if (OverlappedChar->GetVelocity().Z < 100.f)
						{
							MoveComp->AddImpulse(FVector(0.f, 0.f, 20.f), true);
						}
					}
				}
			}
		}

		// 만약 플레이어가 영역 밖으로 나갔다면 중력 원복
		if (!bIsPlayerInArea && TargetCharacter)
		{
			UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement();
			if (MoveComp && MoveComp->GravityScale < 1.0f)
			{
				MoveComp->GravityScale = DefaultGravityScale;
				MoveComp->AirControl = DefaultAirControl;
				MoveComp->MaxWalkSpeed = DefaultMaxWalkSpeed;
			}
		}
}

void ABossSkeletonMage::StartSecondPhase()
{
	if ( BlackboardComp == nullptr ) return;

	BlackboardComp->SetValueAsBool("SecondPhase", true);
}

void ABossSkeletonMage::PlaySecondPhaseMontage()
{
	if ( SecondPhaseMontage )
	{
		PlayAnimMontage(SecondPhaseMontage);
	}
	
	// 현재 위치에 텔레포트 입장 이펙트 생성
	if ( TeleportInEffect ) SpawnTeleportEffectAtLocation(GetActorLocation(), TeleportInEffect); 
	// 타깃 캐릭터 위치에도 텔레포트 입장 이펙트 생성
	if ( TargetCharacter && TeleportInEffect )
	{
		SpawnTeleportEffectAtLocation(TargetCharacter->GetActorLocation(), TeleportInEffect);
	}
}

void ABossSkeletonMage::SecondPhaseTeleportBossAndPlayer()
{
	// 1. 보스 순간이동 처리
	if (BossTeleportPoint)
	{
		FVector BossDest = BossTeleportPoint->GetActorLocation();
		
		// 보스 본체는 캡슐 높이만큼 보정해서 이동
		float SafeZOffset = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		FVector AdjustedBossDest = BossDest + FVector(0.f, 0.f, SafeZOffset);
		SetActorLocation(AdjustedBossDest);

		// 도착 지점(타깃 포인트 위치)에 텔레포트 아웃 이펙트 생성
		if (TeleportOutEffect)
		{
			// SpawnTeleportEffectAtLocation 내부의 LineTrace가 바닥에 예쁘게 깔아줍니다.
			SpawnTeleportEffectAtLocation(BossDest, TeleportOutEffect);
		}
	}

	// 2. 플레이어 순간이동 처리
	if (PlayerTeleportPoint && TargetCharacter)
	{
		FVector PlayerDest = PlayerTeleportPoint->GetActorLocation();
		
		// 플레이어 본체는 캡슐 높이만큼 보정해서 이동
		float PlayerSafeZOffset = TargetCharacter->GetCapsuleComponent() ? TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		FVector AdjustedPlayerDest = PlayerDest + FVector(0.f, 0.f, PlayerSafeZOffset);
		TargetCharacter->SetActorLocation(AdjustedPlayerDest);

		// 플레이어 도착 지점(타깃 포인트 위치)에 텔레포트 아웃 이펙트 생성
		if (TeleportOutEffect)
		{
			SpawnTeleportEffectAtLocation(PlayerDest, TeleportOutEffect);
		}
	}
	
	// 3. 순간이동 후 보스가 플레이어를 즉시 바라보게 회전 업데이트
	if (TargetCharacter)
	{
		const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetCharacter->GetActorLocation());
		SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));
	}
	
	if (TargetCharacter)
	{
		if (APlayerController* PC = Cast<APlayerController>(TargetCharacter->GetController()))
		{
			// 1. 플레이어 위치에서 보스 위치를 바라보는 기본 회전값 계산
			FVector StartLook = TargetCharacter->GetActorLocation();
			FVector EndLook = GetActorLocation();
    
			FRotator NewControlRotation = UKismetMathLibrary::FindLookAtRotation(StartLook, EndLook);
        
			// 2. 각도 보정: 카메라를 약간 위로 들게 함 (Pitch 조정)
			// 음수(-) 방향으로 갈수록 하늘을 바라보고, 양수(+) 방향으로 갈수록 바닥을 바라봅니다.
			// -10.0f ~ -20.0f 사이의 값을 주어 보스와 캐릭터가 같이 보이게 조절하세요.
			NewControlRotation.Pitch -= 15.0f; 

			// 3. 카메라 제어 회전(Control Rotation) 설정
			PC->SetControlRotation(NewControlRotation);
		}
	}
}

void ABossSkeletonMage::SummonThunderToPlayer()
{
	if (!TargetCharacter || !ThunderTargetingEffect || !ThunderImpactEffect) return;

    // 1. 플레이어의 현재 바닥 위치 계산
    const FVector PlayerLocation = TargetCharacter->GetActorLocation();
    FVector SpawnLocation = PlayerLocation;
    
    FHitResult HitResult;
    FCollisionQueryParams TraceParams;
    TraceParams.AddIgnoredActor(this);
    if ( TargetCharacter ) TraceParams.AddIgnoredActor(TargetCharacter);

	TArray<AActor*> PetActors;
	TArray<AActor*> EnemyActors;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Pet"), PetActors);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), EnemyActors);

	// TraceParams에 각각 추가
	TraceParams.AddIgnoredActors(PetActors);
	TraceParams.AddIgnoredActors(EnemyActors);
	
    // 바닥 감지 (LineTrace)
    if (GetWorld()->LineTraceSingleByChannel(HitResult, PlayerLocation, PlayerLocation - FVector(0.f, 0.f, 1000.f), ECC_WorldStatic, TraceParams))
    {
        SpawnLocation = HitResult.Location;
    }

    // 2. 타겟팅 이펙트 생성 (번개가 떨어질 곳을 미리 알림)
    // 위치를 약간 띄워(Z+5.0) 바닥에 묻히지 않게 합니다.
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ThunderTargetingEffect, SpawnLocation + FVector(0.f, 0.f, 5.f));

    // [Combat Timeline Logger] 번개 윈드업 기록
    float PlayerHP = 0.0f;
    float PlayerMaxHP = 0.0f;
    FString TargetCombatState = TEXT("Idle");
    float Dist = 0.0f;
    QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

    const FString ThunderActionID = UCombatTimelineLogger::LogSkillWindup(
        TEXT("Phase2_Thunder"),
        GetWorld()->GetTimeSeconds(),
        Dist,
        TargetCombatState,
        Health,
        MaxHealth,
        PlayerHP,
        PlayerMaxHP
    );
    CurrentPhase2ThunderActionID = ThunderActionID;

    // 3. 딜레이 이후 실제 번개 이펙트 및 대미지 로직 실행 (상황에 맞게 시간 조절)
    float LightningDelay = 1.0f; 
    
    FTimerHandle ThunderTimerHandle;
    GetWorldTimerManager().SetTimer(ThunderTimerHandle, [this, SpawnLocation, ThunderActionID]()
    {
        float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        float CurPlayerHP = 0.0f;
        float CurPlayerMaxHP = 0.0f;
        FString CurPlayerState = TEXT("Idle");
        float CurDist = 0.0f;
        QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

        // [Combat Timeline Logger] 번개 방출 기록
        UCombatTimelineLogger::LogSkillRelease(
            ThunderActionID,
            CurrentTime,
            CurDist,
            CurPlayerState,
            Health,
            MaxHealth,
            CurPlayerHP,
            CurPlayerMaxHP,
            TEXT("Lightning Strike")
        );

        // A. 실제 번개 이펙트 소환
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ThunderImpactEffect, SpawnLocation);

        // B. 대미지 판정용 디버그 스피어 (2초 유지)
        float DamageRadius = 100.f;
        //DrawDebugSphere(GetWorld(), SpawnLocation, DamageRadius, 12, FColor::Yellow, false, 2.0f);

        // C. 실제 대미지 적용 (범위 내 플레이어 체크)
        TArray<FOverlapResult> OverlapResults;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageRadius);
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

    	if ( ThunderSound ) UGameplayStatics::PlaySound2D(GetWorld(), ThunderSound);
    	
        bool bHitPlayer = false;
        if (GetWorld()->OverlapMultiByChannel(OverlapResults, SpawnLocation, FQuat::Identity, ECC_Pawn, Sphere, QueryParams))
        {
	        for (auto& Result : OverlapResults)
	        {
				AActor* HitActor = Result.GetActor();
				if (HitActor && HitActor->ActorHasTag(TEXT("Player")))
				{
					CurrentSkillHitIntensity = AttackStruct.SecondPhaseThunderHitIntensity;
					CurrentSkillKnockdownType = AttackStruct.SecondPhaseThunderKnockdownType;
					CurrentSkillKnockdownDuration = AttackStruct.SecondPhaseThunderKnockdownDuration; bHasExplicitSkillReaction = true;
					UGameplayStatics::ApplyDamage(HitActor, AttackStruct.SecondPhaseThunderAttackDamage, GetController(), this, UDamageType::StaticClass());
					
					BossSkeletonMageLogData.SecondPhaseThunderAttackDamage += AttackStruct.SecondPhaseThunderAttackDamage;
					CommonBossLogData.TotalDamageDealt += AttackStruct.SecondPhaseThunderAttackDamage;

					// [Combat Timeline Logger] 적중 기록
					float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackStruct.SecondPhaseThunderAttackDamage);
					UCombatTimelineLogger::LogHitSuccess(
						ThunderActionID,
						TEXT("Phase2_Thunder"),
						AttackStruct.SecondPhaseThunderAttackDamage,
						CurrentTime,
						CurDist,
						CurPlayerState,
						Health,
						MaxHealth,
						EstimatedAfterHP,
						CurPlayerMaxHP
					);
					bHitPlayer = true;
				}
			}
        }

        if (!bHitPlayer)
        {
            // [Combat Timeline Logger] 빗나감 기록
            UCombatTimelineLogger::LogHitMissed(
                ThunderActionID,
                TEXT("Phase2_Thunder"),
                CurrentTime,
                CurDist,
                TEXT("Player Escaped Strike Area")
            );
        }
    }, LightningDelay, false);
    
}

void ABossSkeletonMage::StartSummonRandomMeteor()
{
	float MeteorRadius = 350.f; 
	float ForwardOffset = 450.f; 

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys || !TargetCharacter) return;

	// 1. 기준 보정 위치 계산
	const FVector TargetLoc = TargetCharacter->GetActorLocation();
	const FVector TargetForward = TargetCharacter->GetActorForwardVector();
    
	// 캐릭터의 캡슐 절반 높이를 가져옵니다.
	float HalfHeight = TargetCharacter->GetCapsuleComponent() ? TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;

	// 2. 기준점 계산 (캐릭터 앞쪽 ~ 유닛 + 캡슐 높이 절반만큼 아래로)
	const FVector OriginPoint = TargetLoc + (TargetForward * ForwardOffset) - FVector(0.f, 0.f, HalfHeight);

	// 디버그 드로우 (위치가 내려갔는지 확인용)
	//DrawDebugSphere(GetWorld(), OriginPoint, MeteorRadius, 16, FColor::Purple.WithAlpha(50), false, 1.0f);

	// 3. 단일 위치 찾기 및 생성
	FNavLocation RandomNavLocation;
	if (NavSys->GetRandomPointInNavigableRadius(OriginPoint, MeteorRadius, RandomNavLocation))
	{
        FVector SpawnLocation = RandomNavLocation.Location;
		
        // [Combat Timeline Logger] 메테오 윈드업 기록
        float PlayerHP = 0.0f;
        float PlayerMaxHP = 0.0f;
        FString TargetCombatState = TEXT("Idle");
        float Dist = 0.0f;
        QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetCombatState, Dist);

        const FString MeteorActionID = UCombatTimelineLogger::LogSkillWindup(
            TEXT("Phase2_Meteor"),
            GetWorld()->GetTimeSeconds(),
            Dist,
            TargetCombatState,
            Health,
            MaxHealth,
            PlayerHP,
            PlayerMaxHP
        );
        CurrentPhase2MeteorActionID = MeteorActionID;

        // 이펙트 알림
        if (MeteorEffect)
        {
        	// Z값을 20 정도로 높여서 바닥에 묻히는걸 방지
        	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), 
				MeteorEffect, 
				SpawnLocation + FVector(0, 0, 5.f), 
				FRotator::ZeroRotator,
				FVector(1.f),
				true // Auto Destroy
			);
        }

        // 실제 대미지 판정 (0.9초 뒤 낙하)
        float FallDelay = 0.9f;
        FTimerHandle IndividualMeteorTimer;
        GetWorldTimerManager().SetTimer(IndividualMeteorTimer, [this, SpawnLocation, MeteorActionID]()
        {
            float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
            float CurPlayerHP = 0.0f;
            float CurPlayerMaxHP = 0.0f;
            FString CurPlayerState = TEXT("Idle");
            float CurDist = 0.0f;
            QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

            // [Combat Timeline Logger] 메테오 방출 기록
            UCombatTimelineLogger::LogSkillRelease(
                MeteorActionID,
                CurrentTime,
                CurDist,
                CurPlayerState,
                Health,
                MaxHealth,
                CurPlayerHP,
                CurPlayerMaxHP,
                TEXT("Meteor Impact")
            );

            float DamageRadius = 200.f;
            if (MeteorSound) UGameplayStatics::PlaySound2D(GetWorld(), MeteorSound);
            
            TArray<FOverlapResult> OverlapResults;
            FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageRadius);
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(this);

            bool bHitPlayer = false;
            if (GetWorld()->OverlapMultiByChannel(OverlapResults, SpawnLocation, FQuat::Identity, ECC_Pawn, Sphere, QueryParams))
            {
                for (auto& Result : OverlapResults)
                {
                    AActor* HitActor = Result.GetActor();
                    if (HitActor && HitActor->ActorHasTag(TEXT("Player")))
                    {
                        CurrentSkillHitIntensity = AttackStruct.SecondPhaseMeteorHitIntensity;
                        CurrentSkillKnockdownType = AttackStruct.SecondPhaseMeteorKnockdownType;
                        CurrentSkillKnockdownDuration = AttackStruct.SecondPhaseMeteorKnockdownDuration; bHasExplicitSkillReaction = true;
                        UGameplayStatics::ApplyDamage(HitActor, AttackStruct.SecondPhaseMeteorAttackDamage, GetController(), this, UDamageType::StaticClass());

                        BossSkeletonMageLogData.SecondPhaseMeteorAttackDamage += AttackStruct.SecondPhaseMeteorAttackDamage;
						CommonBossLogData.TotalDamageDealt += AttackStruct.SecondPhaseMeteorAttackDamage;

						// [Combat Timeline Logger] 적중 기록
						float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackStruct.SecondPhaseMeteorAttackDamage);
						UCombatTimelineLogger::LogHitSuccess(
							MeteorActionID,
							TEXT("Phase2_Meteor"),
							AttackStruct.SecondPhaseMeteorAttackDamage,
							CurrentTime,
							CurDist,
							CurPlayerState,
							Health,
							MaxHealth,
							EstimatedAfterHP,
							CurPlayerMaxHP
						);
						bHitPlayer = true;
                    }
                }
            }

            if (!bHitPlayer)
            {
                // [Combat Timeline Logger] 빗나감 기록
                UCombatTimelineLogger::LogHitMissed(
                    MeteorActionID,
                    TEXT("Phase2_Meteor"),
                    CurrentTime,
                    CurDist,
                    TEXT("Player Escaped Meteor Area")
                );
            }
        }, FallDelay, false);
    }
}

void ABossSkeletonMage::EndBattleLog()
{
	if (ICombatLogParticipantInterface* Participant = Cast<ICombatLogParticipantInterface>(TargetCharacter))
	{
		BossSkeletonMageLogData.PlayerHitStunDuration = Participant->GetParticipantHitStunDuration();
		BossSkeletonMageLogData.PlayerHitStunCount = Participant->GetParticipantHitStunCount();
		BossSkeletonMageLogData.PlayerKnockdownDuration = Participant->GetParticipantKnockdownDuration();
		BossSkeletonMageLogData.PlayerKnockdownCount = Participant->GetParticipantKnockdownCount();
		Participant->ResetParticipantStunMetrics();
	}

	Super::EndBattleLog();
	
	BossSkeletonMageLogData.Base = CommonBossLogData;
	
	UCSVLog::AddSkeletonMageLog( BossSkeletonMageLogData );
	
	CommonBossLogData = FCommonBossLogData();
	BossSkeletonMageLogData = FBossSkeletonMageLogData();
}

void ABossSkeletonMage::AddSummonDamage(float DamageAmount)
{
	Super::AddSummonDamage(DamageAmount);
	BossSkeletonMageLogData.SummonDamage += DamageAmount;
}

void ABossSkeletonMage::InitializeStatsFromDataTable()
{
	Super::InitializeStatsFromDataTable();

	// ─── 1. 범용 통합 전투 밸런스 테이블(DT_BossCombatBalance)에서 스킬 파라미터 주입 ───
	if (CachedSkillBalanceMap.Num() > 0)
	{
		// FireBall
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("FireBall")))
		{
			AttackStruct.FireBallWeight = Row->Weight;
			AttackStruct.FireBallDelay = Row->Cooldown;
			AttackStruct.FireBallDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 0.8f);
			AttackStruct.FireBallHitIntensity = Row->HitIntensity;
			AttackStruct.FireBallKnockdownDuration = Row->KnockdownDuration;
			AttackStruct.FireBallKnockdownType = Row->KnockdownType;
		}

		// GroundAttack
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("GroundAttack")))
		{
			AttackStruct.GroundAttackWeight = Row->Weight;
			AttackStruct.GroundAttackDelay = Row->Cooldown;
			AttackStruct.GroundAttackHitIntensity = Row->HitIntensity;
			AttackStruct.GroundAttackKnockdownDuration = Row->KnockdownDuration;
			AttackStruct.GroundAttackKnockdownType = Row->KnockdownType;
		}

		// GravityAttack
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("GravityAttack")))
		{
			AttackStruct.GravityAttackWeight = Row->Weight;
			AttackStruct.GravityAttackDelay = Row->Cooldown;
			AttackStruct.GravityAttackDamage = BaseAttackDamage * Row->DamageMultiplier;
			AttackStruct.GravityAttackHitIntensity = Row->HitIntensity;
			AttackStruct.GravityAttackKnockdownDuration = Row->KnockdownDuration;
			AttackStruct.GravityAttackKnockdownType = Row->KnockdownType;
		}

		// PushTarget
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("PushTarget")))
		{
			AttackStruct.PushTargetWeight = Row->Weight;
			AttackStruct.PushTargetDelay = Row->Cooldown;
			AttackStruct.PushTargetHitIntensity = Row->HitIntensity;
			AttackStruct.PushTargetKnockdownDuration = Row->KnockdownDuration;
			AttackStruct.PushTargetKnockdownType = Row->KnockdownType;
		}

		// Teleport
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("Teleport")))
		{
			AttackStruct.TeleportWeight = Row->Weight;
			AttackStruct.TeleportDelay = Row->Cooldown;
		}

		// Summon
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("Summon")))
		{
			AttackStruct.SummonWeight = Row->Weight;
			AttackStruct.SummonEnemyDelay = Row->Cooldown;
		}

		// Thunder
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("Thunder")))
		{
			AttackStruct.SecondPhaseThunderAttackDamage = BaseAttackDamage * Row->DamageMultiplier;
			AttackStruct.SecondPhaseThunderHitIntensity = Row->HitIntensity;
			AttackStruct.SecondPhaseThunderKnockdownDuration = Row->KnockdownDuration;
			AttackStruct.SecondPhaseThunderKnockdownType = Row->KnockdownType;
		}

		// Meteor
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("Meteor")))
		{
			AttackStruct.SecondPhaseMeteorAttackDamage = BaseAttackDamage * Row->DamageMultiplier;
			AttackStruct.SecondPhaseMeteorHitIntensity = Row->HitIntensity;
			AttackStruct.SecondPhaseMeteorKnockdownDuration = Row->KnockdownDuration;
			AttackStruct.SecondPhaseMeteorKnockdownType = Row->KnockdownType;
		}

		// BaseStat에서 공통 행동 딜레이(CommonAttackDelay) 읽기 (기본값: 1.0초)
		if (const FBossCombatBalanceRow* BaseStatRow = FindSkillBalance(TEXT("BaseStat")))
		{
			if (BaseStatRow->ActionDelay > 0.0f)
			{
				CommonAttackDelay = BaseStatRow->ActionDelay;
			}
			else if (BaseStatRow->Cooldown > 0.0f)
			{
				CommonAttackDelay = BaseStatRow->Cooldown;
			}
		}

		// 스킬별 독립 쿨타임 및 고유 행동 딜레이 관리 맵 구성 (스킬 시전 시 AI 가중치 0 처리 -> 쿨타임 종료 시 원복)
		SkillCooldownMap.Empty();

		auto RegisterSkillCooldown = [this](FName SkillName, FName BBKey, float Cooldown, float ActionDelay, float Weight)
		{
			FBossSkillCooldownInfo Info;
			Info.Cooldown = Cooldown;
			Info.ActionDelay = ActionDelay;
			Info.BaseWeight = Weight;
			Info.BlackboardKey = BBKey;
			Info.LastCastTime = -999.0f;
			SkillCooldownMap.Add(SkillName, Info);
		};

		auto GetSkillActionDelay = [this](FName SkillName, float FallbackDelay) -> float
		{
			if (const FBossCombatBalanceRow* Row = FindSkillBalance(SkillName))
			{
				if (Row->ActionDelay > 0.0f)
				{
					return Row->ActionDelay;
				}
			}
			return FallbackDelay;
		};

		RegisterSkillCooldown(TEXT("FireBall"), TEXT("W_FireBall"), AttackStruct.FireBallDelay, GetSkillActionDelay(TEXT("FireBall"), 1.0f), AttackStruct.FireBallWeight);
		RegisterSkillCooldown(TEXT("GroundAttack"), TEXT("W_GroundAttack"), AttackStruct.GroundAttackDelay, GetSkillActionDelay(TEXT("GroundAttack"), 1.5f), AttackStruct.GroundAttackWeight);
		RegisterSkillCooldown(TEXT("GravityAttack"), TEXT("W_GravityAttack"), AttackStruct.GravityAttackDelay, GetSkillActionDelay(TEXT("GravityAttack"), 1.5f), AttackStruct.GravityAttackWeight);
		RegisterSkillCooldown(TEXT("PushTarget"), TEXT("W_PushTarget"), AttackStruct.PushTargetDelay, GetSkillActionDelay(TEXT("PushTarget"), 0.5f), AttackStruct.PushTargetWeight);
		RegisterSkillCooldown(TEXT("Teleport"), TEXT("W_Teleport"), AttackStruct.TeleportDelay, GetSkillActionDelay(TEXT("Teleport"), 4.5f), AttackStruct.TeleportWeight);
		RegisterSkillCooldown(TEXT("Summon"), TEXT("W_Summon"), AttackStruct.SummonEnemyDelay, GetSkillActionDelay(TEXT("Summon"), 6.0f), AttackStruct.SummonWeight);

		UE_LOG(LogTemp, Log, TEXT("[BossSkeletonMage] Successfully injected all skill balance parameters and cooldowns! (Common Delay: %.1fs)"), CommonAttackDelay);
		return;
	}

	// ─── 2. 기존 CombatStatHandle (하위 호환성 유지) ───
	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())
	{
		AttackStruct.GravityAttackDamage = BaseAttackDamage * 0.15f;
		AttackStruct.SecondPhaseThunderAttackDamage = BaseAttackDamage * 0.05f;
		AttackStruct.SecondPhaseMeteorAttackDamage = BaseAttackDamage * 0.1f;
	}
}