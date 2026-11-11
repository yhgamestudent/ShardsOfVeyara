#include "EnemyBoss/Worm/BossWorm.h"

#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "EnemyBoss/Worm/BossWormProjectile.h"
#include "EnemyProjectile/BaseStreamProjectile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "CSVLog.h"
#include "CombatTimelineLogger.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"

ABossWorm::ABossWorm()
{
	BossLogId = TEXT("Boss_BossWorm");
	CombatStatHandle.RowName = TEXT("Boss_BossWorm");

	// 범용 통합 전투 밸런스 데이터테이블 기본 바인딩 (DT_BossCombatBalance)
	static ConstructorHelpers::FObjectFinder<UDataTable> UniversalBalanceTableObj(
		TEXT("/Game/HYH/DataAssets/DT_BossCombatBalance.DT_BossCombatBalance")
	);
	if (UniversalBalanceTableObj.Succeeded())
	{
		UniversalBalanceTableHandle.DataTable = UniversalBalanceTableObj.Object;
	}

	// 최대 걷기 속도 제로로 해서 움직이지 않도록 설정
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	
	UnBurrowAttackCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("UnBurrowAttackCollisionSphere"));
	UnBurrowAttackCollisionSphere->SetupAttachment(GetRootComponent());
	UnBurrowAttackCollisionSphere->ShapeColor = FColor::Blue;
	UnBurrowAttackCollisionSphere->SetVisibility(false);
	UnBurrowAttackCollisionSphere->SetHiddenInGame(false);
	// 콜리전 오버랩만 감지하도록 설정
	UnBurrowAttackCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	RangedAttackPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RangedAttackPoint"));
	RangedAttackPoint->SetupAttachment(RootComponent); // 루트 컴포넌트
	
	SuctionRadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("SuctionRadialForceComp"));
	SuctionRadialForceComp->SetupAttachment(RootComponent);
	
	SuctionRadialForceComp->Radius = 10000.f;        // 영향을 미칠 범위
	SuctionRadialForceComp->ForceStrength = AttackStruct.SuctionForce; // 빨아들이는 힘 (음수)
	SuctionRadialForceComp->bImpulseVelChange = false ; // 질량에 따른 힘 조정 여부 (false면 질량이 클수록 덜 빨려감)
	SuctionRadialForceComp->bAutoActivate = false;
}

void ABossWorm::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<UShapeComponent*> ShapeComps;
	GetComponents<UShapeComponent>(ShapeComps);
	
	// 반복문 돌면서 태그 확인 (Capsule, Sphere, Box 등 모든 UShapeComponent 지원)
	for (UShapeComponent* Shape : ShapeComps)
	{
		if (Shape && Shape->ComponentHasTag(TEXT("Attack"))) // "Attack" 태그가 있는 ShapeComponent를 찾습니다.
		{
			AttackCollisionComp = Shape;
			AttackCollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABossWorm::OnBeginOverlapAttackCollisionSphere);
			// 콜리전 끄기
			AttackCollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UE_LOG(LogTemp, Log, TEXT("[BossWorm] Attack Collision Component successfully bound: %s (%s)"),
				*Shape->GetName(), *Shape->GetClass()->GetName());
			break; // 찾았으니 루프 종료
		}
	}
	
	// FireBreath 태그 있는 씬 컴포넌트 찾아서 FireBreathPoint에 할당
	TArray<USceneComponent*> SceneComps;
	GetComponents<USceneComponent>(SceneComps);
	for (USceneComponent* SceneComp : SceneComps)
	{
		if (SceneComp && SceneComp->ComponentHasTag(TEXT("FireBreath")))
		{
			FireBreathPoint = SceneComp;
			break; // 찾았으니 루프 종료
		}
	}
}

void ABossWorm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	HandleSuction(DeltaTime);
}

void ABossWorm::Die()
{
	Super::Die();
	
	// 타이머 정지 (발사 중단)
	GetWorldTimerManager().ClearTimer(FireBreathTimerHandle);
}

void ABossWorm::OnBeginOverlapAttackCollisionSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack Sphere Overlapped with Player!"));

		// 대미지 적용
		UGameplayStatics::ApplyDamage(OtherActor, AttackDamage, GetController(),
			this, UDamageType::StaticClass());

		if ( AttackDamage == AttackStruct.LungeAttackDamage || AttackDamage == AttackStruct.SuctionAttackDamage )
		{
			ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
			
			const float PushForce = 2000.0f;       // 밀어내는 힘
			const float PushUpwardForce = 400.0f;  // 띄우는 힘

			// 밀어낼 방향 계산 (보스 -> 플레이어)
			FVector PushDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
			PushDirection.Z = 0; 
			PushDirection.Normalize();

			// 밀어낼 속도 계산
			const FVector LaunchVelocity = PushDirection * PushForce + FVector(0.f, 0.f, PushUpwardForce);

			// 플레이어 캐릭터를 밀어냄
			PlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}
		
		if ( AttackDamage == AttackStruct.NormalAttackDamage )
		{
			BossWormLogData.NormalAttackDamage += AttackDamage;
			if (UCombatTimelineLogger::IsSessionActive())
			{
				float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
				FString CurPlayerState = TEXT("Idle");
				QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
				float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackDamage);
				UCombatTimelineLogger::LogHitSuccess(
					CurrentNormalAttackActionID,
					TEXT("NormalAttack"),
					AttackDamage,
					GetWorld()->GetTimeSeconds(),
					CurDist,
					CurPlayerState,
					Health,
					MaxHealth,
					EstimatedAfterHP,
					CurPlayerMaxHP
				);
			}
		}
		else if ( AttackDamage == AttackStruct.LungeAttackDamage )
		{
			BossWormLogData.LungeAttackDamage += AttackDamage;
			if (UCombatTimelineLogger::IsSessionActive())
			{
				float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
				FString CurPlayerState = TEXT("Idle");
				QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
				float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackDamage);
				UCombatTimelineLogger::LogHitSuccess(
					CurrentLungeAttackActionID,
					TEXT("LungeAttack"),
					AttackDamage,
					GetWorld()->GetTimeSeconds(),
					CurDist,
					CurPlayerState,
					Health,
					MaxHealth,
					EstimatedAfterHP,
					CurPlayerMaxHP
				);
			}
		}
		else if ( AttackDamage ==  AttackStruct.SuctionAttackDamage )
		{
			BossWormLogData.SuctionAttackDamage += AttackDamage;
			if (UCombatTimelineLogger::IsSessionActive())
			{
				float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
				FString CurPlayerState = TEXT("Idle");
				QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
				float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackDamage);
				UCombatTimelineLogger::LogHitSuccess(
					CurrentSuctionActionID,
					TEXT("Suction"),
					AttackDamage,
					GetWorld()->GetTimeSeconds(),
					CurDist,
					CurPlayerState,
					Health,
					MaxHealth,
					EstimatedAfterHP,
					CurPlayerMaxHP
				);
			}
		}
			
		
		// 다시 콜리전 끄기
		if ( AttackCollisionComp ) AttackCollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABossWorm::AttackStart_AttackCollisionSphere()
{
	if ( AttackCollisionComp )
	{
		AttackCollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// 코앞에서 맞딜 중인 플레이어에 대한 초기 오버랩 감지 (적중률 0% 버그 해결)
		TArray<AActor*> OverlappingActors;
		AttackCollisionComp->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());
		for (AActor* OverlappedActor : OverlappingActors)
		{
			if (OverlappedActor && OverlappedActor != this && OverlappedActor->ActorHasTag(FName("Player")))
			{
				FHitResult DummyHit;
				OnBeginOverlapAttackCollisionSphere(AttackCollisionComp, OverlappedActor, nullptr, 0, false, DummyHit);
				break;
			}
		}
	}

	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		if (AttackDamage == AttackStruct.LungeAttackDamage && !CurrentLungeAttackActionID.IsEmpty())
		{
			UCombatTimelineLogger::LogSkillRelease(
				CurrentLungeAttackActionID,
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
		else if (!CurrentNormalAttackActionID.IsEmpty())
		{
			UCombatTimelineLogger::LogSkillRelease(
				CurrentNormalAttackActionID,
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}
}

void ABossWorm::AttackEnd_AttackCollisionSphere()
{
	if ( AttackCollisionComp ) AttackCollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABossWorm::FinishBurrow()
{
	// 캡슐 콜리전 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bIsBurrowing = true; // 땅 아래로 들어가는 몽타주가 끝나는 시점에 땅파는 상태를 true로 변경

	if (UCombatTimelineLogger::IsSessionActive() && !CurrentBurrowActionID.IsEmpty())
	{
		float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
		UCombatTimelineLogger::LogSkillRelease(
			CurrentBurrowActionID,
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
		CurrentBurrowActionID.Empty();
	}
	
	// 딜레이 설정
	if ( BlackboardComp )
	{
		BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.BurrowDelay); // 행동 딜레이 설정
	}
	
	// 타겟 위치 세팅하는 함수 == BurrowDelay 에서 2초 정도 남았을 때 호출되어 타겟 위치 세팅
	float TargetSetDelay = AttackStruct.BurrowDelay - 0.1f;
	
	// 만약 BurrowDelay가 2초보다 짧다면 즉시 호출하거나 아주 짧은 딜레이로 설정
	if (TargetSetDelay < 0.0f)
	{
		TargetSetDelay = 0.1f;
	}

	GetWorld()->GetTimerManager().SetTimer(
		UnBurrowTargetTimerHandle, 
		this, 
		&ABossWorm::SetUnBurrowTargetLocation, 
		TargetSetDelay, 
		false
	);
}

void ABossWorm::FinishUnburrow()
{
	// 캡슐 콜리전 활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	bIsBurrowing = false; // 땅에서 나오는 몽타주가 끝나는 시점에 땅파는 상태를 false로 변경
	
	// 공격 딜레이 설정
	if ( BlackboardComp )
	{
		BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.UnBurrowAttackDelay); // 행동 딜레이 설정
	}
}

void ABossWorm::NormalAttack()
{
	if ( NormalAttackMontage )
	{
		PlayAnimMontage(NormalAttackMontage);

		if ( bIsInSuctionAttackArea)
		{
			AttackDamage = AttackStruct.SuctionAttackDamage  ; // 석션 공격 범위 안에 있으면 석션 공격 대미지로 설정
			bIsInSuctionAttackArea = false; // 공격 후에는 범위 플래그 초기화
			CurrentSkillHitIntensity = AttackStruct.SuctionAttackHitIntensity;
			CurrentSkillKnockdownType = EKnockdownType::None;
			bHasExplicitSkillReaction = true;
		}
		else
		{
			AttackDamage = AttackStruct.NormalAttackDamage; // 공격 대미지 설정
			CurrentSkillHitIntensity = AttackStruct.NormalAttackHitIntensity;
			CurrentSkillKnockdownType = EKnockdownType::None;
			bHasExplicitSkillReaction = true;
		}
		
		bFocusPlayerAfterAttack = false; // 공격 중에는 포커스 비활성화
		
		if ( BlackboardComp )
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.NormalAttackDelay); // 행동 딜레이 설정
		}

		BossWormLogData.NormalAttackCount++;

		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
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
	}
}

void ABossWorm::LungeAttack()
{
	if ( LungeAttackMontage )
	{
		PlayAnimMontage(LungeAttackMontage);

		AttackDamage = AttackStruct.LungeAttackDamage; // 공격 대미지 설정
		CurrentSkillHitIntensity = AttackStruct.LungeAttackHitIntensity;
		CurrentSkillKnockdownType = EKnockdownType::None;
		bHasExplicitSkillReaction = true;
		
		bFocusPlayerAfterAttack = false;  
		
		if ( BlackboardComp )
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.LungeAttackDelay); // 행동 딜레이 설정
		}

		BossWormLogData.LungeAttackCount++;

		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

			CurrentLungeAttackActionID = UCombatTimelineLogger::LogSkillWindup(
				TEXT("LungeAttack"),
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}
}

void ABossWorm::Burrow()
{
	if ( BurrowMontage )
	{
		PlayAnimMontage(BurrowMontage);
		BlackboardComp->SetValueAsBool( TEXT("bIsBurrowing"), true);
		
		ChangePlayerLockOn(false); // 플레이어 락온 해제

		BossWormLogData.BurrowCount++;

		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

			CurrentBurrowActionID = UCombatTimelineLogger::LogSkillWindup(
				TEXT("Burrow"),
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}
}

void ABossWorm::SetUnBurrowTargetLocation()
{
	if (TargetCharacter)
	{
		// 1. 타겟의 현재 위치 (캡슐 중심)
		UnBurrowTargetLocation = TargetCharacter->GetActorLocation();

		// 타겟의 캡슐 컴포넌트 가져오기 (타겟 발바닥 위치 계산용)
		UCapsuleComponent* TargetCapsule = TargetCharacter->GetCapsuleComponent();
		float TargetHalfHeight = 0.f;
		if (TargetCapsule)
		{
			TargetHalfHeight = TargetCapsule->GetScaledCapsuleHalfHeight();
		}

		// 내(보스) 캡슐 컴포넌트 가져오기 (내 중심 위치 보정용)
		UCapsuleComponent* MyCapsule = GetCapsuleComponent();
		float MyHalfHeight = 0.f;
		if (MyCapsule)
		{
			MyHalfHeight = MyCapsule->GetScaledCapsuleHalfHeight();
		}

		// 공식: 타겟 중심 - 타겟 키(발바닥으로 내림) + 내 키(내 중심으로 올림)
		UnBurrowTargetLocation.Z = (UnBurrowTargetLocation.Z - TargetHalfHeight) + MyHalfHeight;

		// 2. 먼저 계산된 위치로 이동
		SetActorLocation(UnBurrowTargetLocation);
		
		// 3. 회전 설정 (보스 현재 위치에서 타겟을 바라보는 회전 계산)
		// 주의: 이미 위치를 옮겼으므로 단순히 TargetCharacter를 바라보면 됨
		const FRotator LookAtRotation =
			UKismetMathLibrary::FindLookAtRotation(UnBurrowTargetLocation, TargetCharacter->GetActorLocation());

		// 수평으로만 바라보도록 Yaw 값만 사용합니다.
		const FRotator SpawnRotation = FRotator(0.f, LookAtRotation.Yaw, 0.f);

		SetActorRotation(SpawnRotation);
	}
	else
	{
		// 타겟이 없으면 제자리
		UnBurrowTargetLocation = GetActorLocation();
	}
}

void ABossWorm::Unburrow()
{
	// ⭐️ 경직 및 넉다운 완전 제거 (넉백 물리만 유지)
	CurrentSkillHitIntensity = 0.0f;
	CurrentSkillKnockdownType = EKnockdownType::None;
	CurrentSkillKnockdownDuration = 0.0f;
	bHasExplicitSkillReaction = true;

	if ( UnburrowMontage )
	{
		PlayAnimMontage(UnburrowMontage);
		BlackboardComp->SetValueAsBool( TEXT("bIsBurrowing"), false);
	}

	BossWormLogData.UnBurrowAttackCount++;

	if (UCombatTimelineLogger::IsSessionActive())
	{
		float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		CurrentUnBurrowActionID = UCombatTimelineLogger::LogSkillWindup(
			TEXT("Unburrow"),
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}
	
	// --- 여기서부터 즉시 오버랩 검사 및 밀치기 로직 ---
	if (UnBurrowAttackCollisionSphere)
	{
		TArray<AActor*> OverlappingActors;
		UnBurrowAttackCollisionSphere->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

		bool bHitPlayer = false;

		for (AActor* Actor : OverlappingActors)
		{
			if (Actor && Actor != this && Actor->ActorHasTag(FName("Player")))
			{
				ACharacter* PlayerCharacter = Cast<ACharacter>(Actor);
				if (PlayerCharacter)
				{
					bHitPlayer = true;
					const float PushForce = 1600.0f;       // 시원하게 밀려나는 수평 넉백
					const float PushUpwardForce = 350.0f;  // 지면에서 살짝 뜨는 수직 넉백

					// 밀어낼 방향 계산 (보스 -> 플레이어)
					FVector PushDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
					PushDirection.Z = 0.0f;

					// 보스와 플레이어 좌표가 거의 일치할 경우(발밑 스폰) 보스 전방으로 안전 Fallback
					if (PushDirection.SizeSquared2D() < 400.0f)
					{
						PushDirection = GetActorForwardVector();
						PushDirection.Z = 0.0f;
					}
					PushDirection.Normalize();

					// 밀어낼 속도 계산
					const FVector LaunchVelocity = PushDirection * PushForce + FVector(0.f, 0.f, PushUpwardForce);

					// 1. 대미지 적용 (넉다운 없음, 경직 없음 명시)
					CurrentSkillKnockdownType = EKnockdownType::None;
					CurrentSkillHitIntensity = 0.0f;
					CurrentSkillKnockdownDuration = 0.0f;
					bHasExplicitSkillReaction = true;
					UGameplayStatics::ApplyDamage(PlayerCharacter, AttackStruct.UnBurrowAttackDamage, GetController(),
						this, UDamageType::StaticClass());

					// 2. ⭐️ 넉백(물리 밀쳐짐) 부여: 플레이어가 시원하게 튕겨 나가며 조작은 즉시 가능
					PlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);

					BossWormLogData.UnBurrowAttackDamage += AttackStruct.UnBurrowAttackDamage;

					if (UCombatTimelineLogger::IsSessionActive())
					{
						float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
						FString CurPlayerState = TEXT("Idle");
						QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
						float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackStruct.UnBurrowAttackDamage);
						UCombatTimelineLogger::LogHitSuccess(
							CurrentUnBurrowActionID,
							TEXT("Unburrow"),
							AttackStruct.UnBurrowAttackDamage,
							GetWorld()->GetTimeSeconds(),
							CurDist,
							CurPlayerState,
							Health,
							MaxHealth,
							EstimatedAfterHP,
							CurPlayerMaxHP
						);
					}

					// 로그 출력 (필요시 주석 해제)
					UE_LOG(LogTemp, Warning, TEXT("Player Hit At Unburrow Attack!"));
				}
			}
		}

		if (!bHitPlayer && UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
			UCombatTimelineLogger::LogHitMissed(
				CurrentUnBurrowActionID,
				TEXT("Unburrow"),
				GetWorld()->GetTimeSeconds(),
				CurDist,
				TEXT("Player out of unburrow blast range")
			);
		}
	}
}

void ABossWorm::RangedAttack()
{
	if ( RangedAttackMontage )
	{
		PlayAnimMontage( RangedAttackMontage);
		
		bFocusPlayerAfterAttack = false; // 공격 중에는 포커스 비활성화
		
		if ( BlackboardComp )
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.RangedAttackDelay); // 행동 딜레이 설정
		}

		float CurDist = 0.0f;
		float CurPlayerHP = 0.0f;
		float CurPlayerMaxHP = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

		CurrentRangedAttackActionID = UCombatTimelineLogger::LogSkillWindup(
			TEXT("RangedAttack"),
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}
}

void ABossWorm::ShootRangedProjectile()
{
    if (RangedProjectileClass && RangedAttackPoint && TargetCharacter)
    {
        const FVector SpawnLocation = RangedAttackPoint->GetComponentLocation();

        // 타겟 위치 보정 (발 밑)
        FVector TargetLocation = TargetCharacter->GetActorLocation();
        const UCapsuleComponent* TargetCapsule = TargetCharacter->GetCapsuleComponent();
        if (TargetCapsule)
        {
            TargetLocation.Z -= TargetCapsule->GetScaledCapsuleHalfHeight();
        }

        FVector LaunchVelocity;

        // [수정] 생성자는 4개의 인자만(World, Start, End, Speed) 받습니다.
        UGameplayStatics::FSuggestProjectileVelocityParameters Params(GetWorld(), 
        	SpawnLocation, TargetLocation, 2000.0f);

        // 나머지 옵션은 멤버 변수로 직접 설정
        Params.CollisionRadius = 10.0f;
        Params.TraceOption = ESuggestProjVelocityTraceOption::DoNotTrace;
        Params.bFavorHighArc = false; // 여기서 곡사/직사 여부 설정

        // 함수 실행 (Reference로 LaunchVelocity를 받아옴)
        bool bHaveSolution = UGameplayStatics::SuggestProjectileVelocity(Params, LaunchVelocity);

        // 해결책을 못 찾았을 경우
        if (!bHaveSolution)
        {
            FVector Direction = (TargetLocation - SpawnLocation);
            float Distance = Direction.Size();
            float FallbackSpeed = FMath::Clamp(Distance * 1.5f, 1000.0f, 3000.0f);

            // [수정] 동일하게 Fallback 파라미터도 생성자 인자 개수 맞춤
            UGameplayStatics::FSuggestProjectileVelocityParameters FallbackParams(GetWorld(), 
            	SpawnLocation, TargetLocation, FallbackSpeed);
            FallbackParams.CollisionRadius = 10.0f;
            FallbackParams.TraceOption = ESuggestProjVelocityTraceOption::DoNotTrace;
            FallbackParams.bFavorHighArc = true; // 멀면 고각 발사 시도

            if(!UGameplayStatics::SuggestProjectileVelocity(FallbackParams, LaunchVelocity))
            {
                 // 그래도 실패하면 그냥 타겟 방향으로 쏘기
                 Direction.Normalize();
                 Direction.Z += 0.5f;
                 LaunchVelocity = Direction.GetSafeNormal() * FallbackSpeed;
            }
        }

        // 회전값은 속도 벡터의 방향으로 설정
        const FRotator SpawnRotation = LaunchVelocity.Rotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = this;

        UWorld* World = GetWorld();
        if (World)
        {
            ABaseEnemyProjectile* SpawnedProjectile =
                World->SpawnActor<ABaseEnemyProjectile>(RangedProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (SpawnedProjectile)
            {
                SpawnedProjectile->SetDamage(AttackStruct.RangedAttackDamage);
                SpawnedProjectile->ActionID = CurrentRangedAttackActionID;
                SpawnedProjectile->ActionName = TEXT("RangedAttack");

                UProjectileMovementComponent* ProjMoveComp =
                    SpawnedProjectile->FindComponentByClass<UProjectileMovementComponent>();
                if (ProjMoveComp)
                {
                    ProjMoveComp->Velocity = LaunchVelocity;
                    ProjMoveComp->InitialSpeed = LaunchVelocity.Size();
                    ProjMoveComp->MaxSpeed = LaunchVelocity.Size();
                }

                float CurDist = 0.0f;
                float CurPlayerHP = 0.0f;
                float CurPlayerMaxHP = 0.0f;
                FString CurPlayerState = TEXT("Idle");
                QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);

                UCombatTimelineLogger::LogSkillRelease(
                    CurrentRangedAttackActionID,
                    GetWorld()->GetTimeSeconds(),
                    CurDist,
                    CurPlayerState,
                    Health,
                    MaxHealth,
                    CurPlayerHP,
                    CurPlayerMaxHP,
                    TEXT("RangedProjectile_Fired")
                );
            }
        }
    }
}

void ABossWorm::LinearFireBreathStart()
{
	if ( LinearFireBreathMontage )
	{
		PlayAnimMontage(LinearFireBreathMontage);
		
		bFocusPlayerAfterAttack = false; // 공격 중에는 포커스 비활성화
		
		if ( BlackboardComp )
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.LinearFireBreathDelay); // 행동 딜레이 설정
		}

		BossWormLogData.FireBreathCount++;

		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
			CurrentFireBreathActionID = UCombatTimelineLogger::LogSkillWindup(
				TEXT("FireBreath_Linear"),
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}
}

void ABossWorm::FanFireBreathStart()
{
	CurrentSkillHitIntensity = AttackStruct.FanFireBreathHitIntensity;
	CurrentSkillKnockdownType = EKnockdownType::None;
	bHasExplicitSkillReaction = true;

	if ( FanFireBreathMontage )
	{
		PlayAnimMontage(FanFireBreathMontage);
		
		bFocusPlayerAfterAttack = false; // 공격 중에는 포커스 비활성화
		
		if ( BlackboardComp )
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.FanFireBreathDelay); // 행동 딜레이 설정
		}

		BossWormLogData.FireBreathCount++;

		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
			CurrentFireBreathActionID = UCombatTimelineLogger::LogSkillWindup(
				TEXT("FireBreath_Fan"),
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}
}

void ABossWorm::StartFireBreath()
{
	CurrentSkillHitIntensity = AttackStruct.LinearFireBreathHitIntensity;
	CurrentSkillKnockdownType = EKnockdownType::None;
	bHasExplicitSkillReaction = true;

	// 화염 방사 틱 대미지 판정 타이머 시작
	if (!GetWorldTimerManager().IsTimerActive(FireBreathDamageTimerHandle))
	{
		GetWorldTimerManager().SetTimer(FireBreathDamageTimerHandle, this,
			&ABossWorm::HandleFireBreathDamageTick, FireBreathDamageInterval, true, 0.0f);
	}

	if (UCombatTimelineLogger::IsSessionActive() && !CurrentFireBreathActionID.IsEmpty())
	{
		float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
		FString CurPlayerState = TEXT("Idle");
		QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
		UCombatTimelineLogger::LogSkillRelease(
			CurrentFireBreathActionID,
			GetWorld()->GetTimeSeconds(),
			CurDist,
			CurPlayerState,
			Health,
			MaxHealth,
			CurPlayerHP,
			CurPlayerMaxHP
		);
	}
}

void ABossWorm::EndFireBreath()
{
	// 화염 방사 틱 대미지 판정 타이머 정지
	GetWorldTimerManager().ClearTimer(FireBreathDamageTimerHandle);
}

void ABossWorm::HandleFireBreathDamageTick()
{
	FVector StartLocation = FVector::ZeroVector;

	if (GetMesh() && GetMesh()->DoesSocketExist(FireBreathSocketName))
	{
		StartLocation = GetMesh()->GetSocketLocation(FireBreathSocketName);
	}
	else if (FireBreathPoint)
	{
		StartLocation = FireBreathPoint->GetComponentLocation();
	}
	else
	{
		StartLocation = GetActorLocation();
	}

	// 발사 방향: 입(TongueMaster) 위치에서 지렁이 액터 전방 기준 상하 각도(Pitch)를 적용하여 발사
	const FRotator AimRotation = GetActorRotation() + FRotator(FireBreathPitchAngle, 0.0f, 0.0f);
	const FVector AimDirection = AimRotation.Vector();
	const FVector EndLocation = StartLocation + (AimDirection * FireBreathRange);

	// 스피어 트레이스로 전방 범위 플레이어 검출 및 디버그 시각화
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	// 자체설정 탭의 bDebugMode 또는 bFireBreathDebug가 켜져 있으면 디버그 트레이스 렌더링
	const bool bShowDebug = bDebugMode || bFireBreathDebug;
	const EDrawDebugTrace::Type DebugTraceType = bShowDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	// ⭐️ Pawn 오브젝트 채널 전용 쿼리 배열 구성 (바닥/환경 지형 충돌 착시 제거)
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TArray<FHitResult> HitResults;
	const bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		this,
		StartLocation,
		EndLocation,
		FireBreathDamageRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		DebugTraceType,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		FireBreathDamageInterval
	);

	if (bHit)
	{
		TSet<AActor*> DamagedActors;
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor) continue;

			// 🔍 적중한 대상의 액터명, 컴포넌트명, 보유 태그 목록 및 Player 태그 보유 여부 정밀 로깅
			FString TagsStr = TEXT("[");
			for (const FName& TagName : HitActor->Tags)
			{
				TagsStr += TagName.ToString() + TEXT(", ");
			}
			TagsStr += TEXT("]");

			const bool bHasPlayerTag = HitActor->ActorHasTag(FName("Player"));
			UE_LOG(LogTemp, Warning, TEXT("[FireBreath Hit] Actor: %s (%s), Comp: %s, Tags: %s, HasPlayerTag: %d"),
				*HitActor->GetName(),
				*HitActor->GetClass()->GetName(),
				Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("None"),
				*TagsStr,
				bHasPlayerTag ? 1 : 0
			);

			if (!DamagedActors.Contains(HitActor) && bHasPlayerTag)
			{
				DamagedActors.Add(HitActor);
				UGameplayStatics::ApplyDamage(
					HitActor,
					AttackStruct.LinearFireBreathDamage,
					GetController(),
					this,
					UDamageType::StaticClass()
				);

				BossWormLogData.FireBreathDamage += AttackStruct.LinearFireBreathDamage;

				if (UCombatTimelineLogger::IsSessionActive())
				{
					float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
					FString CurPlayerState = TEXT("Idle");
					QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
					float EstimatedAfterHP = FMath::Max(0.0f, CurPlayerHP - AttackStruct.LinearFireBreathDamage);
					UCombatTimelineLogger::LogHitSuccess(
						CurrentFireBreathActionID,
						TEXT("FireBreath"),
						AttackStruct.LinearFireBreathDamage,
						GetWorld()->GetTimeSeconds(),
						CurDist,
						CurPlayerState,
						Health,
						MaxHealth,
						EstimatedAfterHP,
						CurPlayerMaxHP
					);
				}
			}
		}
	}
}

void ABossWorm::SpawnFireBreathProjectile()
{
	if (StreamProjectileClass)
	{
		// FireBreathPoint는 머리 뼈/소켓에 부착되어 실시간 위치를 추적
		FVector SpawnLocation = FVector::ZeroVector;
		if (GetMesh() && GetMesh()->DoesSocketExist(FireBreathSocketName))
		{
			SpawnLocation = GetMesh()->GetSocketLocation(FireBreathSocketName);
		}
		else if (FireBreathPoint)
		{
			SpawnLocation = FireBreathPoint->GetComponentLocation();
		}
		else
		{
			SpawnLocation = GetActorLocation();
		}
		
		FRotator SpawnRotation;
		ACharacter* Target = TargetCharacter ? TargetCharacter.Get() : UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		if (Target)
		{
			// 타겟(플레이어 상체)을 향하는 방향 계산
			const FVector AimDirection = (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
			if (FVector::DotProduct(AimDirection, GetActorForwardVector()) > 0.0f)
			{
				SpawnRotation = AimDirection.Rotation();
			}
			else
			{
				SpawnRotation = GetActorForwardVector().Rotation();
			}
		}
		else
		{
			SpawnRotation = GetActorForwardVector().Rotation();
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;

		GetWorld()->SpawnActor<ABaseStreamProjectile>(StreamProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	}
}

void ABossWorm::HandleSuction(float DeltaTime)
{
	if ( bIsSuctioning == true && TargetCharacter )
	{
		
		// 만약 보스랑 타깃 캐릭터 사이 거리가가까워지면 흡입력 끄기
		float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetCharacter->GetActorLocation());
		if (DistanceToTarget < AttackStruct.MinLungeDistance)
		{
			bIsInSuctionAttackArea = true; // 흡입 공격 범위 안에 들어왔음을 표시
			
			AttackDamage = AttackStruct.SuctionAttackDamage; // 공격 대미지 설정
			
			BlackboardComp->SetValueAsBool("bIsInSuctionAttackArea", true); // 블랙보드에도 반영
			
			float ElapsedTime = GetWorld()->GetTimeSeconds() - SuctionStartTime;

			EndSuction();
			return;
		}
		
		// 방향 벡터 계산 (플레이어 -> 보스)
		FVector ToBoss = GetActorLocation() - TargetCharacter->GetActorLocation();
		ToBoss.Normalize();

		// [수정] 실제 속도(Velocity) 대신 플레이어가 바라보는 방향(ForwardVector)을 사용
		// Velocity는 흡입력 때문에 뒤로 가려 해도 보스 쪽으로 끌려가면 양수가 나옵니다.
		// 따라서 캐릭터가 등을 돌리고 있는지(ForwardVector)로 판단하는 게 정확합니다.
		FVector PlayerForward = TargetCharacter->GetActorForwardVector();

		// 내적(Dot Product) 계산
		// 플레이어가 보스를 보고 있으면(공격 중) 양수, 등을 돌리고 있으면(도망 중) 음수
		float Resistance = FVector::DotProduct(ToBoss, PlayerForward);

		bool bIsMovingInput = false;
		if (ACharacter* Char = Cast<ACharacter>(TargetCharacter))
		{
			if (UCharacterMovementComponent* Movement = Char->GetCharacterMovement())
			{
				// 작은 오차를 고려해 1.0f 이상이면 입력이 있는 것으로 간주
				bIsMovingInput = Movement->GetCurrentAcceleration().SizeSquared() > 1.0f;
			}
		}
	 	
		float FinalForce = AttackStruct.SuctionForce;

		if (Resistance < 0.0f && bIsMovingInput) // 플레이어가 보스를 등지고 + 도망가려고 할 때
		{
			// 저항이 음수일 때 힘을 줄여서 플레이어가 도망갈 수 있도록 허용
			FinalForce *= 0.8f;
		}
		
		//UE_LOG(LogTemp, Warning, TEXT("Resistance Value: %f, FinalForce Value: %f"), Resistance, FinalForce);
	 	
		// 매 프레임 힘의 크기를 갱신하여 적용
		SuctionRadialForceComp->ForceStrength = FinalForce;
	} 
}

void ABossWorm::SuctionStartMontagePlay()
{
	if ( SuctionMontage )
	{
		PlayAnimMontage(SuctionMontage);
		
		bFocusPlayerAfterAttack = false; // 공격 중에는 포커스 비활성화
		
		if ( BlackboardComp )
		{
			BlackboardComp->SetValueAsFloat("AttackDelay", AttackStruct.SuctionAttackDelay); // 행동 딜레이 설정
		}

		BossWormLogData.SuctionAttackCount++;

		if (UCombatTimelineLogger::IsSessionActive())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
			CurrentSuctionActionID = UCombatTimelineLogger::LogSkillWindup(
				TEXT("Suction"),
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}
}

void ABossWorm::StartSuction()
{	
	if (SuctionRadialForceComp)
	{
		SuctionRadialForceComp->Activate(); // 빨아들이는 힘 활성화
		
		bIsSuctioning = true;
		
		SuctionStartTime = GetWorld()->GetTimeSeconds(); // 시작 시간 기록
		
		if ( BlackboardComp )
		{
			BlackboardComp->SetValueAsBool("bIsInSuctionAttackArea", false); 
		}
		bIsSuctioning = true; // 빨아들이는 중임을 표시

		if (UCombatTimelineLogger::IsSessionActive() && !CurrentSuctionActionID.IsEmpty())
		{
			float CurPlayerHP = 0.0f, CurPlayerMaxHP = 0.0f, CurDist = 0.0f;
			FString CurPlayerState = TEXT("Idle");
			QueryTargetPlayerData(CurPlayerHP, CurPlayerMaxHP, CurPlayerState, CurDist);
			UCombatTimelineLogger::LogSkillRelease(
				CurrentSuctionActionID,
				GetWorld()->GetTimeSeconds(),
				CurDist,
				CurPlayerState,
				Health,
				MaxHealth,
				CurPlayerHP,
				CurPlayerMaxHP
			);
		}
	}
}

void ABossWorm::EndSuction()
{
	if (SuctionRadialForceComp)
	{
		SuctionRadialForceComp->Deactivate(); // 빨아들이는 힘 비활성화
		bIsSuctioning = false;
	}
}

#if WITH_EDITOR
void ABossWorm::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// 변경된 프로퍼티의 이름을 가져옵니다.
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	// 디버그 모드에 따라 어택, 디텍트, 체이스 범위 구체의 가시성을 설정합니다.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABossWorm, bDebugMode))
	{
		if ( bDebugMode == true )
		{
			if ( UnBurrowAttackCollisionSphere ) UnBurrowAttackCollisionSphere->SetVisibility(true);
		}
		else
		{
			if ( UnBurrowAttackCollisionSphere ) UnBurrowAttackCollisionSphere->SetVisibility(false);
		}
	}
}
#endif

void ABossWorm::InitializeStatsFromDataTable()
{
	Super::InitializeStatsFromDataTable();

	// 1. 범용 통합 전투 밸런스 테이블(DT_BossCombatBalance)에서 스킬 파라미터 주입
	if (CachedSkillBalanceMap.Num() > 0)
	{
		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("NormalAttack")))
		{
			if (Row->Weight > 0.0f) AttackStruct.NormalAttackWeight = Row->Weight;
			if (Row->ActionDelay > 0.0f) AttackStruct.NormalAttackDelay = Row->ActionDelay;
			AttackStruct.NormalAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.0f);
			if (Row->HitIntensity >= 0.0f) AttackStruct.NormalAttackHitIntensity = Row->HitIntensity;
		}

		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("LungeAttack")))
		{
			if (Row->Weight > 0.0f) AttackStruct.LungeAttackWeight = Row->Weight;
			if (Row->ActionDelay > 0.0f) AttackStruct.LungeAttackDelay = Row->ActionDelay;
			AttackStruct.LungeAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.5f);
			if (Row->HitIntensity >= 0.0f) AttackStruct.LungeAttackHitIntensity = Row->HitIntensity;
		}

		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("Suction")))
		{
			if (Row->Weight > 0.0f) AttackStruct.SuctionAttackWeight = Row->Weight;
			if (Row->ActionDelay > 0.0f) AttackStruct.SuctionAttackDelay = Row->ActionDelay;
			AttackStruct.SuctionAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.8f);
			if (Row->HitIntensity >= 0.0f) AttackStruct.SuctionAttackHitIntensity = Row->HitIntensity;
		}

		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("LinearFireBreath")))
		{
			if (Row->Weight > 0.0f) AttackStruct.LinearFireBreathWeight = Row->Weight;
			if (Row->ActionDelay > 0.0f) AttackStruct.LinearFireBreathDelay = Row->ActionDelay;
			AttackStruct.LinearFireBreathDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 0.20f);
			if (Row->HitIntensity >= 0.0f) AttackStruct.LinearFireBreathHitIntensity = Row->HitIntensity;
		}

		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("FanFireBreath")))
		{
			if (Row->Weight > 0.0f) AttackStruct.FanFireBreathWeight = Row->Weight;
			if (Row->ActionDelay > 0.0f) AttackStruct.FanFireBreathDelay = Row->ActionDelay;
			AttackStruct.FanFireBreathDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 0.16f);
			if (Row->HitIntensity >= 0.0f) AttackStruct.FanFireBreathHitIntensity = Row->HitIntensity;
		}

		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("Burrow")))
		{
			if (Row->Weight > 0.0f) AttackStruct.BurrowWeight = Row->Weight;
			if (Row->ActionDelay > 0.0f) AttackStruct.BurrowDelay = Row->ActionDelay;
		}

		const FBossCombatBalanceRow* UnBurrowRow = FindSkillBalance(TEXT("UnBurrow"));
		if (!UnBurrowRow) UnBurrowRow = FindSkillBalance(TEXT("Unburrow"));
		if (UnBurrowRow)
		{
			AttackStruct.UnBurrowWeight = UnBurrowRow->Weight;
			if (UnBurrowRow->ActionDelay > 0.0f) AttackStruct.UnBurrowAttackDelay = UnBurrowRow->ActionDelay;
			AttackStruct.UnBurrowAttackDamage = BaseAttackDamage * (UnBurrowRow->DamageMultiplier > 0.0f ? UnBurrowRow->DamageMultiplier : 1.25f);
			AttackStruct.UnBurrowAttackHitIntensity = UnBurrowRow->HitIntensity;
		}

		if (const FBossCombatBalanceRow* Row = FindSkillBalance(TEXT("RangedAttack")))
		{
			if (Row->Weight > 0.0f) AttackStruct.RangedAttackWeight = Row->Weight;
			if (Row->ActionDelay > 0.0f) AttackStruct.RangedAttackDelay = Row->ActionDelay;
			AttackStruct.RangedAttackDamage = BaseAttackDamage * (Row->DamageMultiplier > 0.0f ? Row->DamageMultiplier : 1.0f);
		}

		AttackDamage = AttackStruct.NormalAttackDamage;

		UE_LOG(LogTemp, Log, TEXT("[BossWorm] Initialized stats from UniversalBalanceTable (DT_BossCombatBalance) successfully. Normal: %.1f, Lunge: %.1f, Suction: %.1f, Fire: %.1f, Ranged: %.1f"),
			AttackStruct.NormalAttackDamage, AttackStruct.LungeAttackDamage, AttackStruct.SuctionAttackDamage, AttackStruct.LinearFireBreathDamage, AttackStruct.RangedAttackDamage);
	}
	else
	{
		// 2. Fallback: 기존 CombatStatHandle 또는 기본 스케일링 유지
		if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())
		{
			AttackStruct.NormalAttackDamage = BaseAttackDamage;
			AttackStruct.UnBurrowAttackDamage = BaseAttackDamage * 1.25f;
			AttackStruct.LungeAttackDamage = BaseAttackDamage * 1.5f;
			AttackStruct.SuctionAttackDamage = BaseAttackDamage * 1.8f;
			AttackStruct.LinearFireBreathDamage = BaseAttackDamage * 0.20f;
			AttackStruct.FanFireBreathDamage = BaseAttackDamage * 0.16f;
			AttackStruct.RangedAttackDamage = BaseAttackDamage;
			AttackDamage = BaseAttackDamage;
		}
	}

	// 3. 밸런스 정상화 안전장치: 체력 12,000 및 공격력 220 최소선 보장 (에셋 Reimport 여부 무관)
	if (MaxHealth < 12000.0f)
	{
		MaxHealth = 12000.0f;
		Health = MaxHealth;
	}
	if (BaseAttackDamage < 220.0f)
	{
		BaseAttackDamage = 220.0f;
		if (AttackStruct.NormalAttackDamage < BaseAttackDamage)
		{
			AttackStruct.NormalAttackDamage = BaseAttackDamage;
			AttackStruct.UnBurrowAttackDamage = BaseAttackDamage * 1.25f;
			AttackStruct.LungeAttackDamage = BaseAttackDamage * 1.5f;
			AttackStruct.SuctionAttackDamage = BaseAttackDamage * 1.8f;
			AttackStruct.LinearFireBreathDamage = BaseAttackDamage * 0.20f;
			AttackStruct.FanFireBreathDamage = BaseAttackDamage * 0.16f;
			AttackDamage = BaseAttackDamage;
		}
	}
}




void ABossWorm::EndBattleLog()
{
	if (ICombatLogParticipantInterface* Participant = Cast<ICombatLogParticipantInterface>(TargetCharacter))
	{
		BossWormLogData.PlayerHitStunDuration = Participant->GetParticipantHitStunDuration();
		BossWormLogData.PlayerHitStunCount = Participant->GetParticipantHitStunCount();
		BossWormLogData.PlayerKnockdownDuration = Participant->GetParticipantKnockdownDuration();
		BossWormLogData.PlayerKnockdownCount = Participant->GetParticipantKnockdownCount();
		Participant->ResetParticipantStunMetrics();
	}

	Super::EndBattleLog();
	
	BossWormLogData.Base = CommonBossLogData;
	
	UCSVLog::AddBossWormLog(BossWormLogData);
	
	CommonBossLogData = FCommonBossLogData();
	BossWormLogData = FBossWormLogData();
}
