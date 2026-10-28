#include "EnemyBoss/BlackKnight/BossBlackKnight.h"

#include "NiagaraFunctionLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"
#include "CombatTimelineLogger.h"
#include "CombatLogParticipantInterface.h"

// 깃 허브 업데이트 확인
ABossBlackKnight::ABossBlackKnight()
{
	RushAttackSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RushAttackSphere"));
	RushAttackSphere ->SetupAttachment(GetRootComponent());
	RushAttackSphere->ShapeColor = FColor::Blue;
	RushAttackSphere->SetVisibility(false);
	RushAttackSphere->SetHiddenInGame(false);
	// 콜리전 끄기
	RushAttackSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 최대 걷기 속도 100
	GetCharacterMovement()->MaxWalkSpeed = 100.0f;

	// 보스 식별자 설정
	BossLogId = TEXT("Boss_BlackKnight");
	CombatStatHandle.RowName = TEXT("Boss_BlackKnight");

	// 범용 통합 전투 밸런스 데이터테이블 기본 바인딩 (DT_BossCombatBalance)
	static ConstructorHelpers::FObjectFinder<UDataTable> UniversalBalanceTableObj(TEXT("/Game/HYH/DataAssets/DT_BossCombatBalance.DT_BossCombatBalance"));
	if (UniversalBalanceTableObj.Succeeded())
	{
		UniversalBalanceTableHandle.DataTable = UniversalBalanceTableObj.Object;
	}
}

void ABossBlackKnight::BeginPlay()
{
	Super::BeginPlay();

	if ( RushAttackSphere )
	{
		RushAttackSphere->OnComponentBeginOverlap.AddDynamic(this, &ABossBlackKnight::OnBeginOverlapRushAttackSphere);
	}

	TArray<USphereComponent*> SphereComps;
	GetComponents<USphereComponent>(SphereComps);
	
	// 반복문 돌면서 태그 확인
	for (USphereComponent* Sphere : SphereComps)
	{
		if (Sphere && Sphere->ComponentHasTag(TEXT("Axe")))
		{
			AxeCollisionSphere = Sphere;
			AxeCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABossBlackKnight::OnBeginOverlapAxeCollisionSphere);
			// 콜리전 끄기
			AxeCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break; // 찾았으니 루프 종료
		}
	}
}

void ABossBlackKnight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 스킬 쿨타임 만료 검사 및 가중치 복원
	UpdateSkillCooldowns();
}

void ABossBlackKnight::StartSkillCooldown(FName SkillName)
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

		// 행동 딜레이는 스킬별 고유 ActionDelay 주입
		const float DelayToSet = Info->ActionDelay > 0.0f ? Info->ActionDelay : CommonAttackDelay;
		if (BlackboardComp)
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", DelayToSet);
		}

		UE_LOG(LogTemp, Log, TEXT("[BossBlackKnight] Skill [%s] on Cooldown for %.1fs (ActionDelay: %.1fs)"),
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

void ABossBlackKnight::UpdateSkillCooldowns()
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
			const float CurrentDist = TargetCharacter ? FVector::Dist(GetActorLocation(), TargetCharacter->GetActorLocation()) : 9999.0f;
			const bool bIsCloseRange = (CurrentDist <= 400.0f);
			bool bValidForCurrentRange = true;
			if (Pair.Key == TEXT("NormalAttack") || Pair.Key == TEXT("Guard"))
			{
				bValidForCurrentRange = bIsCloseRange;
			}
			else if (Pair.Key == TEXT("RushAttack") || Pair.Key == TEXT("ZapAttack"))
			{
				bValidForCurrentRange = !bIsCloseRange;
			}
			const float CurrentW = BlackboardComp->GetValueAsFloat(Info.BlackboardKey);
			if (bValidForCurrentRange && CurrentW <= 0.0f)
			{
				// 쿨다운 만료: 원래 가중치로 복구
				BlackboardComp->SetValueAsFloat(Info.BlackboardKey, Info.BaseWeight);
				UE_LOG(LogTemp, Log, TEXT("[BossBlackKnight] Skill [%s] Cooldown Finished -> Restored Weight to %.1f"),
					*Pair.Key.ToString(), Info.BaseWeight);
			}
		}
	}
}

bool ABossBlackKnight::IsSkillOnCooldown(FName SkillOrBlackboardKey) const
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

void ABossBlackKnight::Die()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ZapStrikeTimerHandle);
	}
	Super::Die();
}

float ABossBlackKnight::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	if ( bIsGuarding )
	{
		DamageWhileGuarding += DamageAmount;

		// ─── [보스 가드 성공 타임라인 로깅] ───
		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f;
			float CurPlayerMaxHP = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			float CurDist = 0.0f;
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

			UCombatTimelineLogger::LogHitGuarded(
				TEXT("BossGuard"),
				TEXT("ShieldBlock"),
				DamageAmount,
				GetWorld()->GetTimeSeconds(),
				CurDist,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}

		// 아직 리액션 대미지에 도달하지 않았으면 가드 몽타주 재생
		if ( DamageWhileGuarding < AttackStruct.MaxDamageToReaction && GuardMontage )
		{
			PlayAnimMontage(GuardMontage);
		}

		return 0.f; // 대미지 무효화
	}
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ABossBlackKnight::OnBeginOverlapAxeCollisionSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack Sphere Overlapped with Player!"));
		bAxeHitPlayer = true;

		// ─── [도끼 공격 적중/방어 타임라인 로깅] ───
		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f;
			float CurPlayerMaxHP = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			float CurDist = 0.0f;
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

			FString CurrentActionID = CurrentNormalAttackActionID;
			FString CurrentActionName = TEXT("NormalAttack");
			if (AttackDamage == AttackStruct.ChargeAttackDamage)
			{
				CurrentActionID = CurrentChargeAttackActionID;
				CurrentActionName = TEXT("ChargeAttack");
			}
			else if (AttackDamage == AttackStruct.GuardAttackDamage)
			{
				CurrentActionID = CurrentGuardAttackActionID;
				CurrentActionName = TEXT("GuardAttack");
			}

			const float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurPlayerState == TEXT("JustGuarding"))
			{
				UCombatTimelineLogger::LogHitJustGuard(CurrentActionID, CurrentActionName, CurrentTime, CurDist, Health, MaxHealth, CurPlayerHP, CurPlayerMaxHP);
			}
			else if (CurPlayerState == TEXT("Guarding"))
			{
				UCombatTimelineLogger::LogHitGuarded(CurrentActionID, CurrentActionName, AttackDamage, CurrentTime, CurDist, Health, MaxHealth, CurPlayerHP, CurPlayerMaxHP);
			}
			else
			{
				const float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackDamage);
				UCombatTimelineLogger::LogHitSuccess(CurrentActionID, CurrentActionName, AttackDamage, CurrentTime, CurDist, CurPlayerState, Health, MaxHealth, EstimatedAfterHP, CurPlayerMaxHP);
			}
		}

		// 대미지 적용 ( 어택 대미지는 공격 전 가드 공격인지 아님 일반 공격인지에 따라 각각 함수에서 설정 )
		UGameplayStatics::ApplyDamage(OtherActor, AttackDamage, GetController(),
			this, UDamageType::StaticClass());

		if ( AttackDamage == AttackStruct.GuardAttackDamage )
		{
		}
		else if ( AttackDamage == AttackStruct.NormalAttackDamage )
		{
		}
		else if ( AttackDamage == AttackStruct.ChargeAttackDamage )
		{
		}
		
		// 다시 콜리전 끄기
		if ( AxeCollisionSphere ) AxeCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABossBlackKnight::AttackStart_AxeCollisionSphere()
{
	bAxeHitPlayer = false;
	if ( AxeCollisionSphere ) AxeCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// ─── [도끼 공격 판정 개시(Release) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		FString CurrentActionID = CurrentNormalAttackActionID;
		FString Detail = TEXT("Normal Attack Swing");
		if (AttackDamage == AttackStruct.ChargeAttackDamage)
		{
			CurrentActionID = CurrentChargeAttackActionID;
			Detail = TEXT("Charge Attack Swing");
		}
		else if (AttackDamage == AttackStruct.GuardAttackDamage)
		{
			CurrentActionID = CurrentGuardAttackActionID;
			Detail = TEXT("Guard Counter Swing");
		}

		UCombatTimelineLogger::LogSkillRelease(
			CurrentActionID,
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP,
			Detail
		);
	}
}

void ABossBlackKnight::AttackEnd_AxeCollisionSphere()
{
	if ( AxeCollisionSphere ) AxeCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ─── [도끼 공격 판정 종료(Recovery/Missed) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		FString CurrentActionID = CurrentNormalAttackActionID;
		FString CurrentActionName = TEXT("NormalAttack");
		if (AttackDamage == AttackStruct.ChargeAttackDamage)
		{
			CurrentActionID = CurrentChargeAttackActionID;
			CurrentActionName = TEXT("ChargeAttack");
		}
		else if (AttackDamage == AttackStruct.GuardAttackDamage)
		{
			CurrentActionID = CurrentGuardAttackActionID;
			CurrentActionName = TEXT("GuardAttack");
		}

		const float CurrentTime = GetWorld()->GetTimeSeconds();
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		if (!bAxeHitPlayer)
		{
			UCombatTimelineLogger::LogHitMissed(
				CurrentActionID,
				CurrentActionName,
				CurrentTime,
				CurDist,
				TEXT("Player Dodged Axe")
			);
		}

		UCombatTimelineLogger::LogSkillRecovery(
			CurrentActionID,
			CurrentTime,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}
}

void ABossBlackKnight::OnBeginOverlapRushAttackSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 오버랩된 액터가 플레이어인지 확인합니다.
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		UE_LOG(LogTemp, Warning, TEXT("Rush Attack Sphere Overlapped with Player!"));
		bRushHitPlayer = true;

		// ─── [돌진 적중/방어 타임라인 로깅] ───
		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f;
			float CurPlayerMaxHP = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			float CurDist = 0.0f;
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

			const float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurPlayerState == TEXT("JustGuarding"))
			{
				UCombatTimelineLogger::LogHitJustGuard(CurrentRushAttackActionID, TEXT("RushAttack"), CurrentTime, CurDist, Health, MaxHealth, CurPlayerHP, CurPlayerMaxHP);
			}
			else if (CurPlayerState == TEXT("Guarding"))
			{
				UCombatTimelineLogger::LogHitGuarded(CurrentRushAttackActionID, TEXT("RushAttack"), AttackStruct.RushAttackDamage, CurrentTime, CurDist, Health, MaxHealth, CurPlayerHP, CurPlayerMaxHP);
			}
			else
			{
				const float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackStruct.RushAttackDamage);
				UCombatTimelineLogger::LogHitSuccess(CurrentRushAttackActionID, TEXT("RushAttack"), AttackStruct.RushAttackDamage, CurrentTime, CurDist, CurPlayerState, Health, MaxHealth, EstimatedAfterHP, CurPlayerMaxHP);
			}
		}

		// 대미지 적용
		UGameplayStatics::ApplyDamage(OtherActor, AttackStruct.RushAttackDamage, GetController(),
			this, UDamageType::StaticClass());

		// --- 플레이어 밀치기 효과 시작 ---
		ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
		if (PlayerCharacter)
		{
			// 1. 밀어낼 방향 계산 (보스 -> 플레이어)
			FVector PushDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
			PushDirection.Z = 0; // 수평 방향으로만 밀도록 Z값을 0으로 설정
			PushDirection.Normalize();

			// 2. 밀어낼 속도 계산 (방향 * 힘 + 위로 띄우는 힘)
			const FVector LaunchVelocity = PushDirection * PushForce + FVector(0.f, 0.f, PushUpwardForce);

			// 3. 플레이어 캐릭터를 밀어냄
			// bXYOverride와 bZOverride를 true로 설정하여 현재 속도를 무시하고 새로운 속도를 적용합니다.
			PlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}

		// 다시 콜리전 끄기
		RushAttackSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABossBlackKnight::RushAttack()
{
	bRushHitPlayer = false;

	// ─── [돌진 선딜레이 시작(Windup) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		CurrentRushAttackActionID = UCombatTimelineLogger::LogSkillWindup(
			TEXT("RushAttack"),
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}

	if ( RushAttackMontage )
	{
		PlayAnimMontage(RushAttackMontage);
		
		bFocusPlayerAfterAttack = false; // 돌진 공격 중에는 포커스 비활성화
		
		StartSkillCooldown(TEXT("RushAttack"));
		ApplySkillBalanceReaction(TEXT("RushAttack"), 1.0f, EKnockdownType::AirborneFloor, 1.5f);
	}
}

void ABossBlackKnight::StartRush()
{
	// 콜리전 켜기: 돌진 중 플레이어와 충돌 감지
	RushAttackSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// ─── [돌진 판정 개시(Release) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		UCombatTimelineLogger::LogSkillRelease(
			CurrentRushAttackActionID,
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP,
			TEXT("Rush Dash Movement Started")
		);
	}
}

void ABossBlackKnight::EndRush()
{
	RushAttackSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ─── [돌진 종료 및 후딜 복귀(Recovery/Missed) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		if (!bRushHitPlayer)
		{
			UCombatTimelineLogger::LogHitMissed(
				CurrentRushAttackActionID,
				TEXT("RushAttack"),
				CurrentTime,
				CurDist,
				TEXT("Player Evaded Rush")
			);
		}

		UCombatTimelineLogger::LogSkillRecovery(
			CurrentRushAttackActionID,
			CurrentTime,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}
}

void ABossBlackKnight::GuardAttack()
{
	bAxeHitPlayer = false;

	// ─── [가드 카운터 선딜레이 시작(Windup) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		CurrentGuardAttackActionID = UCombatTimelineLogger::LogSkillWindup(
			TEXT("GuardAttack"),
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}

	if ( GuardAttackMontage )
	{
		PlayAnimMontage(GuardAttackMontage);
		// 공격 데미지를 가드 공격 데미지로 설정
		AttackDamage = AttackStruct.GuardAttackDamage;
		
		bFocusPlayerAfterAttack = false; // 가드 공격 중에는 포커스 비활성화
		
		StartSkillCooldown(TEXT("Guard"));
		ApplySkillBalanceReaction(TEXT("Guard"), 0.5f, EKnockdownType::None, 0.0f);
		LastWaveParentActionID = CurrentGuardAttackActionID;
	}
}

void ABossBlackKnight::StartWaveAttack()
{
	bIsHitZap = false; // 번개 공격 초기화
	
    const int32 TotalWaves = 4;
    const float TimeBetweenWaves = 0.4f;
    const float BaseDistance = 150.0f; 
    const float DistanceStep = 200.0f;
	
    UWorld* World = GetWorld();
    if (!World) return;

	const FString WaveActionID = !LastWaveParentActionID.IsEmpty()
		? LastWaveParentActionID
		: (!CurrentChargeAttackActionID.IsEmpty() ? CurrentChargeAttackActionID : CurrentGuardAttackActionID);

	// ─── [가드/차지 웨이브 판정 개시(Release) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		UCombatTimelineLogger::LogSkillRelease(
			WaveActionID,
			World->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP,
			TEXT("Wave Zap Discharged")
		);
	}

    FVector StartLocation = (AxeCollisionSphere) ? AxeCollisionSphere->GetComponentLocation() : GetActorLocation();
    FVector FixedForward = GetActorForwardVector();

    TWeakObjectPtr<ABossBlackKnight> WeakThis(this);

    for (int32 i = 0; i < TotalWaves; i++)
    {
        // 웨이브 로직을 별도의 람다로 정의
        auto ExecuteWave = [WeakThis, i, BaseDistance, DistanceStep, World, StartLocation, FixedForward, WaveActionID]()
        {
            if (!WeakThis.IsValid()) return;

            float CurrentDistance = BaseDistance + (i * DistanceStep);
            const TArray<float> Angles = { -30.0f, 0.0f, 30.0f };

            for (float Angle : Angles)
            {
                FVector Direction = FixedForward.RotateAngleAxis(Angle, FVector::UpVector);
                FVector TargetLocation = StartLocation + (Direction * CurrentDistance);

                FHitResult HitResult;
                FVector TraceStart = TargetLocation + FVector(0.f, 0.f, 1000.f);
                FVector TraceEnd = TargetLocation - FVector(0.f, 0.f, 1000.f);

            	FCollisionQueryParams TraceParams;
            	TraceParams.AddIgnoredActor(WeakThis.Get());
            	TraceParams.AddIgnoredActor(WeakThis->TargetCharacter);
            	
                FVector FinalLocation = TargetLocation;
                if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
                {
                    FinalLocation = HitResult.Location;
                }

            	if ( WeakThis->bDebugMode == true ) DrawDebugSphere(World, FinalLocation, 80.0f, 16, FColor::Purple, false, 2.0f);
            	
            	// --- 플레이어 감지 로직 추가 ---
            	TArray<FOverlapResult> OverlapResults;
            	FCollisionShape SphereShape = FCollisionShape::MakeSphere(80.0f); // 디버그 스피어와 동일한 크기
            	FCollisionQueryParams QueryParams;
            	QueryParams.AddIgnoredActor(WeakThis.Get()); // 보스 자신은 제외

            	// ECC_Pawn 채널을 사용하여 주변 액터 검출
            	if (World->OverlapMultiByChannel(OverlapResults, FinalLocation, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams))
            	{
            		for (const FOverlapResult& Result : OverlapResults)
            		{
            			AActor* OverlappedActor = Result.GetActor();
            			if (OverlappedActor && OverlappedActor->ActorHasTag(TEXT("Player")))
            			{
            				if ( WeakThis->bIsHitZap == false && WeakThis->bAxeHitPlayer == false )
            				{
            					UE_LOG(LogTemp, Warning, TEXT("Player Hit At Zap"));
            					WeakThis->bIsHitZap = true;

								// ─── [웨이브 피격/방어 타임라인 로깅] ───
								if (UCombatTimelineLogger::IsSessionActive())
								{
									float CurPlayerHP = 0.0f;
									float CurPlayerMaxHP = 0.0f;
									FString CurPlayerState = TEXT("Idle");
									float CurDist = 0.0f;
									WeakThis->QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
									const float CurrentTime = World->GetTimeSeconds();

									if (CurPlayerState == TEXT("JustGuarding"))
									{
										UCombatTimelineLogger::LogHitJustGuard(WaveActionID, TEXT("GuardWaveZap"), CurrentTime, CurDist, WeakThis->Health, WeakThis->MaxHealth, CurPlayerHP, CurPlayerMaxHP);
									}
									else if (CurPlayerState == TEXT("Guarding"))
									{
										UCombatTimelineLogger::LogHitGuarded(WaveActionID, TEXT("GuardWaveZap"), WeakThis->AttackStruct.ZapDamage, CurrentTime, CurDist, WeakThis->Health, WeakThis->MaxHealth, CurPlayerHP, CurPlayerMaxHP);
									}
									else
									{
										const float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - WeakThis->AttackStruct.ZapDamage);
										UCombatTimelineLogger::LogHitSuccess(WaveActionID, TEXT("GuardWaveZap"), WeakThis->AttackStruct.ZapDamage, CurrentTime, CurDist, CurPlayerState, WeakThis->Health, WeakThis->MaxHealth, EstimatedAfterHP, CurPlayerMaxHP);
									}
								}

								WeakThis->CurrentSkillHitIntensity = 0.5f;
								WeakThis->CurrentSkillKnockdownType = EKnockdownType::None;
								WeakThis->CurrentSkillKnockdownDuration = 0.0f;
								WeakThis->bHasExplicitSkillReaction = true;
            					
            					UGameplayStatics::ApplyDamage(
									OverlappedActor,            // 데미지를 받을 대상
									WeakThis->AttackStruct.ZapDamage,    // 데미지 수치
									WeakThis->GetController(),  // 가해자 컨트롤러
									WeakThis.Get(),             // 가해자 액터 (보스)
									UDamageType::StaticClass()  // 데미지 타입 클래스
								);
				            }
            			}
            		}
            	}
            	// 가드 어택 이펙트 있으면 재생
            	if (WeakThis->ZapEffect)
            	{
            		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						World, 
						WeakThis->ZapEffect, 
						FinalLocation, 
						FixedForward.Rotation(), // 보스가 바라보는 정면 방향으로 이펙트 회전 설정
						FVector(5.f)
					);
            	}
            }
        };
        // 첫 번째 웨이브(i=0)는 즉시 실행, 나머지는 타이머 예약
        if (i == 0)
        {
            ExecuteWave();
        }
        else
        {
            FTimerHandle WaveTimerHandle;
            FTimerDelegate WaveDelegate;
            WaveDelegate.BindLambda(ExecuteWave);
            World->GetTimerManager().SetTimer(WaveTimerHandle, WaveDelegate, i * TimeBetweenWaves, false);
        }
    }
}

void ABossBlackKnight::NormalAttack()
{
	bAxeHitPlayer = false;

	// ─── [기본 공격 선딜레이 시작(Windup) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		CurrentNormalAttackActionID = UCombatTimelineLogger::LogSkillWindup(
			TEXT("NormalAttack"),
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}

	if ( NormalAttackMontage )
	{
		PlayAnimMontage(NormalAttackMontage);

		AttackDamage = AttackStruct.NormalAttackDamage;
		
		bFocusPlayerAfterAttack = false; // 차지 공격 중에는 포커스 비활성화
		
		StartSkillCooldown(TEXT("NormalAttack"));
		ApplySkillBalanceReaction(TEXT("NormalAttack"), 0.5f, EKnockdownType::None, 0.0f);
	}
}

void ABossBlackKnight::ChargeAttack()
{
	bAxeHitPlayer = false;

	// ─── [차지 공격 선딜레이 시작(Windup) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		CurrentChargeAttackActionID = UCombatTimelineLogger::LogSkillWindup(
			TEXT("ChargeAttack"),
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}

	if ( ChargeAttackMontage )
	{
		const float EffectiveChargeRate = (ChargeAttackMontage->RateScale >= 1.4f) ? 1.0f : 1.5f;
		PlayAnimMontage(ChargeAttackMontage, EffectiveChargeRate);

		AttackDamage = AttackStruct.ChargeAttackDamage;
		
		bFocusPlayerAfterAttack = false; // 차지 공격 중에는 포커스 비활성화
		
		StartSkillCooldown(TEXT("ChargeAttack"));
		ApplySkillBalanceReaction(TEXT("ChargeAttack"), 1.0f, EKnockdownType::Kneel, 1.0f);
		LastWaveParentActionID = CurrentChargeAttackActionID;
	}
}

void ABossBlackKnight::SpawnRandomZapEffect()
{
	// [설정] 헤더 변수 대신 함수 내부에서 값 정의
    const int32  SpawnCount      = 7;      // 떨어질 번개 횟수
    const float  SpawnRadius     = 800.0f;  // 보스 기준 생성 반경
    const float  SpawnInterval   = 0.3f;   // 번개 생성 간격 (초)
    const float  DamageRadius    = 50.0f;  // 대미지 판정 범위
    const float  EffectScale     = 3.0f;    // 이펙트 크기 배율

    UWorld* World = GetWorld();
    if (!World) return;

    // 타이머 람다에서 안전하게 접근하기 위해 Weak Pointer 사용
    TWeakObjectPtr<ABossBlackKnight> WeakThis(this);

    for (int32 i = 0; i < SpawnCount; i++)
    {
        // 순차적으로 터지도록 딜레이 설정
        float Delay = i * SpawnInterval;

        FTimerHandle ZapTimerHandle;
        
        // 람다 함수 내부에 로직 구현
        World->GetTimerManager().SetTimer(ZapTimerHandle, [WeakThis, SpawnRadius, DamageRadius, EffectScale]()
        {
            if (!WeakThis.IsValid()) return;
            UWorld* WorldContext = WeakThis->GetWorld();
            if (!WorldContext) return;

            // 1. 랜덤 위치 계산 (보스 주변 원형 범위)
            FVector BossLocation = WeakThis->GetActorLocation();
            FVector RandomDir = FMath::VRand();
            RandomDir.Z = 0.0f; // 수평 방향만 고려
            RandomDir.Normalize();

            // 최소 150cm ~ 최대 SpawnRadius 사이 랜덤 거리
            float RandomDist = FMath::FRandRange(150.0f, SpawnRadius); 
            FVector TargetLocation = BossLocation + (RandomDir * RandomDist);

            // 2. 바닥 위치 보정 (공중에 뜨지 않게 LineTrace)
            FHitResult HitResult;
            FVector TraceStart = TargetLocation + FVector(0.f, 0.f, 500.f);
            FVector TraceEnd = TargetLocation - FVector(0.f, 0.f, 500.f);

        	FCollisionQueryParams TraceParams;
        	TraceParams.AddIgnoredActor(WeakThis.Get());
			TraceParams.AddIgnoredActor(WeakThis->TargetCharacter);
        	
            if (WorldContext->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
            {
                TargetLocation = HitResult.Location;
            }

            // 3. 이펙트 재생 
            if (WeakThis->ZapEffect)
            {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    WorldContext,
                    WeakThis->ZapEffect,
                    TargetLocation,
                    FRotator::ZeroRotator,
                    FVector(EffectScale)
                );
            }

            // (디버그용) 위치 표시
            if ( WeakThis->bDebugMode == true )  DrawDebugSphere(WorldContext, TargetLocation, DamageRadius, 12, FColor::Yellow, false, 1.0f);

        	/*
            // 4. 범위 대미지 판정
            TArray<FOverlapResult> OverlapResults;
            FCollisionShape SphereShape = FCollisionShape::MakeSphere(DamageRadius);
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(WeakThis.Get()); // 보스 자신 제외

            if (WorldContext->OverlapMultiByChannel(OverlapResults, TargetLocation, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams))
            {
                for (const FOverlapResult& Result : OverlapResults)
                {
                    AActor* OverlappedActor = Result.GetActor();
                    // 플레이어 태그 확인
                    if (OverlappedActor && OverlappedActor->ActorHasTag(TEXT("Player")))
                    {
                        // ZapDamage 변수 재사용 (StartWaveAttack에 있는 변수)
                        UGameplayStatics::ApplyDamage(
                            OverlappedActor,
                            WeakThis->ZapDamage, 
                            WeakThis->GetController(),
                            WeakThis.Get(),
                            UDamageType::StaticClass()
                        );
                        
                        // 로그 출력 (필요시 주석 해제)
                        // UE_LOG(LogTemp, Warning, TEXT("Random Zap Hit Player!"));
                    }
                }
            }
            */

        }, Delay, false);
    }
}

void ABossBlackKnight::ZapAttack()
{
	bZapHitPlayer = false;

	// ─── [벼락 공격 선딜레이 시작(Windup) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		CurrentZapAttackActionID = UCombatTimelineLogger::LogSkillWindup(
			TEXT("ZapAttack"),
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}

	if ( ZapAttackMontage )
	{
		PlayAnimMontage(ZapAttackMontage);

		AttackDamage = AttackStruct.ZapDamage;
		
		bFocusPlayerAfterAttack = false; // 잽 공격 중에는 포커스 비활성화
		
		StartSkillCooldown(TEXT("ZapAttack"));
		ApplySkillBalanceReaction(TEXT("ZapAttack"), 0.7f, EKnockdownType::None, 0.0f);
	}
}

void ABossBlackKnight::SetZapTargetLocation()
{
	if (TargetCharacter == nullptr) return;

	// 1. 타겟의 현재 위치를 가져옵니다.
	const FVector CharacterLocation = TargetCharacter->GetActorLocation();
	FVector GoalLocation = CharacterLocation; // 기본 위치

	// 2. 바닥을 찾기 위해 라인 트레이스를 수행합니다. (참고 코드 로직 적용)
	FHitResult HitResult;
	const FVector StartTrace = CharacterLocation;
	const FVector EndTrace = CharacterLocation - FVector(0.f, 0.f, 1000.f); // 아래로 1000 유닛
	
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(TargetCharacter);
	TraceParams.AddIgnoredActor(this);

	// 라인 트레이스로 바닥 위치를 찾습니다.
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_WorldStatic, TraceParams))
	{
		// 충돌 지점을 목표 위치로 설정합니다.
		GoalLocation = HitResult.Location;
	}

	// 3. 최종 계산된 바닥 위치 저장 및 전조 이펙트 표시
	ZapTargetLocation = GoalLocation;
	if (ZapEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ZapEffect, ZapTargetLocation, FRotator::ZeroRotator, FVector(1.8f));
	}
}

void ABossBlackKnight::SpawnZapAttackEffect()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 저장된 위치 사용
	const FVector SpawnLocation = ZapTargetLocation;
	const FString ZapActionID = CurrentZapAttackActionID;

	// ─── [벼락 타겟 확정 및 낙하 카운트다운(Release) 로깅] ───
	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		float CurDist = 0.0f;
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		UCombatTimelineLogger::LogSkillRelease(
			ZapActionID,
			World->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP,
			TEXT("Lightning Target Locked (0.55s Strike Delay)")
		);
	}

	bZapHitPlayer = false;

	// 0.55초 유예 후 실제 낙뢰 스폰 및 판정 (이동 회피 가능하도록)
	const float StrikeDelay = 0.55f;
	TWeakObjectPtr<ABossBlackKnight> WeakThis(this);
	World->GetTimerManager().SetTimer(ZapStrikeTimerHandle, [WeakThis, SpawnLocation, ZapActionID]()
	{
		if (!WeakThis.IsValid() || WeakThis->Health <= 0.0f) return;
		UWorld* InnerWorld = WeakThis->GetWorld();
		if (!InnerWorld) return;

		if (WeakThis->ZapEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(InnerWorld, WeakThis->ZapEffect, SpawnLocation, FRotator::ZeroRotator, FVector(5.f));
		}

		// 대미지 판정 범위 설정 (기존 150.0f -> 110.0f로 축소하여 도보 이탈 허용)
		const float DamageRadius = 110.0f;
		if (WeakThis->bDebugMode == true) DrawDebugSphere(InnerWorld, SpawnLocation, DamageRadius, 12, FColor::Blue, false, 2.0f);

		WeakThis->ApplySkillBalanceReaction(TEXT("ZapAttack"), 0.7f, EKnockdownType::None, 0.0f);

		TArray<FOverlapResult> OverlapResults;
		FCollisionShape SphereShape = FCollisionShape::MakeSphere(DamageRadius);
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(WeakThis.Get());

		if (InnerWorld->OverlapMultiByChannel(OverlapResults, SpawnLocation, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams))
		{
			for (const FOverlapResult& Result : OverlapResults)
			{
				AActor* OverlappedActor = Result.GetActor();
				if (OverlappedActor && OverlappedActor->ActorHasTag(TEXT("Player")))
				{
					WeakThis->bZapHitPlayer = true;

					if (UCombatTimelineLogger::IsSessionActive())
					{
						float CurPlayerHP = 0.0f;
						float CurPlayerMaxHP = 0.0f;
						FString CurPlayerState = TEXT("Idle");
						float CurDist = 0.0f;
						WeakThis->QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
						const float CurrentTime = InnerWorld->GetTimeSeconds();

						if (CurPlayerState == TEXT("JustGuarding"))
						{
							UCombatTimelineLogger::LogHitJustGuard(ZapActionID, TEXT("ZapAttack"), CurrentTime, CurDist, WeakThis->Health, WeakThis->MaxHealth, CurPlayerHP, CurPlayerMaxHP);
						}
						else if (CurPlayerState == TEXT("Guarding"))
						{
							UCombatTimelineLogger::LogHitGuarded(ZapActionID, TEXT("ZapAttack"), WeakThis->AttackStruct.ZapAttackDamage, CurrentTime, CurDist, WeakThis->Health, WeakThis->MaxHealth, CurPlayerHP, CurPlayerMaxHP);
						}
						else
						{
							const float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - WeakThis->AttackStruct.ZapAttackDamage);
							UCombatTimelineLogger::LogHitSuccess(ZapActionID, TEXT("ZapAttack"), WeakThis->AttackStruct.ZapAttackDamage, CurrentTime, CurDist, CurPlayerState, WeakThis->Health, WeakThis->MaxHealth, EstimatedAfterHP, CurPlayerMaxHP);
						}
					}

					UGameplayStatics::ApplyDamage(
						OverlappedActor,
						WeakThis->AttackStruct.ZapAttackDamage,
						WeakThis->GetController(),
						WeakThis.Get(),
						UDamageType::StaticClass()
					);
					UE_LOG(LogTemp, Warning, TEXT("Zap Attack Hit Player!"));
				}
			}
		}

		if (UCombatTimelineLogger::IsSessionActive())
		{
			const float CurrentTime = InnerWorld->GetTimeSeconds();
			float CurPlayerHP = 0.0f;
			float CurPlayerMaxHP = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			float CurDist = 0.0f;
			WeakThis->QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

			if (!WeakThis->bZapHitPlayer)
			{
				UCombatTimelineLogger::LogHitMissed(
					ZapActionID,
					TEXT("ZapAttack"),
					CurrentTime,
					CurDist,
					TEXT("Target Evaded Lightning")
				);
			}

			UCombatTimelineLogger::LogSkillRecovery(
				ZapActionID,
				CurrentTime,
				WeakThis->Health,
				WeakThis->MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}, StrikeDelay, false);
}

#if WITH_EDITOR
void ABossBlackKnight::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 변경된 프로퍼티의 이름을 가져옵니다.
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	// 디버그 모드에 따라 어택, 디텍트, 체이스 범위 구체의 가시성을 설정합니다.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABossBlackKnight, bDebugMode))
	{
		if ( bDebugMode == true )
		{
			if ( RushAttackSphere ) RushAttackSphere->SetVisibility(true);
		}
		else
		{
			if ( RushAttackSphere ) RushAttackSphere->SetVisibility(false);
		}
	}
}
#endif

void ABossBlackKnight::InitializeStatsFromDataTable()
{
	Super::InitializeStatsFromDataTable();

	if (MaxHealth > 0.0f && MaxHealth < 8500.0f)
	{
		MaxHealth = 8500.0f;
		Health = MaxHealth;
	}

	// ─── 1. 범용 통합 전투 밸런스 테이블(DT_BossCombatBalance)에서 스킬 파라미터 주입 ───
	if (CachedSkillBalanceMap.Num() > 0)
	{
		// NormalAttack
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("NormalAttack")))
		{
			AttackStruct.NormalAttackWeight = Row->Weight;
			AttackStruct.NormalAttackDelay = FMath::Min(Row->ActionDelay, 0.6f);
			AttackStruct.NormalAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.0f);
		}

		// RushAttack
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("RushAttack")))
		{
			AttackStruct.RushAttackWeight = Row->Weight;
			AttackStruct.RushAttackDelay = FMath::Min(Row->ActionDelay, 1.2f);
			AttackStruct.RushAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.2f);
		}

		// Guard
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("Guard")))
		{
			AttackStruct.GuardWeight = Row->Weight;
			AttackStruct.GuardDelay = FMath::Min(Row->ActionDelay, 0.8f);
			AttackStruct.GuardAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.0f);
			AttackStruct.ZapDamage = BaseAttackDamage * 0.6f;
		}

		// ChargeAttack
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("ChargeAttack")))
		{
			AttackStruct.ChargeAttackWeight = Row->Weight;
			AttackStruct.ChargeAttackDelay = FMath::Min(Row->ActionDelay, 1.0f);
			AttackStruct.ChargeAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.4f);
		}

		// ZapAttack
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("ZapAttack")))
		{
			AttackStruct.ZapAttackWeight = Row->Weight;
			AttackStruct.ZapAttackDelay = FMath::Min(Row->ActionDelay, 0.8f);
			AttackStruct.ZapAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 0.8f);
		}

		// BaseStat에서 공통 행동 딜레이(CommonAttackDelay) 읽기 (기본값: 0.6초)
		if (const FBossCombatBalanceRow* BaseStatRow = FindSkillBalance(TEXT("BaseStat")))
		{
			if (BaseStatRow->ActionDelay > 0.0f)
			{
				CommonAttackDelay = FMath::Min(BaseStatRow->ActionDelay, 0.6f);
			}
			else if (BaseStatRow->Cooldown > 0.0f)
			{
				CommonAttackDelay = FMath::Min(BaseStatRow->Cooldown, 0.6f);
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

		auto GetSkillCooldown = [this](FName SkillName, float FallbackCooldown) -> float
		{
			if (const FBossCombatBalanceRow* Row = FindSkillBalance(SkillName))
			{
				if (Row->Cooldown > 0.0f)
				{
					return Row->Cooldown;
				}
			}
			return FallbackCooldown;
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

		RegisterSkillCooldown(TEXT("NormalAttack"), TEXT("W_NormalAttack"), GetSkillCooldown(TEXT("NormalAttack"), 2.0f), AttackStruct.NormalAttackDelay, AttackStruct.NormalAttackWeight);
		RegisterSkillCooldown(TEXT("RushAttack"), TEXT("W_RushAttack"), GetSkillCooldown(TEXT("RushAttack"), 5.0f), AttackStruct.RushAttackDelay, AttackStruct.RushAttackWeight);
		RegisterSkillCooldown(TEXT("Guard"), TEXT("W_Guard"), GetSkillCooldown(TEXT("Guard"), 4.0f), AttackStruct.GuardDelay, AttackStruct.GuardWeight);
		RegisterSkillCooldown(TEXT("ChargeAttack"), TEXT("W_ChargeAttack"), GetSkillCooldown(TEXT("ChargeAttack"), 3.0f), AttackStruct.ChargeAttackDelay, AttackStruct.ChargeAttackWeight);
		RegisterSkillCooldown(TEXT("ZapAttack"), TEXT("W_ZapAttack"), GetSkillCooldown(TEXT("ZapAttack"), 2.5f), AttackStruct.ZapAttackDelay, AttackStruct.ZapAttackWeight);

		UE_LOG(LogTemp, Log, TEXT("[BossBlackKnight] Successfully injected all skill balance parameters and cooldowns from DT_BossCombatBalance! (Common Delay: %.1fs)"), CommonAttackDelay);
		return;
	}

	// ─── 2. 기존 CombatStatHandle (하위 호환성 및 폴백) ───
	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())
	{
		AttackStruct.NormalAttackDamage = BaseAttackDamage;
		AttackStruct.RushAttackDamage = BaseAttackDamage * 1.2f;
		AttackStruct.GuardAttackDamage = BaseAttackDamage;
		AttackStruct.ChargeAttackDamage = BaseAttackDamage * 1.4f;
		AttackStruct.ZapAttackDamage = BaseAttackDamage * 0.8f;
		AttackStruct.ZapDamage = BaseAttackDamage * 0.6f;
	}
}

