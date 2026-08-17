#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ECharacterState.h"
#include "HoldingWeapon.h"
#include "AGSDGuardComponent.generated.h"

class AAGSDCharacter;
class UNiagaraSystem;
class UParticleSystem;
class USoundBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AGSD_API UAGSDGuardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGSDGuardComponent();

protected:
	virtual void BeginPlay() override;

public:
	// 가드 시작 및 종료 (입력 액션 바인딩용)
	UFUNCTION(BlueprintCallable, Category = "Guard")
	void StartBlock();

	UFUNCTION(BlueprintCallable, Category = "Guard")
	void StopBlock();

	// 가드/저스트가드 상태 조회
	UFUNCTION(BlueprintCallable, Category = "Guard")
	bool IsGuardActive() const;

	UFUNCTION(BlueprintCallable, Category = "Guard")
	bool IsGuardKeyPressed() const { return bGuardPressed; }

	UFUNCTION(BlueprintCallable, Category = "Guard")
	bool IsJustGuardActive() const { return bIsJustGuardWindow; }

	UFUNCTION(BlueprintCallable, Category = "Guard")
	bool IsJustGuardSuccessful() const { return bJustGuardSuccessful; }

	UFUNCTION(BlueprintCallable, Category = "Guard")
	void ResetJustGuardSuccess() { bJustGuardSuccessful = false; }

	// 저스트 가드 성공 연출 및 상태 리셋
	UFUNCTION(BlueprintCallable, Category = "Guard")
	void HandleJustGuardSuccess();

	// 수동 타이머 정리 및 판정 리셋 (예: 콤보 리셋 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Guard")
	void ResetGuardState();

	// 대미지 감쇄 프로세스 (TakeDamage 위임용)
	// 가드/저스트가드로 대미지가 감쇄된 경우 true 반환 및 OutMitigatedDamage 갱신
	bool ProcessDamageMitigation(float IncomingDamage, float& OutMitigatedDamage);

public:
	// 저스트 가드 활성화 여부 동적 설정 (노티파이 연동용)
	UFUNCTION(BlueprintCallable, Category = "Guard")
	void SetJustGuardWindowActive(bool bActive);

private:
	// 저스트 가드 활성화 여부
	bool bIsJustGuardWindow = false;

	// 저스트 가드 성공 여부 (카운터 공격 입력 대기용)
	bool bJustGuardSuccessful = false;

	// 가드 입력 버튼 누름/홀드 상태
	bool bGuardPressed = false;



	// ── 설정값 (캐릭터에서 마이그레이션) ──

	// 일반 가드 시 스폰할 나이아가라 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Effects", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> GuardEffect;

	// 저스트 가드 성공 시 스폰할 캐스케이드 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Effects", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UParticleSystem> JustGuardParticle;

	// 저스트 가드 성공 시 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Effects", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> JustGuardSound;



	// 가드 가능한 무기 종류 설정 (기본값: Spear)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Config", meta = (AllowPrivateAccess = "true"))
	TArray<EHoldingWeapon> BlockableWeapons;

	// 소유주 캐릭터 캐싱
	UPROPERTY()
	TObjectPtr<AAGSDCharacter> OwnerCharacter;
};
