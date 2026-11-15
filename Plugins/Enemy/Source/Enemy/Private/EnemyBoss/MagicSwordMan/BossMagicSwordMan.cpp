#include "EnemyBoss/MagicSwordMan/BossMagicSwordMan.h"

#include "CSVLog.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "EnemyProjectile/BaseEnemyProjectile.h"
#include "Engine/OverlapResult.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "CombatTimelineLogger.h"

ABossMagicSwordMan::ABossMagicSwordMan()
{
	PowerAttackCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PowerAttackCollisionSphere"));
	PowerAttackCollisionSphere->SetupAttachment(GetRootComponent());
	PowerAttackCollisionSphere->SetSphereRadius( 540.f ); // 궁극기 공격 범위
	PowerAttackCollisionSphere->ShapeColor = FColor::Red;
	PowerAttackCollisionSphere->SetVisibility(false);
	PowerAttackCollisionSphere->SetHiddenInGame(false);

	// 전투 로그/타임라인 파일 이름 및 DT_BossCombatBalance 조회에 쓰이는 보스 ID
	BossLogId = TEXT("Boss_MagicSwordMan");

	// 범용 통합 전투 밸런스 데이터테이블 기본 바인딩 (DT_BossCombatBalance)
	static ConstructorHelpers::FObjectFinder<UDataTable> UniversalBalanceTableObj(TEXT("/Game/HYH/DataAssets/DT_BossCombatBalance.DT_BossCombatBalance"));
	if (UniversalBalanceTableObj.Succeeded())
	{
		UniversalBalanceTableHandle.DataTable = UniversalBalanceTableObj.Object;
	}
}

void ABossMagicSwordMan::BeginPlay()
{
	Super::BeginPlay();
	
	CommonBossLogData.BossID = BossLogId; // 로그 데이터에 보스 ID 기록
	
	TArray<UCapsuleComponent*> CapsuleCollisionComps;
	GetComponents<UCapsuleComponent>(CapsuleCollisionComps);
	
	// 반복문 돌면서 태그 확인
	for (UCapsuleComponent* Capsule : CapsuleCollisionComps)
	{
		if (Capsule && Capsule->ComponentHasTag(TEXT("Weapon"))) // "Weapon" 태그를 가진 콜리전 스피어를 찾습니다.)))
		{
			WeaponCollision = Capsule;
			WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ABossMagicSwordMan::OnBeginOverlapWeaponCollisionSphere);
			// 콜리전 끄기
			WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break; // 찾았으니 루프 종료
		}
	}
	
	if ( PowerAttackCollisionSphere )
	{
		PowerAttackCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, 
			&ABossMagicSwordMan::OnBeginOverlapPowerAttackCollisionSphere);
		PowerAttackCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABossMagicSwordMan::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

float ABossMagicSwordMan::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	if ( bIsGuarding )
	{
		DamageWhileGuarding += DamageAmount;

		// 아직 리액션 대미지에 도달하지 않았으면 가드 몽타주 재생
		if ( DamageWhileGuarding < AttackStruct.MaxDamageToReaction && GuardHitMontage )
		{
			PlayAnimMontage(GuardHitMontage);
		}

		BossMagicSwordManLogData.TotalDamageGuarded += DamageAmount; // 로그 데이터에 가드로 막은 대미지 누적
		
		return 0.f; // 대미지 무효화
	}
	
	// 2. 가드 중이 아닐 때 실제 대미지 적용
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 3. 체력 비율 체크를 통한 2페이즈(특수 패턴) 진입 로직
	if (Health > 0.f && bIsSecondPhaseStarted == false)
	{
		// 현재 체력 비율 계산 (MaxHealth가 0이 아님을 전제)
		float CurrentHealthRatio = Health / MaxHealth;

		if (CurrentHealthRatio <= SecondPhaseHealthThreshold)
		{
			bIsSecondPhaseStarted = true; // 중복 실행 방지
			
			// 블랙보드 값 업데이트 (StartSecondPhase 함수 호출)
			StartSecondPhase(); 
		}
	}
	
	return ActualDamage;
}

void ABossMagicSwordMan::OnBeginOverlapWeaponCollisionSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ( AttackDamage == 0.f ) return; // 대미지가 0이면 피격 처리 안 함
	
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		// 어택 대미지 로그 
		UE_LOG(LogTemp, Warning, TEXT("Boss Attack Damage : %f"), AttackDamage);

		// 띄우기 첫 타: 플레이어가 지상에서 가드/패링 중이면 일반 가드로 처리 (에어본 넉다운이 가드를 깨지 않도록 리액션 해제, 띄우지 않음)
		const bool bIsJumpUpAttack = (AttackType == EMagicSwordManAttackType::CloseJumpUpAttack || AttackType == EMagicSwordManAttackType::DashJumpUpAttack);
		bool bJumpUpBlocked = false;
		if (bIsJumpUpAttack)
		{
			float PlayerHP = 0.0f, PlayerMaxHP = 0.0f, Dist = 0.0f;
			FString TargetPlayerState = TEXT("Idle");
			QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetPlayerState, Dist);

			const ACharacter* HitChar = Cast<ACharacter>(OtherActor);
			const bool bTargetGrounded = HitChar && HitChar->GetCharacterMovement() && !HitChar->GetCharacterMovement()->IsFalling();
			bJumpUpBlocked = bTargetGrounded && (TargetPlayerState == TEXT("Guarding") || TargetPlayerState == TEXT("JustGuarding"));

			if (bJumpUpBlocked)
			{
				CurrentSkillKnockdownType = EKnockdownType::None;
				CurrentSkillKnockdownDuration = 0.0f;
			}
			else
			{
				// 이전 판정에서 해제됐을 수 있으므로 행 설정(에어본) 재적용
				BeginBalanceAction(CurrentBalanceAction, 1.0f, EKnockdownType::AirborneFloor, 0.7f, false);
			}
		}

		// 전투 타임라인: 적중/가드/저스트가드/회피 기록 (대미지 적용 전 플레이어 상태 기준)
		bWeaponHitThisSwing = true;
		LogPlayerHitResult(CurrentActionLogName.IsEmpty() ? TEXT("WeaponAttack") : CurrentActionLogName, AttackDamage);

		// 대미지 적용 ( 어택 대미지는 공격 전 가드 공격인지 아님 일반 공격인지에 따라 각각 함수에서 설정 )
		UGameplayStatics::ApplyDamage(OtherActor, AttackDamage, GetController(),
			this, UDamageType::StaticClass());

		if ( bIsJumpUpAttack && !bJumpUpBlocked )
		{
			// 플레이어 캐릭터 캐스팅 (가드로 막힌 경우는 띄우지 않음 → 공중 연계 없음)
			ACharacter* HitCharacter = Cast<ACharacter>(OtherActor);
			if (HitCharacter)
			{
				HitCharacter->LaunchCharacter(FVector(0.f, 0.f, 900.f), false, true);
				bSuccessJumpUpAttack = true; // 띄우기 성공 여부를 true로 설정
			}
		}
		else if ( AttackType == EMagicSwordManAttackType::AirAttack )
		{
			// 공중 공격 피격 시 로직 추가
			ACharacter* HitCharacter = Cast<ACharacter>(OtherActor);
			if (HitCharacter)
			{
				//HitCharacter->LaunchCharacter(FVector(0.f, 0.f, 10.f), false, true);
			}
		}
		
		if ( AttackType == EMagicSwordManAttackType::CloseAttack )
		{
			BossMagicSwordManLogData.CloseAttackDamage += AttackDamage;
			CommonBossLogData.TotalDamageDealt += AttackDamage;
		}
		else if ( AttackType == EMagicSwordManAttackType::DashAttack )
		{
			BossMagicSwordManLogData.DashAttackDamage += AttackDamage;
			CommonBossLogData.TotalDamageDealt += AttackDamage;
		}
		else if ( AttackType == EMagicSwordManAttackType::CloseJumpUpAttack )
		{
			BossMagicSwordManLogData.CloseJumpUpAttackDamage += AttackDamage;
			CommonBossLogData.TotalDamageDealt += AttackDamage;
		}
		else if ( AttackType == EMagicSwordManAttackType::DashJumpUpAttack )
		{
			BossMagicSwordManLogData.DashJumpUpAttackDamage += AttackDamage;
			CommonBossLogData.TotalDamageDealt += AttackDamage;
		}
		else if ( AttackType == EMagicSwordManAttackType::GuardCounterAttack )
		{
			BossMagicSwordManLogData.GuardCounterAttackDamage += AttackDamage;
			CommonBossLogData.TotalDamageDealt += AttackDamage;
		} 
		else if ( AttackType == EMagicSwordManAttackType::AirAttack )
		{
			BossMagicSwordManLogData.AirAttackDamage += AttackDamage;
			CommonBossLogData.TotalDamageDealt += AttackDamage;
		}
		else if ( AttackType == EMagicSwordManAttackType::JumpAttack )
		{
			BossMagicSwordManLogData.JumpAttackDamage += AttackDamage;
			CommonBossLogData.TotalDamageDealt += AttackDamage;
		}
		
		// 다시 콜리전 끄기
		if ( WeaponCollision ) WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AttackDamage = 0.f; // 대미지 초기화
	}
}

void ABossMagicSwordMan::AttackStart_WeaponCollision()
{
	if ( WeaponCollision ) WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	bWeaponHitThisSwing = false;
	LogActionRelease(TEXT("WeaponSwing"));
}

void ABossMagicSwordMan::AttackEnd_WeaponCollision()
{
	if ( WeaponCollision ) WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LogActionEnd(bWeaponHitThisSwing, TEXT("Player Dodged Sword"));
}

void ABossMagicSwordMan::SetAttackDamage(float DamageToApply)
{
	AttackDamage = GetBalanceDamage(CurrentBalanceAction, DamageToApply);
}

void ABossMagicSwordMan::BeginBalanceAction(FName ActionName, float DefaultHitIntensity, EKnockdownType DefaultKnockdownType, float DefaultKnockdownDuration, bool bStartNewAction)
{
	CurrentBalanceAction = ActionName;
	ApplySkillBalanceReaction(ActionName, DefaultHitIntensity, DefaultKnockdownType, DefaultKnockdownDuration);

	if (bStartNewAction)
	{
		// 새 패턴 시작 시 이전 패턴의 가드 불가 상태 해제 (필요한 패턴은 아래에서 다시 부여)
		Tags.Remove(TEXT("Unblockable"));

		// 가드/패링 불가 패턴
		// - AirAttack: 띄우기 후 공중 연계 (띄우기 첫 타는 가드 가능 - 막으면 띄워지지 않고 연계 끊김)
		// - GuardCounterAttack: 보스 가드를 때렸을 때 나오는 반격 (가드 위를 두드리는 플레이에 대한 페널티)
		if (ActionName == TEXT("AirAttack") || ActionName == TEXT("GuardCounterAttack"))
		{
			Tags.AddUnique(TEXT("Unblockable"));
		}

		LogActionWindup(ActionName.ToString());
	}
}

void ABossMagicSwordMan::LogActionWindup(const FString& ActionName)
{
	CurrentActionLogName = ActionName;
	CurrentActionID = ActionName;
	bWeaponHitThisSwing = false;

	if (!UCombatTimelineLogger::IsSessionActive()) return;

	float PlayerHP = 0.0f, PlayerMaxHP = 0.0f, Dist = 0.0f;
	FString TargetPlayerState = TEXT("Idle");
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetPlayerState, Dist);

	CurrentActionID = UCombatTimelineLogger::LogSkillWindup(
		ActionName, GetWorld()->GetTimeSeconds(), Dist, TargetPlayerState, Health, MaxHealth, PlayerHP, PlayerMaxHP);
}

void ABossMagicSwordMan::LogActionRelease(const FString& Details)
{
	if (!UCombatTimelineLogger::IsSessionActive() || CurrentActionID.IsEmpty()) return;

	float PlayerHP = 0.0f, PlayerMaxHP = 0.0f, Dist = 0.0f;
	FString TargetPlayerState = TEXT("Idle");
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetPlayerState, Dist);

	UCombatTimelineLogger::LogSkillRelease(
		CurrentActionID, GetWorld()->GetTimeSeconds(), Dist, TargetPlayerState, Health, MaxHealth, PlayerHP, PlayerMaxHP, Details);
}

void ABossMagicSwordMan::LogPlayerHitResult(const FString& ActionName, float Damage, const FString& Details)
{
	if (!UCombatTimelineLogger::IsSessionActive()) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	float PlayerHP = 0.0f, PlayerMaxHP = 0.0f, Dist = 0.0f;
	FString TargetPlayerState = TEXT("Idle");
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetPlayerState, Dist);

	const FString ActionID = CurrentActionID.IsEmpty() ? ActionName : CurrentActionID;

	const bool bUnblockable = Tags.Contains(TEXT("Unblockable"));

	if (!bUnblockable && TargetPlayerState == TEXT("JustGuarding"))
	{
		UCombatTimelineLogger::LogHitJustGuard(ActionID, ActionName, CurrentTime, Dist, Health, MaxHealth, PlayerHP, PlayerMaxHP);
	}
	else if (TargetPlayerState == TEXT("Rolling"))
	{
		UCombatTimelineLogger::LogHitMissed(ActionID, ActionName, CurrentTime, Dist, TEXT("Player Evaded with Roll"));
	}
	else if (!bUnblockable && TargetPlayerState == TEXT("Guarding"))
	{
		UCombatTimelineLogger::LogHitGuarded(ActionID, ActionName, Damage, CurrentTime, Dist, Health, MaxHealth, PlayerHP, PlayerMaxHP);
	}
	else
	{
		const float EstimatedAfterHP = FMath::Max(0.0f, PlayerHP - Damage);
		UCombatTimelineLogger::LogHitSuccess(ActionID, ActionName, Damage, CurrentTime, Dist, TargetPlayerState, Health, MaxHealth, EstimatedAfterHP, PlayerMaxHP, Details);
	}
}

void ABossMagicSwordMan::LogActionEnd(bool bHitPlayer, const FString& MissReason)
{
	if (!UCombatTimelineLogger::IsSessionActive() || CurrentActionID.IsEmpty()) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	float PlayerHP = 0.0f, PlayerMaxHP = 0.0f, Dist = 0.0f;
	FString TargetPlayerState = TEXT("Idle");
	QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetPlayerState, Dist);

	if (!bHitPlayer)
	{
		UCombatTimelineLogger::LogHitMissed(CurrentActionID, CurrentActionLogName, CurrentTime, Dist, MissReason);
	}
	UCombatTimelineLogger::LogSkillRecovery(CurrentActionID, CurrentTime, Health, MaxHealth, PlayerHP, PlayerMaxHP);
}

float ABossMagicSwordMan::GetBalanceDamage(FName ActionName, float FallbackDamage) const
{
	if (const FBossCombatBalanceRow* Row = FindSkillBalance(ActionName))
	{
		if (Row->DamageMultiplier > 0.0f)
		{
			return BaseAttackDamage * Row->DamageMultiplier;
		}
	}
	return FallbackDamage;
}

UAnimMontage* ABossMagicSwordMan::StartCloseAttack()
{
	if (CloseAttackMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, CloseAttackMontages.Num() - 1);
		
		if (CloseAttackMontages[RandomIndex])
		{
			PlayAnimMontage(CloseAttackMontages[RandomIndex]);
			AttackType = EMagicSwordManAttackType::CloseAttack;
			BeginBalanceAction(TEXT("CloseAttack"), 0.6f);

			BossMagicSwordManLogData.CloseAttackCount++;
			
			if (BlackboardComp) BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.CloseAttackDelay);
			return CloseAttackMontages[RandomIndex];
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("array is EMPTY"));
	return nullptr;
}

UAnimMontage* ABossMagicSwordMan::StartDashAttack()
{	
	if (DashAttackMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, DashAttackMontages.Num() - 1);
		
		if (DashAttackMontages[RandomIndex])
		{
			PlayAnimMontage(DashAttackMontages[RandomIndex]);
			AttackType = EMagicSwordManAttackType::DashAttack;
			BeginBalanceAction(TEXT("DashAttack"), 0.8f);

			BossMagicSwordManLogData.DashAttackCount++;
			
			if (BlackboardComp) BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.DashAttackDelay);
			return DashAttackMontages[RandomIndex];
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("array is EMPTY"));
	return nullptr;
}

UAnimMontage* ABossMagicSwordMan::StartCloseJumpUpAttack()
{
	if (CloseJumpUpAttackMontage)
	{
		PlayAnimMontage(CloseJumpUpAttackMontage);
		AttackType = EMagicSwordManAttackType::CloseJumpUpAttack;
		BeginBalanceAction(TEXT("CloseJumpUpAttack"), 1.0f);

		BossMagicSwordManLogData.CloseJumpUpAttackCount++;
		
		// 플라잉 모드로 전환
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		bSuccessJumpUpAttack = false; // 공격 시작 시점에는 띄우기 성공 여부를 false로 초기화
		if (BlackboardComp)
		{
			BlackboardComp->SetValueAsBool("AirAttack", false);
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.CloseJumpUpAttackDelay);
		}
		return CloseJumpUpAttackMontage;
	}
	return nullptr;
}

UAnimMontage* ABossMagicSwordMan::StartDashJumpUpAttack()
{
	if (DashJumpUpAttackMontage)
	{
		PlayAnimMontage(DashJumpUpAttackMontage);
		AttackType = EMagicSwordManAttackType::DashJumpUpAttack;
		BeginBalanceAction(TEXT("DashJumpUpAttack"), 1.0f);

		BossMagicSwordManLogData.DashJumpUpAttackCount++;
		
		// 플라잉 모드로 전환
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		bSuccessJumpUpAttack = false; // 공격 시작 시점에는 띄우기 성공 여부를 false로 초기화
		if (BlackboardComp)
		{
			BlackboardComp->SetValueAsBool("AirAttack", false);
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.DashJumpUpAttackDelay);
		}
		return DashJumpUpAttackMontage;
	}
	return nullptr;
}

void ABossMagicSwordMan::JumpUpAttackCheck()
{
	UE_LOG( LogTemp, Warning, TEXT("JumpUpAttackCheck called. bSuccessJumpUpAttack: %s"), bSuccessJumpUpAttack ? TEXT("true") : TEXT("false") );

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	if ( bSuccessJumpUpAttack == true )
	{
		BossMagicSwordManLogData.JumpUpAttackSuccessCount++;
		
		// 띄우기 성공 시 공중 공격 패턴으로 전환
		BlackboardComp->SetValueAsBool("AirAttack", true);
		
		BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.AirAttackDelay);
	}
	else
	{
		// 띄우기 실패 → 공중 공격으로 이어지지 않으므로 가드 불가 해제
		Tags.Remove(TEXT("Unblockable"));
	}
}

UAnimMontage* ABossMagicSwordMan::StartAirAttack()
{
	if (AirAttackMontage)
	{
		PlayAnimMontage(AirAttackMontage);
		AttackType = EMagicSwordManAttackType::AirAttack;
		BeginBalanceAction(TEXT("AirAttack"), 1.0f);

		BossGravityScaleBeforeAirAttack = GetCharacterMovement()->GravityScale;
		// 보스랑 타깃 캐릭터의 중력 설정
		GetCharacterMovement()->GravityScale = 0.78f;

		// 타깃 캐릭터의 중력 스케일 조절
		if (TargetCharacter)
		{
			// TargetCharacter가 ACharacter 타입인지 확인 후 캐스팅
			if (ACharacter* CastedTarget = Cast<ACharacter>(TargetCharacter))
			{
				TargetCharacterGravityScaleBeforeAirAttack = CastedTarget->GetCharacterMovement()->GravityScale;
				
				CastedTarget->GetCharacterMovement()->GravityScale = 0.75f;
				
			}
		}
		return AirAttackMontage;
	}
	return nullptr;
}

void ABossMagicSwordMan::AirAttackEnd()
{
	// 공중 연계 종료 → 가드 불가 해제
	Tags.Remove(TEXT("Unblockable"));

	// 공중 공격이 끝났을 때 중력 스케일을 원래대로 복구
	GetCharacterMovement()->GravityScale = BossGravityScaleBeforeAirAttack;
	if (TargetCharacter)
	{
		if (ACharacter* CastedTarget = Cast<ACharacter>(TargetCharacter))
		{
			CastedTarget->GetCharacterMovement()->GravityScale = TargetCharacterGravityScaleBeforeAirAttack;
		}
	}
}

UAnimMontage* ABossMagicSwordMan::StartJumpAttack()
{
	if (JumpAttackMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, JumpAttackMontages.Num() - 1);
		
		if (JumpAttackMontages[RandomIndex])
		{
			PlayAnimMontage(JumpAttackMontages[RandomIndex]);
			AttackType = EMagicSwordManAttackType::JumpAttack;
			BeginBalanceAction(TEXT("JumpAttack"), 1.0f, EKnockdownType::Kneel, 1.0f);

			BossMagicSwordManLogData.JumpAttackCount++;
			
			if (BlackboardComp) BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.JumpAttackDelay);
			return JumpAttackMontages[RandomIndex];
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("array is EMPTY"));
	return nullptr;
}

void ABossMagicSwordMan::JumpStart()
{
	LaunchCharacter( FVector(0.f, 0.f, 800.f), false, true);
}

UAnimMontage* ABossMagicSwordMan::StartGuardReactionAttack()
{
	if (GuardReactionMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, GuardReactionMontages.Num() - 1);
		
		if (GuardReactionMontages[RandomIndex])
		{
			BossMagicSwordManLogData.GuardCounterAttackCount++;
			
			PlayAnimMontage(GuardReactionMontages[RandomIndex]);
			AttackType = EMagicSwordManAttackType::GuardCounterAttack;
			BeginBalanceAction(TEXT("GuardCounterAttack"), 1.0f);
			return GuardReactionMontages[RandomIndex];
		}
	}
	return nullptr;
}

UAnimMontage* ABossMagicSwordMan::StartPowerAttack()
{
	if (PowerAttackMontage)
	{
		PlayAnimMontage(PowerAttackMontage);
		BeginBalanceAction(TEXT("PowerAttack"), 0.3f);
		bIsPowerAttackHit = false; // 공격 시작 시점에는 궁극기 공격이 적중했는지 여부를 false로 초기화
		if (BlackboardComp) BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.PowerAttackDelay);
		return PowerAttackMontage;
	}
	return nullptr;
}

void ABossMagicSwordMan::OnBeginOverlapPowerAttackCollisionSphere(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		HandlePowerAttackDamage(OtherActor);
		
		bIsPowerAttackHit = true; // 궁극기 공격이 적중했음을 표시하는 플래그를 true로 설정

		// 타겟 정지 및 입력 차단 로직 시작 
		if (ACharacter* HitCharacter = Cast<ACharacter>(OtherActor))
		{
			//  플레이어 컨트롤러를 가져와서 입력을 비활성화
			if (APlayerController* PlayerController = Cast<APlayerController>(HitCharacter->GetController()))
			{
				HitCharacter->DisableInput(PlayerController);
			}

			//무브먼트 컴포넌트를 가져와서 움직임을 정지
			if (UCharacterMovementComponent* MovementComp = HitCharacter->GetCharacterMovement())
			{
				MovementComp->StopMovementImmediately(); // 현재 속도와 가속도를 0으로 만듦
				MovementComp->SetMovementMode(EMovementMode::MOVE_None); // 이동 모드를 끄면 중력 영향을 받지 않고 그 자리에 멈춤
			}
			// 애니메이션도 멈추기 
			if (USkeletalMeshComponent* StopMesh = HitCharacter->GetMesh())
			{
				StopMesh->bPauseAnims = true;
			}
		}
		
		// 궁극기 공격이 한 번 적중하면 콜리전 비활성화 
		PowerAttackCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABossMagicSwordMan::HandlePowerAttackDamage(AActor* OtherActor)
{
	if (!OtherActor) return;

	// 1. 반응성을 위해 즉시 1회 대미지 적용 (DT_BossCombatBalance PowerAttack 행 우선)
	const float InitialHitDamage = GetBalanceDamage(TEXT("PowerAttack"), AttackDamage);
	LogPlayerHitResult(TEXT("PowerAttack"), InitialHitDamage, TEXT("Grab Start"));
	UGameplayStatics::ApplyDamage(OtherActor, InitialHitDamage, GetController(), this, UDamageType::StaticClass());

	BossMagicSwordManLogData.PowerAttackDamage += InitialHitDamage;
	CommonBossLogData.TotalDamageDealt += InitialHitDamage;
	
	// 2. 파라미터(OtherActor)를 전달하기 위해 델리게이트 생성
	FTimerDelegate TimerCallback;
	TimerCallback.BindUObject(this, &ABossMagicSwordMan::OnPowerAttackTimerTick, OtherActor);

	// 3. 0.3초 간격으로 반복(true) 실행되는 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(PowerAttackTimerHandle, TimerCallback, 0.05f, true);
}

void ABossMagicSwordMan::StartPowerAttackCollision()
{
	if ( PowerAttackCollisionSphere )
	{
		PowerAttackCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	PowerAttackTickDamageAccum = 0.0f;
	LogActionRelease(TEXT("PowerAttack Grab Window"));
}

void ABossMagicSwordMan::EndPowerAttackCollision()
{
	if ( PowerAttackCollisionSphere )
	{
		PowerAttackCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// 공격이 끝나면 타이머를 해제하여 대미지를 멈춤
	GetWorld()->GetTimerManager().ClearTimer(PowerAttackTimerHandle);

	// 궁극기 틱 대미지는 합산하여 1회 기록
	if (PowerAttackTickDamageAccum > 0.0f)
	{
		LogPlayerHitResult(TEXT("PowerAttackTick"), PowerAttackTickDamageAccum,
			FString::Printf(TEXT("Ticks Total (%.1f per tick)"), AttackStruct.PowerAttackTickDamage));
		PowerAttackTickDamageAccum = 0.0f;
	}
	LogActionEnd(bIsPowerAttackHit, TEXT("Player Escaped Grab"));
}

void ABossMagicSwordMan::OnPowerAttackTimerTick(AActor* TargetActor)
{
	// 타겟이 유효한지(파괴되지 않았는지) 확인
	if (IsValid(TargetActor))
	{
		// 주기적 대미지 적용
		UGameplayStatics::ApplyDamage(TargetActor, AttackStruct.PowerAttackTickDamage,
			GetController(), this, UDamageType::StaticClass());
		PowerAttackTickDamageAccum += AttackStruct.PowerAttackTickDamage;
		// 대미지 로그
		UE_LOG(LogTemp, Warning, TEXT("Boss Power Attack Tick Damage : %f"), AttackStruct.PowerAttackTickDamage);
	}
	else
	{
		// 타겟이 유효하지 않으면 타이머 정지
		GetWorld()->GetTimerManager().ClearTimer(PowerAttackTimerHandle);
	}
}

void ABossMagicSwordMan::FinishPowerAttack()
{
	if ( bIsPowerAttackHit )
	{
		BeginBalanceAction(TEXT("PowerAttackFinish"), 1.0f, EKnockdownType::AirborneFloor, 1.5f, false);
		LogPlayerHitResult(TEXT("PowerAttackFinish"), AttackStruct.PowerAttackFinishDamage);
		UGameplayStatics::ApplyDamage(TargetCharacter, AttackStruct.PowerAttackFinishDamage,
			GetController(), this, UDamageType::StaticClass());

		// 피니쉬 대미지 로그
		UE_LOG(LogTemp, Warning, TEXT("Boss Power Attack Finish Damage : %f"), AttackStruct.PowerAttackFinishDamage);
		
		BossMagicSwordManLogData.PowerAttackDamage += AttackStruct.PowerAttackFinishDamage;
		CommonBossLogData.TotalDamageDealt += AttackStruct.PowerAttackFinishDamage;
		
		// 타겟 움직임 및 입력 복구 
		if (ACharacter* HitCharacter = Cast<ACharacter>(TargetCharacter))
		{
			// 이동 모드 복구 
			if (UCharacterMovementComponent* MovementComp = HitCharacter->GetCharacterMovement())
			{
				MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);
			}

			// 플레이어 입력 다시 활성화
			if (APlayerController* PlayerController = Cast<APlayerController>(HitCharacter->GetController()))
			{
				HitCharacter->EnableInput(PlayerController);
			}
			
			// 애니메이션 재생 다시 활성화
			if (USkeletalMeshComponent* StopMesh = HitCharacter->GetMesh())
			{
				StopMesh->bPauseAnims = false;
			}
		}
	}
}

UAnimMontage* ABossMagicSwordMan::StartBladeWaveAttack()
{
	if (BladeWaveAttackMontage)
	{
		BossMagicSwordManLogData.BladeWaveAttackCount++;

		PlayAnimMontage(BladeWaveAttackMontage);
		BeginBalanceAction(TEXT("BladeWave"), 0.8f);
		if (BlackboardComp) BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.BladeWaveAttackDelay);
		return BladeWaveAttackMontage;
	}
	return nullptr;
}

void ABossMagicSwordMan::StartBladeWave()
{
	if (BladeWaveProjectileClass)
	{
		// 정면으로 발사체 스폰 . 약간 앞에서만
		FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 150.f;
		FRotator SpawnRotation = GetActorRotation();

		// Owner를 보스로 지정해야 플레이어 쪽에서 보스 공격(피격 리액션 대상)으로 인식됨
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;

		LogActionRelease(TEXT("BladeWave Projectile Fired"));

		if (ABaseEnemyProjectile* BladeWave = GetWorld()->SpawnActor<ABaseEnemyProjectile>(BladeWaveProjectileClass, SpawnLocation, SpawnRotation, SpawnParams))
		{
			// 발사체가 적중/빗나감을 스스로 타임라인에 기록하도록 ActionID 전달
			BladeWave->ActionID = CurrentActionID;
			BladeWave->ActionName = TEXT("BladeWave");

			// DT_BossCombatBalance BladeWave 행 연동 (행이 없으면 발사체 BP 기본값 유지)
			if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("BladeWave")))
			{
				if (Row->DamageMultiplier > 0.0f)
				{
					BladeWave->SetDamage(BaseAttackDamage * Row->DamageMultiplier);
				}
				if (Row->HitIntensity > 0.0f)
				{
					BladeWave->HitIntensity = Row->HitIntensity;
				}
				BladeWave->KnockdownType = Row->KnockdownType;
				BladeWave->KnockdownDuration = Row->KnockdownDuration;
			}
		}
	}
}

UAnimMontage* ABossMagicSwordMan::StartDashBack()
{
	if ( DashBackMontage )
	{
		BossMagicSwordManLogData.BackDashCount++;
		LogActionWindup(TEXT("BackDash"));

		PlayAnimMontage(DashBackMontage);
		
		if (BlackboardComp) BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.BackDashDelay);
		
		return DashBackMontage;
	}
	return nullptr;
}

void ABossMagicSwordMan::StartSecondPhase()
{
	if ( BlackboardComp == nullptr ) return;

	BlackboardComp->SetValueAsBool("SecondPhase", true);

	if (UCombatTimelineLogger::IsSessionActive())
	{
		float PlayerHP = 0.0f, PlayerMaxHP = 0.0f, Dist = 0.0f;
		FString TargetPlayerState = TEXT("Idle");
		QueryTargetPlayerData(PlayerHP, PlayerMaxHP, TargetPlayerState, Dist);
		UCombatTimelineLogger::LogPhaseTransition(TEXT("SecondPhase"), GetWorld()->GetTimeSeconds(), Health, MaxHealth, PlayerHP, PlayerMaxHP,
			FString::Printf(TEXT("HP Ratio <= %.0f%%"), SecondPhaseHealthThreshold * 100.0f));
	}
}

UAnimMontage* ABossMagicSwordMan::PlaySecondPhaseMontage()
{
	if ( SecondPhaseMontage )
	{
		PlayAnimMontage(SecondPhaseMontage);
		BeginBalanceAction(TEXT("RushStrike"), 1.0f, EKnockdownType::AirborneFloor, 1.5f);
		return SecondPhaseMontage;
	}
	return nullptr;
}

void ABossMagicSwordMan::StartHover()
{
	if (!HoverLocationPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[소드맨] HoverLocationPoint가 설정되지 않았습니다!"));
		return;
	}
	
	// 1. 목표 위치 계산 (기준점 + 높이 700)
	FVector BaseLocation = HoverLocationPoint->GetActorLocation();
	FVector GoalLocation = BaseLocation + FVector(0.f, 0.f, 700.f);

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	
	// 2. 모션 워핑 타겟 업데이트 (회전은 현재 보스 회전 유지 또는 타겟 방향)
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
		FName("WarpTarget"), // 몽타주 노티파이에서 사용할 이름
		GoalLocation,
		GetActorRotation()
	);
}

void ABossMagicSwordMan::SetGravityCharacter()
{
	if (!GetCharacterMovement()) return;
	
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	
	// 캡슐 콜리전 다시 켜기
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	// 중력값 저장
	DefaultGravityScaleBeforeHover = GetCharacterMovement()->GravityScale;

	GetCharacterMovement()->GravityScale = 0.05f;
}

void ABossMagicSwordMan::RushToPlayer()
{
	if (!GetCharacterMovement()) return;
	// 중력 스케일 원상 복구
	GetCharacterMovement()->GravityScale = DefaultGravityScaleBeforeHover;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
}

void ABossMagicSwordMan::RushStrike()
{
	float DamageRadius = 400.f;     // 공격 범위 반지름
	// 참고하신 로직의 힘 설정 (필요 시 헤더파일의 AttackStruct 등에 정의하여 사용하세요)
	float PushForce = 600.f;       // 수평으로 밀어내는 힘
	float PushUpwardForce = 200.f;  // 위로 띄우는 힘 (참고 코드보다 조금 높게 설정하여 타격감 강화)

	// 1. 발바닥 위치 계산
	float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector FootLocation = GetActorLocation() - FVector(0.f, 0.f, CapsuleHalfHeight);

	// 2. 범위 내 모든 액터 감지 (OverlapMulti)
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(DamageRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); 

	bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, FootLocation, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams);

	bool bRushStrikeHitPlayer = false;
	LogActionRelease(TEXT("RushStrike Ground Impact"));

	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* HitActor = Result.GetActor();
			if (HitActor && HitActor->ActorHasTag(FName("Player")))
			{
				// 3. 대미지 적용 (DT_BossCombatBalance RushStrike 행의 피격/넉다운 설정 적용)
				BeginBalanceAction(TEXT("RushStrike"), 1.0f, EKnockdownType::AirborneFloor, 1.5f, false);
				bRushStrikeHitPlayer = true;
				LogPlayerHitResult(TEXT("RushStrike"), AttackStruct.RushStrikeAttackDamage);
				UGameplayStatics::ApplyDamage(HitActor, AttackStruct.RushStrikeAttackDamage,
					GetController(), this, UDamageType::StaticClass());

				// 4. 플레이어 밀치기 (Radial Launch)
				ACharacter* PlayerChar = Cast<ACharacter>(HitActor);
				if (PlayerChar)
				{
					// 폭발 중심(FootLocation)에서 플레이어로 향하는 수평 방향 계산
					FVector PushDirection = PlayerChar->GetActorLocation() - FootLocation;
					PushDirection.Z = 0.f; // 수평 방향 추출
					PushDirection.Normalize();

					// 수평 힘 + 수직 힘 합산
					const FVector LaunchVelocity = (PushDirection * PushForce) + FVector(0.f, 0.f, PushUpwardForce);

					// 플레이어 캐릭터 밀어냄 (기존 속도 무시)
					PlayerChar->LaunchCharacter(LaunchVelocity, true, true);
				}

				BossMagicSwordManLogData.RushStrikeAttackDamage += AttackStruct.RushStrikeAttackDamage;
				CommonBossLogData.TotalDamageDealt += AttackStruct.RushStrikeAttackDamage;
			}
		}
	}
	LogActionEnd(bRushStrikeHitPlayer, TEXT("Player Out of RushStrike Radius"));

	if ( bDebugMode == true )DrawDebugSphere(GetWorld(), FootLocation, DamageRadius, 12, FColor::Red, false, 2.0f);

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
}

void ABossMagicSwordMan::EndBattleLog()
{
	Super::EndBattleLog();
	
	BossMagicSwordManLogData.Base = CommonBossLogData;
	
	UCSVLog::AddMagicSwordManLog( BossMagicSwordManLogData);
	
	CommonBossLogData = FCommonBossLogData(); // 공통 로그 데이터 초기화
	BossMagicSwordManLogData = FBossMagicSwordManLogData(); // 보스별 로그 데이터 초기화
}

#if WITH_EDITOR
void ABossMagicSwordMan::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 변경된 프로퍼티의 이름을 가져옵니다.
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	// 디버그 모드에 따라 어택, 디텍트, 체이스 범위 구체의 가시성을 설정합니다.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABossMagicSwordMan, bDebugMode))
	{
		if ( bDebugMode == true )
		{
			if ( PowerAttackCollisionSphere ) PowerAttackCollisionSphere->SetVisibility(true);
		}
		else
		{
			if ( PowerAttackCollisionSphere ) PowerAttackCollisionSphere->SetVisibility(false);
		}
	}
}
#endif

void ABossMagicSwordMan::InitializeStatsFromDataTable()
{
	Super::InitializeStatsFromDataTable();

	// ─── 1. 범용 통합 전투 밸런스 테이블(DT_BossCombatBalance) 연동 ───
	// - Weight / ActionDelay: 0보다 클 때만 덮어씀 (0 = 블루프린트 AttackStruct 값 유지)
	// - DamageMultiplier: 근접 패턴은 SetAttackDamage에서, 아래 고정 대미지 패턴은 여기서 BaseAttackDamage × 배율로 주입
	if (CachedSkillBalanceMap.Num() > 0)
	{
		auto ApplyWeightAndDelay = [this](FName ActionName, float& InOutWeight, float* InOutDelay)
		{
			if (const FBossCombatBalanceRow* Row = FindSkillBalance(ActionName))
			{
				if (Row->Weight > 0.0f) InOutWeight = Row->Weight;
				if (InOutDelay && Row->ActionDelay > 0.0f) *InOutDelay = Row->ActionDelay;
			}
		};

		ApplyWeightAndDelay(TEXT("CloseAttack"), AttackStruct.CloseAttackWeight, &AttackStruct.CloseAttackDelay);
		ApplyWeightAndDelay(TEXT("DashAttack"), AttackStruct.DashAttackWeight, &AttackStruct.DashAttackDelay);
		ApplyWeightAndDelay(TEXT("CloseJumpUpAttack"), AttackStruct.CloseJumpUpAttackWeight, &AttackStruct.CloseJumpUpAttackDelay);
		ApplyWeightAndDelay(TEXT("DashJumpUpAttack"), AttackStruct.DashJumpUpAttackWeight, &AttackStruct.DashJumpUpAttackDelay);
		ApplyWeightAndDelay(TEXT("JumpAttack"), AttackStruct.JumpAttackWeight, &AttackStruct.JumpAttackDelay);
		ApplyWeightAndDelay(TEXT("Guard"), AttackStruct.GuardWeight, &AttackStruct.GuardDelay);
		ApplyWeightAndDelay(TEXT("PowerAttack"), AttackStruct.PowerAttackWeight, &AttackStruct.PowerAttackDelay);
		ApplyWeightAndDelay(TEXT("BladeWave"), AttackStruct.BladeWaveAttackWeight, &AttackStruct.BladeWaveAttackDelay);
		ApplyWeightAndDelay(TEXT("BackDash"), AttackStruct.BackDashWeight, &AttackStruct.BackDashDelay);

		// 공중 공격은 가중치 없이 띄우기 성공 시 연계되므로 딜레이만 연동
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("AirAttack")))
		{
			if (Row->ActionDelay > 0.0f) AttackStruct.AirAttackDelay = Row->ActionDelay;
		}

		// 노티파이를 거치지 않는 고정 대미지 패턴
		AttackStruct.RushStrikeAttackDamage = GetBalanceDamage(TEXT("RushStrike"), AttackStruct.RushStrikeAttackDamage);
		AttackStruct.PowerAttackTickDamage = GetBalanceDamage(TEXT("PowerAttackTick"), AttackStruct.PowerAttackTickDamage);
		AttackStruct.PowerAttackFinishDamage = GetBalanceDamage(TEXT("PowerAttackFinish"), AttackStruct.PowerAttackFinishDamage);

		UE_LOG(LogTemp, Log, TEXT("[BossMagicSwordMan] DT_BossCombatBalance applied -> MaxHealth: %.0f, Base: %.0f, RushStrike: %.0f, PowerTick: %.1f, PowerFinish: %.0f"),
			MaxHealth, BaseAttackDamage, AttackStruct.RushStrikeAttackDamage, AttackStruct.PowerAttackTickDamage, AttackStruct.PowerAttackFinishDamage);
		return;
	}

	// ─── 2. 기존 CombatStatHandle (하위 호환성 및 폴백) ───
	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())
	{
		AttackDamage = BaseAttackDamage;
		AttackStruct.RushStrikeAttackDamage = BaseAttackDamage * 0.75f;
		AttackStruct.PowerAttackFinishDamage = BaseAttackDamage * 0.75f;
		AttackStruct.PowerAttackTickDamage = BaseAttackDamage * 0.4f;
	}
}