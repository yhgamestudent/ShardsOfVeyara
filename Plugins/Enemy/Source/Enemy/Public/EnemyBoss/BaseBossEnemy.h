#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/ItemDropInterface.h"
#include "Interface/InteractionInterface.h"
#include "CSVLogType.h" 
#include "CombatStatData.h"
#include "CombatLogParticipantInterface.h"
#include "EnemyProjectile/BaseEnemyProjectile.h"
#include "BaseBossEnemy.generated.h"

/**
 * [범용 보스 전투 밸런스 데이터 구조체]
 * 모든 보스의 기본 스탯(체력, 공격력) 및 스킬 밸런스(가중치, 쿨다운, 대미지 배율, 피격 강도, 넉다운)를
 * 단일 통합 CSV(DT_BossCombatBalance.csv)에서 관리하기 위한 행 데이터입니다.
 */
USTRUCT(BlueprintType)
struct ENEMY_API FBossCombatBalanceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	FName BossID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	FName ActionName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|BaseStat")
	float MaxHealth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|BaseStat")
	float BaseAttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|Skill")
	float Weight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|Skill")
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|Skill")
	float ActionDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|Skill")
	float DamageMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|Skill")
	float HitIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|Skill")
	EKnockdownType KnockdownType = EKnockdownType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance|Skill")
	float KnockdownDuration = 0.0f;
};

USTRUCT(BlueprintType)
struct ENEMY_API FBossSkillCooldownInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	float ActionDelay = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooldown")
	float LastCastTime = -999.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooldown")
	bool bIsOnCooldown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	float BaseWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	FName BlackboardKey = NAME_None;
};

UCLASS()
class ENEMY_API ABaseBossEnemy : public ACharacter, public IItemDropInterface, public IInteractionInterface, public ICombatLogParticipantInterface
{
	GENERATED_BODY()

public:
	// ICombatLogParticipantInterface 구현
	virtual float GetParticipantHealth() const override { return Health; }
	virtual float GetParticipantMaxHealth() const override { return MaxHealth; }
	virtual FString GetParticipantCombatState() const override { return TEXT("Boss"); }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** 플레이어의 패링(저스트 가드)에 의해 공격이 쳐내졌을 때 호출되는 가상 함수 */
	virtual void OnAttackParried(AActor* ParryingActor) {}

	/** 플레이어(TargetCharacter)의 체력 및 상태 쿼리 헬퍼 */
	void QueryTargetPlayerData(float& OutPlayerHP, float& OutPlayerMaxHP, FString& OutPlayerState, float& OutDistance) const;

	/** 현재 발동 중인 스킬의 피격 연출 강도 (0.0 ~ 1.0) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|HitReaction")
	float CurrentSkillHitIntensity = 1.0f;

	/** 현재 발동 중인 스킬의 넉다운 타입 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|HitReaction")
	EKnockdownType CurrentSkillKnockdownType = EKnockdownType::None;

	/** 현재 발동 중인 스킬의 넉다운 지속 시간 (초) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|HitReaction")
	float CurrentSkillKnockdownDuration = 1.2f;

	/** 현재 발동 중인 스킬이 명시적인 피격/넉다운 설정(None 포함)을 보유하고 있는지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|HitReaction")
	bool bHasExplicitSkillReaction = false;

	/** DT_BossCombatBalance 캐시에서 스킬별 피격 강도 및 넉다운 설정을 현재 스킬 리액션으로 적용 */
	void ApplySkillBalanceReaction(FName ActionName, float DefaultHitIntensity = 1.0f, EKnockdownType DefaultKnockdownType = EKnockdownType::None, float DefaultKnockdownDuration = 0.0f);

	/** 특정 스킬 이름 또는 블랙보드 가중치 키가 현재 쿨다운 중인지 확인 */
	virtual bool IsSkillOnCooldown(FName SkillOrBlackboardKey) const { return false; }

public:
	UPROPERTY(EditAnywhere, Category="자체설정")
	FString BossLogId;
	UPROPERTY(BlueprintReadOnly)
	FCommonBossLogData CommonBossLogData;
	
protected:
	virtual void BeginPlay() override;
	
	// 아이템 드롭 테이블에 사용하는 에너미 ID
	UPROPERTY(EditAnywhere, Category="자체설정")
	FName ItemDropTableEnemyID;
	
	virtual FName GetItemDropTableEnemyID_Implementation() const override { return ItemDropTableEnemyID; }
	
	// 타겟으로 삼을 캐릭터를 저장할 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정")
	TObjectPtr<ACharacter> TargetCharacter;
	
	// 디버그 모드
	UPROPERTY(EditAnywhere, Category="자체설정")
	bool bDebugMode = false;
	
	UPROPERTY(EditAnywhere, Category="자체설정")
	bool bCheckDeadLogic = false;
	void TestDeadLogic(); // 테스트용 죽음 로직 함수
	
	// 플레이어 인식 후 AI 행동 시작 범위 스피어
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* PlayerDetectRangeSphere;
	// 플레이어 인식 후 AI 행동 중지 범위
	UPROPERTY(EditAnywhere, Category="자체설정")
	float PlayerDetectRange = 1200.f;
	// 플레이어가 감지 범위에 들어왔을 때 호출될 함수
	UFUNCTION()
	void OnPlayerDetectOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	
	// Kalivra 밸런싱 데이터테이블(DT_CombatStats) 행 바인딩 (설정 시 MaxHealth, BaseAttackDamage 자동 초기화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="자체설정|전투스탯")
	FDataTableRowHandle CombatStatHandle;

	// 기본 공격력 (데이터테이블 DT_CombatStats 연동 또는 기본값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="자체설정|전투스탯")
	float BaseAttackDamage = 100.f;

	/** 모든 보스 공통 단일 통합 전투 밸런스 데이터테이블 핸들 (DT_BossCombatBalance.csv) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|전투스탯")
	FDataTableRowHandle UniversalBalanceTableHandle;

	/** 해당 보스의 스킬별 밸런스 데이터 캐시 맵 (ActionName -> BalanceRow) */
	UPROPERTY(Transient)
	TMap<FName, FBossCombatBalanceRow> CachedSkillBalanceMap;

	/** 특정 스킬의 밸런스 데이터 조회 헬퍼 */
	const FBossCombatBalanceRow* FindSkillBalance(FName ActionName) const;

	// 데이터테이블로부터 전투 스탯을 읽어와 초기화합니다
	UFUNCTION(BlueprintCallable, Category="자체설정|전투스탯")
	virtual void InitializeStatsFromDataTable();

	// 최대 체력
	UPROPERTY(EditAnywhere, Category="자체설정")
	float MaxHealth = 100.f;
	// 현재 체력
	UPROPERTY(EditAnywhere, Category="자체설정")
	float Health = 100.f;
	// 이동 속도 = 여기서 값 바꾸면 자동으로 캐릭터 무브먼트 컴포넌트의 MaxWalkSpeed도 바뀜
	UPROPERTY(EditAnywhere, Category="자체설정")
	float MoveSpeed = 300.f;
	
	// 보스 이름
	UPROPERTY(EditDefaultsOnly, Category="자체설정")
	FText BossName;
	// 화면에 띄울 체력바 WBP
	UPROPERTY(EditDefaultsOnly, Category="자체설정")
	TSubclassOf<class UBossHealthBarWidget> BossHealthBarWidgetClass;
	class UBossHealthBarWidget* BossHealthBar; // 보스 체력 바 위젯 캐스팅해서 저장할 변수
	bool HealthBarFlag = false;
	
	// 락온용 위젯 컴포넌트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "자체설정")
	TObjectPtr<class UWidgetComponent> LockOnWidget;
	
	// 블랙보드 컴포넌트
	class UBlackboardComponent* BlackboardComp;
	// 블랙보드 컴포넌트가 설정되었는지 여부
	bool bSetBlackboard = false; 
	// 타겟 캐릭터가 설정되었는지 여부
	bool bTargetInitalize = false;
	// 틱에서 하는 초기화
	void PollInit(float DeltaTime); 

	// 죽음 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UAnimMontage* DeathMontage;
	// 죽을때 실행할 2d 사운드
	UPROPERTY(EditDefaultsOnly, Category="자체설정")
	class USoundBase* DeadSound;
	// 죽을 떄 생성할 이펙트 == 케스케이드
	UPROPERTY(EditDefaultsOnly, Category="자체설정")
	class UParticleSystem* DeathEffectCascade;
	// 죽음 이펙트 생성 앞 뒤 위치 조정용 거리 +가 앞, -가 뒤
	UPROPERTY(EditAnywhere, Category="자체설정")
	float DeathEffectForwardOffset = 0.0f;
	// 죽음 이펙트 생성 좌 우 위치 조정용 거리 +가 오른쪽, -가 왼쪽
	UPROPERTY(EditAnywhere, Category = "자체설정")
	float DeathEffectSideOffset = 0.0f; 
	// 죽음 이펙트 크기 배율
	UPROPERTY(EditAnywhere, Category="자체설정")
	float DeathEffectScale = 1.0f;
	// 죽음 후 일정 시간 뒤에 이펙트 생성 및 액터 제거를 위한 타이머 핸들
	FTimerHandle DeathTimerHandle;
	virtual void SpawnDeadEffectAndDestroy();

	// 피격용 Overlay 머티리얼 == 피격시 머터리얼 덧씌우기
	UPROPERTY(EditDefaultsOnly, Category = "자체설정")
	class UMaterialInterface* HitOverlayMaterial;
	FTimerHandle HitFlashTimerHandle;

	// 히트시 오버레이 덧씌울 함수
	void SetHitOverlay();
	// 오버레이를 다시 제거할 함수
	void ClearHitOverlay();
	
public:
	ABaseBossEnemy();
	virtual void Tick(float DeltaTime) override;
	
	// 데미지 처리 함수 재정의
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;
	
	UPROPERTY(VisibleAnywhere)
	class UMotionWarpingComponent* MotionWarpingComponent;
	
	// 모션 워핑할 타깃 캐릭터 좌표 찍기
	UFUNCTION(BlueprintCallable)
	void UpdateMotionWarpTarget();
	// 모션 워핑 타깃 - 타깃 캐릭터의 앞쪽
	UFUNCTION(BlueprintCallable)
	void UpdateMotionWarpTargetToFront();
	// 모션 워핑 타깃 - 타깃 캐릭터 바닥
	UFUNCTION(BlueprintCallable)
	void UpdateMotionWarpTargetToFloor();
	
	// 공격 지연 시간. 블랙보드 태스크에서 재지정
	float AttackDelay = 0.0f;
	
	// 죽음 처리 함수
	virtual void Die();
	// 죽음 몽타주 끝난 후 호출되는 함수 - 애님 노티파이에서 호출
	UFUNCTION(BlueprintCallable)
	virtual void AfterDieMontageEnd();
	
	// 공격 후 플레이어 주시 여부
	bool bFocusPlayerAfterAttack = true;
	// 공격 후 플레이어 주시 시작 함수
	UFUNCTION(BlueprintCallable)
	void StartFocusPlayerAfterAttack(); 
	
	// 플레이어랑 상호작용 인터페이스
	UPROPERTY()
	FOnLockOnStateChanged OnLockOnStateChanged;
	// 인터페이스 함수 오버라이드 선언
	virtual FOnLockOnStateChanged& GetLockOnStateChangedDelegate() override { return OnLockOnStateChanged; }
	// 플레이어 락온 상태 변경 함수 
	void ChangePlayerLockOn( bool bLockOn );
	
	// 로그 관리 
	float BattleStartTime = 0.f;
	bool bIsInBattle = false;
	void StartBattleLog();
	virtual void EndBattleLog();
	virtual void AddSummonDamage(float DamageAmount);
	void AttackPatternLog(FString PatternName) const;
	FString SelectedRangeName = TEXT("Long Range");
	UFUNCTION()
	void PlayerDeadLog();
	
#if WITH_EDITOR
	// 에디터에서 프로퍼티가 변경될 때 호출됩니다.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};






