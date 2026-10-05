// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnableMovement.generated.h"

/**
 * 공격 또는 액션 몽타주의 후딜레이 시점에 배치하여,
 * 이동 입력을 조기에 허용하고 이동 시 몽타주를 캔슬할 수 있도록 지원하는 애님 노티파이
 */
UCLASS(meta = (DisplayName = "Enable Movement"))
class AGSD_API UAnimNotify_EnableMovement : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
