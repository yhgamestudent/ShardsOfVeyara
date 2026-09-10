// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_IgnorePawnCollision.generated.h"

/**
 * 창을 땅에 꽂고 올라타서 도약/체공하는 동안 몬스터와의 물리적 충돌(밀림)을 무시하고
 * 선택적으로 피격 무적(데미지 무시)을 적용하는 노티파이 스테이트입니다.
 */
UCLASS(meta = (DisplayName = "Ignore Pawn Collision"))
class AGSD_API UAnimNotifyState_IgnorePawnCollision : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_IgnorePawnCollision();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	// 몬스터(Pawn)와의 충돌을 무시할지 여부 (true 시 밀림 없이 통과)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bIgnorePawnCollision = true;

	// 공중 체공 중 데미지(피격) 무적 상태를 적용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bMakeInvulnerable = true;
};
