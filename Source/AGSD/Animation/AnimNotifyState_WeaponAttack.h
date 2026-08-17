// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
class AAGSDCharacter;
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AnimNotifyState_WeaponAttack.generated.h"

UCLASS()
class AGSD_API UAnimNotifyState_WeaponAttack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	// 무기의 충돌 판정 구체 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Collision")
	float TraceRadius = 20.0f;

	// 무기에 붙어있는 타격용 시작 소켓 이름 (손잡이 쪽)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Collision")
	FName StartSocketName = FName("Weapon_Attack_Back");

	// 무기에 붙어있는 타격용 끝 소켓 이름 (창날 끝 쪽)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Collision")
	FName EndSocketName = FName("Weapon_Attack");

	// 트레이스할 충돌 채널 (예: ETraceTypeQuery::TraceTypeQuery3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Collision")
	TEnumAsByte<ETraceTypeQuery> WeaponTraceChannel;

	// 타격 대상 필터링 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Collision")
	FName TargetTag = FName("Enemy");

	// 타격 시 재생할 나이아가라 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|VFX")
	class UNiagaraSystem* HitEffect;

	// 타격 이펙트의 크기 (스케일)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|VFX")
	FVector HitEffectScale = FVector(0.5f, 0.5f, 0.5f);

	// 공격(휘두르기) 시 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Audio")
	class USoundBase* AttackSound;

	// 적 타격 시 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Audio")
	class USoundBase* HitSound;

	UPROPERTY(VisibleAnywhere, Blueprintreadwrite, Category = "Combat|Collision")
	AAGSDCharacter* Player;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Collision")
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Collision")
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugTrace = EDrawDebugTrace::None;
	
private:
	// AnimNotifyState는 에셋 인스턴스이므로 여러 액터가 동시에 사용할 때 데이터가 꼬이지 않도록 MeshComp를 키값으로 맵핑하여 관리합니다.
	TMap<USkeletalMeshComponent*, TArray<AActor*>> HitActorsMap;
	TMap<USkeletalMeshComponent*, FVector> PreviousLocationMap;
};
