#include "AnimNotify_PlayFootstepSound.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"

UAnimNotify_PlayFootstepSound::UAnimNotify_PlayFootstepSound()
{
	VolumeMultiplier = 1.0f;
	PitchMultiplier = 1.0f;
	AttachName = NAME_None;
	TraceDistance = 50.0f;
}

void UAnimNotify_PlayFootstepSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	// 본(소켓)의 현재 위치를 가져옵니다.
	FVector TraceStart = MeshComp->GetSocketLocation(AttachName);
	// 트레이스 종료 지점: 본 위치에서 아래(Z축) 방향으로 TraceDistance만큼 이동
	FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true; // 바닥 재질 정보를 가져오기 위해 필수
	QueryParams.AddIgnoredActor(MeshComp->GetOwner()); // 자기 자신은 트레이스에서 무시

	USoundBase* SoundToPlay = DefaultSound;

	// 바닥 방향으로 Line Trace 실행
	bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	if (bHit && HitResult.PhysMaterial.IsValid())
	{
		EPhysicalSurface SurfaceType = HitResult.PhysMaterial->SurfaceType;
		
		// 맵에 해당 표면 재질에 설정된 사운드가 있는지 확인
		if (USoundBase** FoundSound = SurfaceSoundMap.Find(SurfaceType))
		{
			if (*FoundSound)
			{
				SoundToPlay = *FoundSound;
			}
		}
	}

	// 사운드가 지정되어 있으면 재생
	if (SoundToPlay)
	{
		// 소리를 트레이스 시작점(발 위치)에서 재생합니다.
		UGameplayStatics::PlaySoundAtLocation(World, SoundToPlay, TraceStart, VolumeMultiplier, PitchMultiplier);
	}
}
