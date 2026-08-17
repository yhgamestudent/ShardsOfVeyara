// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_WeaponAttack.h"
#include "AGSDCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"

void UAnimNotifyState_WeaponAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp)
	{
		// 시작 시 타격 배열 초기화 및 무기 중앙 위치 저장
		HitActorsMap.Add(MeshComp, TArray<AActor*>());
		
		FVector StartLoc = MeshComp->GetSocketLocation(StartSocketName);
		FVector EndLoc = MeshComp->GetSocketLocation(EndSocketName);
		FVector MidPoint = (StartLoc + EndLoc) * 0.5f;
		
		PreviousLocationMap.Add(MeshComp, MidPoint);
		Player = Cast<AAGSDCharacter>(MeshComp->GetOwner());
		if (Player)
		{
			// 기본 데미지 설정에 현재 콤보 스테이지의 데미지 배율(DamageMultiplier) 적용
			BaseDamage = (Player->WeaponDamage() * Player->getDamage() / 100.f) * Player->GetCurrentAttackDamageMultiplier();
		}

		// 공격(휘두르기) 시작 시 사운드 재생
		if (AttackSound)
		{
			UGameplayStatics::PlaySoundAtLocation(MeshComp->GetWorld(), AttackSound, MidPoint);
		}
	}
}

void UAnimNotifyState_WeaponAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	// 이전 위치가 맵에 없다면 무시
	if (!PreviousLocationMap.Contains(MeshComp)) return;

	FVector PreviousLocation = PreviousLocationMap[MeshComp];
	FVector StartLoc = MeshComp->GetSocketLocation(StartSocketName);
	FVector EndLoc = MeshComp->GetSocketLocation(EndSocketName);
	
	// 현재 무기의 중앙 위치와 방향, 길이 계산
	FVector CurrentLocation = (StartLoc + EndLoc) * 0.5f;
	float CapsuleHalfHeight = FVector::Dist(StartLoc, EndLoc) * 0.5f;
	FRotator CapsuleRotation = FRotationMatrix::MakeFromZ(EndLoc - StartLoc).Rotator();

	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(MeshComp->GetOwner()); // 무기 소유자(자신)는 타격 무시
	
	// 이미 타격한 액터들을 트레이스 단계에서부터 아예 무시되도록 추가 (최적화)
	TArray<AActor*>& HitActors = HitActorsMap[MeshComp];
	ActorsToIgnore.Append(HitActors);

	// Multi Capsule Trace 수행 (네이티브 Sweep을 사용하여 회전 지원)
	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(TraceRadius, CapsuleHalfHeight);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(ActorsToIgnore);
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnFaceIndex = false;

	bool bHit = MeshComp->GetWorld()->SweepMultiByChannel(
		HitResults,
		PreviousLocation,
		CurrentLocation,
		CapsuleRotation.Quaternion(),
		UEngineTypes::ConvertToCollisionChannel(WeaponTraceChannel),
		CapsuleShape,
		QueryParams
	);

	// 디버그 드로잉 (KismetLibrary 기능을 대신함)
	if (DrawDebugTrace != EDrawDebugTrace::None)
	{
		UKismetSystemLibrary::DrawDebugCapsule(
			MeshComp->GetWorld(),
			CurrentLocation,
			CapsuleHalfHeight,
			TraceRadius,
			CapsuleRotation,
			FLinearColor::Red,
			5.0f
		);
	}

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();

			// 유효한 액터인지, 이번 공격에서 이미 맞은 액터가 아닌지, 그리고 타겟 태그를 가지고 있는지 확인
			if (HitActor && !HitActors.Contains(HitActor) && HitActor->ActorHasTag(TargetTag))
			{
				HitActors.Add(HitActor); // 중복 방지 등록
				AController* InstigatorController = MeshComp->GetOwner() ? MeshComp->GetOwner()->GetInstigatorController() : nullptr;
				
				// 타격 방향 계산
				FVector AttackDirection = (CurrentLocation - PreviousLocation).GetSafeNormal();

				// 데미지 가하기
				UGameplayStatics::ApplyPointDamage(
					HitActor,
					BaseDamage,
					AttackDirection,
					Hit,
					InstigatorController,
					MeshComp->GetOwner(),
					UDamageType::StaticClass()
				);

				// 타격 이펙트 (나이아가라) 재생
				if (HitEffect)
				{
					// Hit.ImpactPoint : 타격된 정확한 위치
					// Hit.ImpactNormal.Rotation() : 타격면의 법선 방향 (이펙트가 표면을 향하도록 설정)
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						MeshComp->GetWorld(),
						HitEffect,
						Hit.ImpactPoint,
						Hit.ImpactNormal.Rotation(),
						HitEffectScale
					);
				}

				// 타격음 (사운드) 재생
				if (HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(MeshComp->GetWorld(), HitSound, Hit.ImpactPoint);
				}
			}
		}
	}

	// 다음 프레임을 위해 현재 위치를 갱신 (연속성 유지)
	PreviousLocationMap[MeshComp] = CurrentLocation;
}

void UAnimNotifyState_WeaponAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	// 몽타주 종료 시 메모리에서 맵핑 정보 제거
	if (MeshComp)
	{
		HitActorsMap.Remove(MeshComp);
		PreviousLocationMap.Remove(MeshComp);
	}
}
