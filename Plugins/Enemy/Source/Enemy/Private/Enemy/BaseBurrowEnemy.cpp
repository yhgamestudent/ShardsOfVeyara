#include "Enemy/BaseBurrowEnemy.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "EnemyProjectile/BaseStreamProjectile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

ABaseBurrowEnemy::ABaseBurrowEnemy()
{
	// 최대 걷기 속도 제로로 해서 움직이지 않도록 설정
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	
	// 이동 불가 
	GetCharacterMovement()->MaxWalkSpeed = 0.0f; 
	
	HealthBarWidget->SetVisibility(false);
	
	DetectRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ChaseRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	MeleeAttackPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AttackPoint"));
	MeleeAttackPoint->SetupAttachment(RootComponent); // 루트 컴포넌트
	
	AttackRangePointSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangePointSphere"));
	AttackRangePointSphere->SetupAttachment(MeleeAttackPoint); // AttackPoint에 부착
	AttackRangePointSphere->ShapeColor = FColor::Purple;
	AttackRangePointSphere->SetVisibility(false);
	AttackRangePointSphere->SetHiddenInGame(true);
	
	BurrowAttackCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("BurrowAttackCollisionSphere"));
	BurrowAttackCollisionSphere->SetupAttachment(RootComponent);
	BurrowAttackCollisionSphere->ShapeColor = FColor::Black;
	BurrowAttackCollisionSphere->SetVisibility(false);
	BurrowAttackCollisionSphere->SetHiddenInGame(true);

	// 근접 공격 발동 범위 시각화 및 감지용 구체 컴포넌트 (노란색)
	MeleeAttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MeleeAttackRangeSphere"));
	MeleeAttackRangeSphere->SetupAttachment(RootComponent);
	MeleeAttackRangeSphere->InitSphereRadius(MeleeAttackRange);
	MeleeAttackRangeSphere->ShapeColor = FColor::Yellow;
	MeleeAttackRangeSphere->SetVisibility(false);
	MeleeAttackRangeSphere->SetHiddenInGame(true);
	MeleeAttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeleeAttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeleeAttackRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	EnemyType = EEnemyType::EET_Burrow;
	AttackDelay = 1.2f; // 근접/원거리 공격 사이 기본 대기 시간 1.2초 단축
	ItemDropTableEnemyID = FName(TEXT("GiantWorm"));
}

void ABaseBurrowEnemy::BeginPlay()
{
	Super::BeginPlay();

	// 디버그 모드 상태에 따른 범위 구체 가시성 초기 동기화
	const bool bShowDebugSphere = bDebugMode;
	if (AttackRangePointSphere)
	{
		AttackRangePointSphere->SetVisibility(bShowDebugSphere);
		AttackRangePointSphere->SetHiddenInGame(!bShowDebugSphere);
	}
	if (BurrowAttackCollisionSphere)
	{
		BurrowAttackCollisionSphere->SetVisibility(bShowDebugSphere);
		BurrowAttackCollisionSphere->SetHiddenInGame(!bShowDebugSphere);
	}
	if (MeleeAttackRangeSphere)
	{
		MeleeAttackRangeSphere->SetVisibility(bShowDebugSphere);
		MeleeAttackRangeSphere->SetHiddenInGame(!bShowDebugSphere);
	}
	
	TArray<USphereComponent*> SphereComps;
	GetComponents<USphereComponent>(SphereComps);
	
	if ( AttackRangePointSphere )
	{
		AttackRangePointSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseBurrowEnemy::OnBeginOverlapAttackCollisionSphere);
		AttackRangePointSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
		
	if ( DetectRangeSphere ) DetectRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseBurrowEnemy::OnBeginOverlapDetectRangeSphere);
	
	if ( ChaseRangeSphere ) ChaseRangeSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseBurrowEnemy::OnEndOverlapChaseRangeSphere);
	
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
	
	Tags.Add(FName("Burrow")); // 버로우 태그 추가
}

void ABaseBurrowEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//bIsRecentlyUnburrowed 계속 로그 찍기 
	UE_LOG( LogTemp, Warning, TEXT("bIsRecentlyUnburrowed : %s"), bIsRecentlyUnburrowed ? TEXT("true") : TEXT("false") );
}

float ABaseBurrowEnemy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

UAnimMontage* ABaseBurrowEnemy::Attack()
{
	if (Health <= 0.0f || bIsBurrowing)
	{
		return nullptr;
	}

	ACharacter* Target = TargetCharacter ? TargetCharacter.Get() : UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!Target)
	{
		return Super::Attack();
	}

	// 1. 노란색 원형 콜리전(MeleeAttackRangeSphere) 안에 플레이어가 들어와 있는지 직관적으로 판정
	bool bIsTargetInMeleeRange = false;
	if (MeleeAttackRangeSphere)
	{
		bIsTargetInMeleeRange = MeleeAttackRangeSphere->IsOverlappingActor(Target);
	}
	else
	{
		const float HorizontalDist = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
		bIsTargetInMeleeRange = (HorizontalDist <= MeleeAttackRange);
	}

	UAnimMontage* MontageToPlay = nullptr;

	if (bIsTargetInMeleeRange)
	{
		// 노란 원 안에 플레이어가 있으면 근접 공격 실행
		if (MeleeAttackMontage)
		{
			MontageToPlay = MeleeAttackMontage;
		}
		else if (AttackMontages.Num() > 0)
		{
			MontageToPlay = AttackMontages[0];
		}
	}
	else
	{
		// 노란 원 밖(안 맞는 위치)에 플레이어가 있으면 무조건 화염 방사 실행
		if (FireBreathMontage)
		{
			MontageToPlay = FireBreathMontage;
		}
		else if (AttackMontages.Num() > 1)
		{
			MontageToPlay = AttackMontages[1];
		}
		else if (AttackMontages.Num() > 0)
		{
			MontageToPlay = AttackMontages[0];
		}
	}

	if (MontageToPlay)
	{
		PlayAnimMontage(MontageToPlay);
		bFocusPlayerAfterAttack = false;
		return MontageToPlay;
	}

	// 전용 몽타주가 모두 비어있을 경우 부모의 기본 랜덤 공격 실행
	return Super::Attack();
}

void ABaseBurrowEnemy::OnBeginOverlapAttackCollisionSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ( bIsRecentlyUnburrowed == true ) return;
	
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack Sphere Overlapped with Player!"));

		ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
		if (PlayerCharacter)
		{
			// 밀어낼 방향 계산 (지렁이 -> 플레이어, 수평)
			FVector PushDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
			PushDirection.Z = 0.0f;
			if (PushDirection.IsNearlyZero())
			{
				PushDirection = GetActorForwardVector();
			}
			else
			{
				PushDirection.Normalize();
			}

			// 밀어낼 속도 벡터 계산 (수평 + 수직 띄우기)
			const FVector LaunchVelocity = PushDirection * MeleeAttackPushForce + FVector(0.f, 0.f, MeleeAttackUpwardForce);
			PlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}

		// 로그 기록 로직
		if (GetMesh()) 
		{
			FString MeshName = GetMesh()->GetSkeletalMeshAsset() ? GetMesh()->GetSkeletalMeshAsset()->GetName() : TEXT("NoMeshAsset");
		}
		
		const float FinalDamage = (MeleeAttackDamage > 0.0f) ? MeleeAttackDamage : AttackDamage;
		EnemyLogData.TotalDamageDealt += FinalDamage;
		
		// 대미지 적용
		UGameplayStatics::ApplyDamage(OtherActor, FinalDamage, GetController(),
			this, UDamageType::StaticClass());
		
		// 다시 콜리전 끄기
		if ( AttackRangePointSphere ) AttackRangePointSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABaseBurrowEnemy::OnBeginOverlapDetectRangeSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		if ( bIsBurrowing == true )
		{
			PlayUnburrowMontage();
		}
	}
}

void ABaseBurrowEnemy::OnEndOverlapChaseRangeSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag(FName("Player")))
	{
		if ( bIsBurrowing == false )
		{
			PlayBurrowMontage();
			
			// 타이머 정지 (발사 중단) 
			GetWorldTimerManager().ClearTimer(FireBreathTimerHandle);
		}
	}
}

void ABaseBurrowEnemy::AttackStart_AttackCollisionSphere()
{
	if ( AttackRangePointSphere ) AttackRangePointSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ABaseBurrowEnemy::AttackEnd_AttackCollisionSphere()
{
	if ( AttackRangePointSphere ) AttackRangePointSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseBurrowEnemy::PlayBurrowMontage()
{
	if ( BurrowMontage )
	{
		PlayAnimMontage(BurrowMontage);
	}
}

void ABaseBurrowEnemy::PlayUnburrowMontage()
{
	if ( UnburrowMontage )
	{
		PlayAnimMontage(UnburrowMontage);
	}
}

void ABaseBurrowEnemy::FinishUnburrow()
{
	bIsBurrowing = false;
	
	if ( HealthBarWidget ) HealthBarWidget->SetVisibility(true);
	
	// --- 1초 공격 제한 로직 추가 ---
	bIsRecentlyUnburrowed = true; // 현재 "공격 제한" 상태임
	GetWorldTimerManager().SetTimer(UnburrowDelayTimerHandle, this, &ABaseBurrowEnemy::ResetUnburrowDelay, 3.0f, false);
	
	if ( BurrowAttackCollisionSphere )
	{
		// 플레이어 액터 감지 
		TArray<AActor*> OverlappingActors;
		BurrowAttackCollisionSphere->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass()); // 캐릭터 클래스만 필터링

		for (AActor* Actor : OverlappingActors)
		{
			// 나 자신이 아니고, Player 태그가 있는지 확인
			if (Actor && Actor != this && Actor->ActorHasTag(FName("Player")))
			{
				ACharacter* PlayerCharacter = Cast<ACharacter>(Actor);
				if (PlayerCharacter)
				{
					const float PushForce = 700.0f;       // 밀어내는 힘 
					const float PushUpwardForce = 200.0f;  // 띄우는 힘 

					// 밀어낼 방향 계산 (적 -> 플레이어)
					FVector PushDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
					PushDirection.Z = 0; // 수직 방향 영향 제거
					PushDirection.Normalize();

					// 밀어낼 속도 계산
					const FVector LaunchVelocity = PushDirection * PushForce + FVector(0.f, 0.f, PushUpwardForce);

					// 플레이어 캐릭터를 밀쳐냄 (XY, Z 모두 강제 적용 override)
					PlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);
					
					// 로그 기록 로직
					if (GetMesh()) 
					{
						// 스켈레탈 메쉬 에셋 이름 가져오기
						FString MeshName = GetMesh()->GetSkeletalMeshAsset() ? GetMesh()->GetSkeletalMeshAsset()->GetName() : TEXT("NoMeshAsset");
					}
					
					EnemyLogData.TotalDamageDealt += UnburrowAttackDamage; // 로그 데이터에 입힌 대미지 누적
					
					// 대미지 적용 
					UGameplayStatics::ApplyDamage(PlayerCharacter, UnburrowAttackDamage, GetController(),
						this, UDamageType::StaticClass());
					
					// 로그 출력
					UE_LOG(LogTemp, Warning, TEXT("[BaseBurrowEnemy] Unburrow Attack Hit Player!"));
				}
			}
		}
	}
}

void ABaseBurrowEnemy::FinishBurrow()
{
	bIsBurrowing = true;
	
	if ( HealthBarWidget ) HealthBarWidget->SetVisibility(false);
}

void ABaseBurrowEnemy::StartFireBreath()
{
	// 화염 방사 틱 대미지 판정 타이머 시작
	if (!GetWorldTimerManager().IsTimerActive(FireBreathDamageTimerHandle))
	{
		GetWorldTimerManager().SetTimer(FireBreathDamageTimerHandle, this,
			&ABaseBurrowEnemy::HandleFireBreathDamageTick, FireBreathDamageInterval, true, 0.0f);
	}
}

void ABaseBurrowEnemy::EndFireBreath()
{
	// 화염 방사 틱 대미지 판정 타이머 정지
	GetWorldTimerManager().ClearTimer(FireBreathDamageTimerHandle);
}

void ABaseBurrowEnemy::HandleFireBreathDamageTick()
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

	TArray<FHitResult> HitResults;
	const bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		this,
		StartLocation,
		EndLocation,
		FireBreathDamageRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
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
			if (HitActor && !DamagedActors.Contains(HitActor) && HitActor->ActorHasTag(FName("Player")))
			{
				DamagedActors.Add(HitActor);
				UGameplayStatics::ApplyDamage(
					HitActor,
					FireBreathDamage,
					GetController(),
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}

void ABaseBurrowEnemy::SpawnFireBreathProjectile()
{
	if (FireBreathPoint && StreamProjectileClass)
	{
		// FireBreathPoint는 머리 뼈에 붙어 있으므로, 애니메이션에 따라 위치와 회전이 실시간으로 변함
		const FVector SpawnLocation = FireBreathPoint->GetComponentLocation();
		
		// 소켓의 회전을 가져오되, Pitch(위아래)와 Roll(좌우기울기)은 0으로 만들어 수평으로 발사
		FRotator SpawnRotation = FireBreathPoint->GetComponentRotation();
		SpawnRotation.Pitch = -30.0f; // 바닥으로 박히지 않게 수평 유지
		SpawnRotation.Roll = 0.0f;  // 기울어지지 않게

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;

		GetWorld()->SpawnActor<ABaseStreamProjectile>(StreamProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	}
}

#if WITH_EDITOR
void ABaseBurrowEnemy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// 변경된 프로퍼티의 이름을 가져옵니다.
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// MeleeAttackRange 프로퍼티가 변경되면 노란색 원형 콜리전의 반지름도 에디터에서 즉시 동기화
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseBurrowEnemy, MeleeAttackRange))
	{
		if (MeleeAttackRangeSphere)
		{
			MeleeAttackRangeSphere->SetSphereRadius(MeleeAttackRange);
		}
	}
	
	// 디버그 모드에 따라 범위 구체의 가시성을 설정합니다.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseBurrowEnemy, bDebugMode))
	{
		const bool bShow = bDebugMode;
		if (AttackRangePointSphere)
		{
			AttackRangePointSphere->SetVisibility(bShow);
			AttackRangePointSphere->SetHiddenInGame(!bShow);
		}
		if (BurrowAttackCollisionSphere)
		{
			BurrowAttackCollisionSphere->SetVisibility(bShow);
			BurrowAttackCollisionSphere->SetHiddenInGame(!bShow);
		}
		if (MeleeAttackRangeSphere)
		{
			MeleeAttackRangeSphere->SetVisibility(bShow);
			MeleeAttackRangeSphere->SetHiddenInGame(!bShow);
		}
	}
}
#endif

void ABaseBurrowEnemy::InitializeStatsFromDataTable()
{
	Super::InitializeStatsFromDataTable();

	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())
	{
		UnburrowAttackDamage = AttackDamage;
	}
}
