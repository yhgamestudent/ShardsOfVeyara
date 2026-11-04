#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "BaseBurrowEnemy.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;


UCLASS()
class ENEMY_API ABaseBurrowEnemy : public ABaseEnemy
{
	GENERATED_BODY()
	
protected:
	
	// 튀어나올시 공격
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* BurrowAttackCollisionSphere;
	// 근접 공격 지점 컴포넌트
	UPROPERTY(VisibleAnywhere)
	class USceneComponent* MeleeAttackPoint;
	// 근접 공격 범위
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* AttackRangePointSphere;

	// 근접 공격 발동 범위 시각화 및 감지용 원형 구체 컴포넌트 (노란색)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정|컴포넌트")
	class USphereComponent* MeleeAttackRangeSphere;
	
	class USceneComponent* FireBreathPoint = nullptr;

public:
	ABaseBurrowEnemy();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, 
		struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void InitializeStatsFromDataTable() override;
	
	UFUNCTION()
	void OnBeginOverlapAttackCollisionSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable)
	void AttackStart_AttackCollisionSphere();
	UFUNCTION(BlueprintCallable)
	void AttackEnd_AttackCollisionSphere();

	// 거리 기반 공격 분기 함수 (부모 가상 함수 오버라이드)
	virtual UAnimMontage* Attack() override;

	// 근접 공격 전용 몽타주 (예: AM_GiantWorm_Attack)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "자체설정|근접공격")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	// 화염 방사 전용 몽타주 (예: AM_GiantWorm_LinearFireBreath)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "자체설정|화염방사")
	TObjectPtr<UAnimMontage> FireBreathMontage;

	// 근접 공격으로 전환되는 기준 거리 (실제 공격 콜리전이 닿는 유효 사거리, 기본값: 220 유닛)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|근접공격")
	float MeleeAttackRange = 220.0f;

	// 근접 공격 밀쳐내는 수평 힘 (기본값: 800.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|근접공격")
	float MeleeAttackPushForce = 800.0f;

	// 근접 공격 띄우는 수직 힘 (기본값: 250.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|근접공격")
	float MeleeAttackUpwardForce = 250.0f;

	// 근접 공격 대미지 (기본값: 15.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|근접공격")
	float MeleeAttackDamage = 15.0f;
	
	// 땅파는 중인지 여부를 나타내는 변수 + 블루프린트 에서 읽기 전용으로 설정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "자체설정")
	bool bIsBurrowing = true; 

	// 땅 아래로 들어가는 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "자체설정")
	UAnimMontage* BurrowMontage; 
	// 땅에서 나오는 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "자체설정")
	UAnimMontage* UnburrowMontage; 
	// 땅 아래로 들어가는 몽타주 재생 함수
	void PlayBurrowMontage();
	// 땅에서 나오는 몽타주 재생 함수
	void PlayUnburrowMontage();
	// 애님 노티파이 함수 - 땅에서 나오는 몽타주 끝나는 시점에 호출되어 땅파는 상태를 false로 변경
	UFUNCTION(BlueprintCallable)
	void FinishUnburrow();
	// 애님 노티파이 함수 - 땅 아래로 들어가는 몽타주 끝나는 시점에 호출되어 땅파는 상태를 true로 변경
	UFUNCTION(BlueprintCallable)
	void FinishBurrow();

	// 탐지 범위 스피어 비긴 오버랩 함수
	UFUNCTION()
	void OnBeginOverlapDetectRangeSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	// 탐지 범위 스피어 엔드 오버랩 함수
	UFUNCTION()
	void OnEndOverlapChaseRangeSphere(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	// 화염 방사 시작 함수
	UFUNCTION(BlueprintCallable)
	void StartFireBreath();
	// 화염 방사 종료 함수
	UFUNCTION(BlueprintCallable)	
	void EndFireBreath();
	// 화염 방사 발사체
	UPROPERTY(EditDefaultsOnly, Category="자체설정")
	TSubclassOf<class ABaseStreamProjectile> StreamProjectileClass;
	// 연사 타이머 핸들
	FTimerHandle FireBreathTimerHandle; 
	// 화염 방사 발사 본/소켓 이름 (기본값: TongueMaster)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|화염방사")
	FName FireBreathSocketName = TEXT("TongueMaster");

	// 화염 방사 상하 발사 각도 (단위: 도, 음수면 아래로 조준, 기본값: -10.0도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|화염방사", meta = (ClampMin = "-45.0", ClampMax = "45.0"))
	float FireBreathPitchAngle = -10.0f;

	void SpawnFireBreathProjectile();

	// 화염 방사 틱당 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|화염방사")
	float FireBreathDamage = 2.0f;

	// 화염 방사 판정 반경 (스피어 트레이스 반지름)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|화염방사")
	float FireBreathDamageRadius = 100.0f;

	// 화염 방사 유효 사정거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|화염방사")
	float FireBreathRange = 1200.0f;

	// 화염 방사 틱 데미지 주기 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정|화염방사")
	float FireBreathDamageInterval = 0.15f;

	FTimerHandle FireBreathDamageTimerHandle;
	void HandleFireBreathDamageTick();

	// 화염 방사 디버그 트레이스 시각화 여부 (자체설정 탭)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	bool bFireBreathDebug = false;
	// 화염 방사 간격
	UPROPERTY(EditAnywhere, Category = "자체설정")	
	float FireBreathInterval = 0.05f;
	// 직선 화염 방사 어택 딜레이
	UPROPERTY(EditAnywhere, Category = "자체설정")
	float LinearFireBreathAttackDelay = 7.0f;
	// 원형 화염 방사 어택 딜레이 
	UPROPERTY(EditAnywhere, Category = "자체설정")
	float FanFireBreathAttackDelay = 10.0f;
	
	// 공격 대미지 (언버로우)
	UPROPERTY(EditAnywhere, Category="자체설정")
	float UnburrowAttackDamage = 10.f;
	
	bool bIsRecentlyUnburrowed = false; // 언버로우 직후인지 체크하는 변수
	FTimerHandle UnburrowDelayTimerHandle; // 1초 뒤 변수를 원복할 타이머
	// 1초 뒤에 호출되어 변수를 false로 만들 함수
	void ResetUnburrowDelay() { bIsRecentlyUnburrowed = false; }

#if WITH_EDITOR
	// 에디터에서 프로퍼티가 변경될 때 호출됩니다.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
