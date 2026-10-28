#pragma once

#include "CoreMinimal.h"
#include "EnemyBoss/BaseBossEnemy.h"
#include "EnemyProjectile/BaseEnemyProjectile.h"
#include "BossSkeletonMage.generated.h"

USTRUCT(BlueprintType)
struct FBossSkeletonMageAttackStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float TeleportWeight = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float FireBallWeight = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float SummonWeight = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float GroundAttackWeight = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float PushTargetWeight = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float GravityAttackWeight = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float TeleportDelay = 1.5f;
	// 텔레포트 최대 거리
	UPROPERTY(EditAnywhere, Category = "자체설정")
	float MaxTeleportDist = 800.f;
	// 텔레포트 최소 거리
	UPROPERTY(EditAnywhere, Category = "자체설정")
	float MinTeleportDist = 500.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float FireBallDamage = 84.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float FireBallDelay = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float SummonEnemyDelay = 8.f;
	// 소환하는 씬 컴포넌트 기준 소환 가능한 최대 거리
	UPROPERTY(EditAnywhere, Category = "자체설정")
	float MaxSummonDist = 400.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float GroundAttackDelay = 1.5f;
	// 장판 유지 시간
	UPROPERTY(EditAnywhere, Category = "자체설정")
	float GroundAttackDuration = 3.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float PushTargetDelay = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float GravityAttackDamage = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float GravityAttackDelay = 1.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float SecondPhaseThunderAttackDamage = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	float SecondPhaseMeteorAttackDamage = 10.0f;

	// ─── [스킬별 피격 연출 강도 (0.0 = 움찔, 1.0 = 비틀거림)] ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|HitReaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireBallHitIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|HitReaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GroundAttackHitIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|HitReaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GravityAttackHitIntensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|HitReaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PushTargetHitIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|HitReaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SecondPhaseThunderHitIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|HitReaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SecondPhaseMeteorHitIntensity = 1.0f;

	// ─── [스킬별 넉다운 설정 (Kneel 무릎 꿇기 vs AirborneFloor 에어본 후 바닥 엎어짐)] ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	EKnockdownType FireBallKnockdownType = EKnockdownType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	float FireBallKnockdownDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	EKnockdownType GroundAttackKnockdownType = EKnockdownType::Kneel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	float GroundAttackKnockdownDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	EKnockdownType GravityAttackKnockdownType = EKnockdownType::AirborneFloor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	float GravityAttackKnockdownDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	EKnockdownType PushTargetKnockdownType = EKnockdownType::AirborneFloor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	float PushTargetKnockdownDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	EKnockdownType SecondPhaseThunderKnockdownType = EKnockdownType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	float SecondPhaseThunderKnockdownDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	EKnockdownType SecondPhaseMeteorKnockdownType = EKnockdownType::AirborneFloor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|Knockdown")
	float SecondPhaseMeteorKnockdownDuration = 2.0f;
};

UCLASS()
class ENEMY_API ABossSkeletonMage : public ABaseBossEnemy
{
	GENERATED_BODY()

protected:
	virtual void SpawnDeadEffectAndDestroy() override;
	
public:
	ABossSkeletonMage();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Die() override;
	virtual void InitializeStatsFromDataTable() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;
	
	virtual void EndBattleLog() override;
	virtual void AddSummonDamage(float DamageAmount) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	FBossSkeletonMageAttackStruct AttackStruct;
	
	UPROPERTY(BlueprintReadOnly)
	FBossSkeletonMageLogData BossSkeletonMageLogData;

	// ─── [스킬별 개별 쿨타임 및 공통 행동 딜레이 시스템] ───
	UPROPERTY()
	TMap<FName, FBossSkillCooldownInfo> SkillCooldownMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Cooldown")
	float CommonAttackDelay = 1.0f;

	void StartSkillCooldown(FName SkillName);
	void UpdateSkillCooldowns();

	// 타임라인 로깅용 현재 활성 ActionID 추적 변수
	FString CurrentFireBallActionID;
	FString CurrentTeleportActionID;
	FString CurrentSummonActionID;
	FString CurrentGroundAttackActionID;
	FString CurrentPushTargetActionID;
	FString CurrentGravityAttackActionID;
	FString CurrentPhase2ThunderActionID;
	FString CurrentPhase2MeteorActionID;

	/** 현재 발동 중인 스킬의 피격 연출 강도 (0.0 ~ 1.0) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|HitReaction")
	float CurrentSkillHitIntensity = 1.0f;

	/** 현재 발동 중인 스킬의 넉다운 타입 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|HitReaction")
	EKnockdownType CurrentSkillKnockdownType = EKnockdownType::None;

	/** 현재 발동 중인 스킬의 넉다운 지속 시간 (초) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|HitReaction")
	float CurrentSkillKnockdownDuration = 1.2f;
	
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* TeleportMontage;
	void PlayTeleportMontage( const FVector& Destination );
	// 애님 노티파이에서 호출할 함수
	UFUNCTION( BlueprintCallable )
	void TeleportMoveToNextPoint();
	// 텔레포트 목적지 위치 저장용 변수
	FVector TeleportDestination;
	// 텔레포트 시각 효과
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* TeleportInEffect;
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* TeleportOutEffect;
	UFUNCTION(BlueprintCallable)
	void SpawnTeleportEffectAtLocation(const FVector& Location, class UNiagaraSystem* EffectToSpawn = nullptr);
	
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* FireBallMontage;
	void PlayFireBallMontage();
	// 애님 노티파이에서 호출할 함수
	UFUNCTION( BlueprintCallable )
	void ShootFireBall();
	// 파이어볼 발사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	TSubclassOf<class ABaseEnemyProjectile> FireBallClass;
	TObjectPtr<class USceneComponent> FireBallSpawnPoint;
	
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* SummonEnemyMontage;
	// 소환 기준 씬 컴포넌트 1, 2
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* SummonPointOne;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* SummonPointTwo;
	void StartSummoning(const FVector& Location1, const FVector& Location2);
	UFUNCTION( BlueprintCallable )
	void SummonEnemy();
	// 소환할 적 클래스들의 배열. 0 미니언 1 로그 2 워리어
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	TArray<TSubclassOf<class ABaseEnemy>> SummonableEnemyClasses;

	// 소환수 미니언에게 적용할 데이터테이블 행 이름 (기본값: Boss_SkeletonMage_Minion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	FName SummonMinionStatRowName = TEXT("Boss_SkeletonMage_Minion");

	// 소환수 로그에게 적용할 데이터테이블 행 이름 (기본값: Boss_SkeletonMage_Rogue)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	FName SummonRogueStatRowName = TEXT("Boss_SkeletonMage_Rogue");

	// 소환수 워리어에게 적용할 데이터테이블 행 이름 (기본값: Boss_SkeletonMage_Warrior)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	FName SummonWarriorStatRowName = TEXT("Boss_SkeletonMage_Warrior");
	// 소환될 위치들을 저장할 배열
	TArray<FVector> SummonLocations;
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* SummonEffectFromMage;
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* SummonEffectFromEnemy;
	void SpawnSummonEffectAtLocation(const FVector& Location);
	
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* GroundAreaAttackMontage;
	void PlayGroundAreaAttackMontage();
	// 애님 노티파이에서 호출할 함수
	UFUNCTION( BlueprintCallable )
	void GroundAreaAttack();
	// 장판 공격 발사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	TSubclassOf<class AGroundAttackProjectile> GroundAttackClass;
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* GroundTargetingEffect;
	/** 스폰된 GroundTargetingEffect 컴포넌트를 추적하기 위한 포인터 */
	UPROPERTY()
	class UNiagaraComponent* GroundTargetingComponent;
	void TraceTargetCharacterForGroundAttackEffect(float DeltaTime);
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* GroundAttackEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* PushTargetMontage;
	UPROPERTY()
	TArray<TObjectPtr<AActor>> HittedActors;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정")
	bool bIsAttacking = false; 
	// 푸시 범위 원형 콜리전 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* PushAreaSphere;
	UFUNCTION(BlueprintCallable, Category = "자체설정")
	bool IsPlayerInPushArea() const;
	void PlayPushTargetMontage();
	// 애님 노티파이에서 호출할 함수
	UFUNCTION( BlueprintCallable )
	void PushTarget_Start( ){ HittedActors.Empty();bIsAttacking = true; };
	UFUNCTION( BlueprintCallable )
	void PushTarget_End( ){HittedActors.Empty();bIsAttacking = false;} ;
	UFUNCTION( BlueprintCallable )
	void CreateMagicShield();
	// 수평으로 밀어내는 힘
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	float PushForce = 1600.f;
	// 위로 띄우는 힘
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	float PushUpwardForce = 350.f; 
	// 쉴드 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* MagicShieldEffect;

	// ─── [미니언 생존 중 매직 실드 결계(밀치기 원 유지) 기믹] ───
	UPROPERTY()
	TArray<TWeakObjectPtr<class ABaseEnemy>> ActiveSummonedMinions;

	UPROPERTY()
	class UNiagaraComponent* ActiveMagicShieldComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Gimmick")
	bool bIsMagicShieldActive = false;

	float LastShieldPushTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gimmick")
	float ShieldRepelRadius = 400.0f;

	void ActivateMagicShieldBarrier();
	void DeactivateMagicShieldBarrier();
	void CheckMinionSurvivors();

	UFUNCTION()
	void OnSummonedMinionDestroyed(AActor* DestroyedActor);
	
	UFUNCTION( BlueprintCallable )
	void GravityAttack();
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* GravityAttackMontage;
	UFUNCTION(BlueprintCallable)
	void StartGravityAttack();
	UFUNCTION(BlueprintCallable)
	void EndGravityAttack();
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* GravityGroundEffect;
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* GravityImpactEffect;
	void HandleGravityAttack(float DeltaTime);
	
	float DefaultGravityScale = 1.0f;
	float DefaultAirControl = 0.05f;
	float DefaultMaxWalkSpeed = 1.0f;
	
	bool bIsGravityAttackActive = false;
	FVector GravityAttackCenter;
	float GravityRadius = 800.f;
	float GravityHalfHeight = 700.f;
	float GravityDuration = 4.8f;
	float GravityTimer = 0.0f;

	FTimerHandle GravityActivationTimerHandle;
	FTimerHandle GravityImpactEffectTimerHandle;
	FTimerHandle GravityEndTimerHandle;
	
	// 특수 패턴 수행할 체력 비율 0 ~ 1
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	float SecondPhaseHealthThreshold = 0.5f;
	// 특정 체력 이하로 내려갈 시 실행할 특수 패턴
	void StartSecondPhase();
	bool bIsSecondPhaseStarted = false;
	// 특수 공격 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* SecondPhaseMontage;
	void PlaySecondPhaseMontage();
	UFUNCTION( BlueprintCallable )
	void SecondPhaseTeleportBossAndPlayer();
	
	// 번개 공격 떨어지기 전 위치 알려주는 나이아가라 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* ThunderTargetingEffect;
	// 번개 공격 나이아가라 임펙트	
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* ThunderImpactEffect;
	// 번개 칠 시 나올 사운드 
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class USoundBase* ThunderSound;
	
	UFUNCTION( BlueprintCallable )
	void SummonThunderToPlayer();
	
	// 특수 패턴 시작시 이동시킬 위치
	UPROPERTY(EditAnywhere, Category = "자체설정")
	TObjectPtr<class ATargetPoint> BossTeleportPoint;
	UPROPERTY(EditAnywhere, Category = "자체설정")
	TObjectPtr<class ATargetPoint> PlayerTeleportPoint;
	
	
	// 메테오 공격 이펙트	
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UNiagaraSystem* MeteorEffect;
	// 메테오 충돌시 사운드
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class USoundBase* MeteorSound;
	
	UFUNCTION( BlueprintCallable )
	void StartSummonRandomMeteor();
};

