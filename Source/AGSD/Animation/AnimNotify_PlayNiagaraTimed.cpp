// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify_PlayNiagaraTimed.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

UAnimNotify_PlayNiagaraTimed::UAnimNotify_PlayNiagaraTimed()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 128, 0, 255);
#endif
}

void UAnimNotify_PlayNiagaraTimed::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !Template) return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	UNiagaraComponent* NiagaraComp = nullptr;
	if (bAttached)
	{
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Template,
			MeshComp,
			SocketName,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			true // bAutoDestroy = true (잔여 파티클 소멸 후 자동 삭제)
		);
	}
	else
	{
		const FTransform SocketTransform = MeshComp->GetSocketTransform(SocketName);
		const FVector SpawnLoc = SocketTransform.TransformPosition(LocationOffset);
		const FRotator SpawnRot = SocketTransform.TransformRotation(RotationOffset.Quaternion()).Rotator();
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			Template,
			SpawnLoc,
			SpawnRot,
			Scale,
			true // bAutoDestroy = true
		);
	}

	// Duration 시간 후 Deactivate(새 파티클 스폰 중단)를 호출하여 이미 발사된 스파크는 끝까지 날아가게 유도
	if (NiagaraComp && Duration > 0.0f)
	{
		FTimerHandle DeactivateTimerHandle;
		FTimerDelegate DeactivateDelegate = FTimerDelegate::CreateWeakLambda(NiagaraComp, [NiagaraComp]()
		{
			if (IsValid(NiagaraComp))
			{
				NiagaraComp->Deactivate();
			}
		});

		World->GetTimerManager().SetTimer(DeactivateTimerHandle, DeactivateDelegate, Duration, false);
	}
}

FString UAnimNotify_PlayNiagaraTimed::GetNotifyName_Implementation() const
{
	if (Template)
	{
		return FString::Printf(TEXT("Niagara Timed: %s (%.2fs)"), *Template->GetName(), Duration);
	}
	return TEXT("Play Niagara Timed");
}

