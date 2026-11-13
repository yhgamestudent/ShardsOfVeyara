#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_HopMove.generated.h"

/**
 * 점프(홉) 이동 구간 노티파이 스테이트
 * - 점프 애니메이션의 '공중에 떠 있는 프레임' 구간에 배치합니다.
 * - 이 구간 동안에만 ABaseExploderEnemy(bUseHopMovement = true)가 실제로 이동합니다.
 */
UCLASS(meta = (DisplayName = "Hop Move Window"))
class ENEMY_API UAnimNotifyState_HopMove : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("Hop Move"); }
};
