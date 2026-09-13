// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_RollCancel.generated.h"

/**
 * 몽타주 재생 중 구르기(스페이스바) 입력 시 공격/액션을 즉시 취소하고 구르기를 실행할 수 있는 구간을 정의하는 노티파이 스테이트
 */
UCLASS()
class AGSD_API UAnimNotifyState_RollCancel : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
