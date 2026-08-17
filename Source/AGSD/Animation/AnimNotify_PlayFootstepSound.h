#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Chaos/ChaosEngineInterface.h" // For EPhysicalSurface in UE5
#include "AnimNotify_PlayFootstepSound.generated.h"

class USoundBase;

/**
 * 발걸음에 맞춰 트레이스를 통해 바닥 재질을 감지하고,
 * 그에 맞는 발소리를 재생하는 애님 노티파이입니다.
 */
UCLASS(meta = (DisplayName = "Play Footstep Sound"))
class AGSD_API UAnimNotify_PlayFootstepSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_PlayFootstepSound();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	// 기본 발소리 (바닥 트레이스가 실패하거나 맵에 없는 재질일 때 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (DisplayName = "Default Sound"))
	USoundBase* DefaultSound;

	// 물리 매테리얼(Surface Type)에 따른 발소리 맵
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (DisplayName = "Sound by Surface Type"))
	TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> SurfaceSoundMap;

	// 소리 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	float VolumeMultiplier;

	// 소리 높낮이 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	float PitchMultiplier;

	// 트레이스를 시작할 본/소켓 이름 (예: foot_l, foot_r)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FName AttachName;

	// 바닥을 감지하기 위해 아래로 쏘는 트레이스 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	float TraceDistance;
};
