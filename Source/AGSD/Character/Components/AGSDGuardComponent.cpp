#include "Character/Components/AGSDGuardComponent.h"
#include "Character/AGSDCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

UAGSDGuardComponent::UAGSDGuardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 기본 가드 가능 무기로 Spear 추가
	BlockableWeapons.Add(EHoldingWeapon::Spear);
}

void UAGSDGuardComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AAGSDCharacter>(GetOwner());
}

void UAGSDGuardComponent::StartBlock()
{
	bGuardPressed = true;

	if (!OwnerCharacter) return;

	// 가드 불가능한 무기 장착 상태면 무시
	if (!BlockableWeapons.Contains(OwnerCharacter->HoldingWeapon)) return;

	// 공격 중인 경우 ComboWindow(bCanCombo) 상태일 때만 캔슬 후 막기 전환
	if (OwnerCharacter->bIsAttacking)
	{
		if (OwnerCharacter->bCanCombo)
		{
			// 선입력 공격 버퍼 해제하여 캔슬 후 자동 연계 차단
			OwnerCharacter->bHasBufferedInput = false;
			OwnerCharacter->StopAnimMontage();
			OwnerCharacter->ResetAttackState();
		}
		else
		{
			// ComboWindow 구간이 아니라면 막기 캔슬 불허
			return;
		}
	}
	else if (OwnerCharacter->Mining)
	{
		// 순수 채광/기타 행동 중일 때는 가드 불가
		return;
	}

	OwnerCharacter->SetCharacterState(ECharacterState::Block);
	OwnerCharacter->UpdateSprintSpeed();
	OwnerCharacter->UpdateCharacterRotationSettings();
}

void UAGSDGuardComponent::StopBlock()
{
	bGuardPressed = false;

	if (!OwnerCharacter) return;

	if (OwnerCharacter->GetCharacterState() == ECharacterState::Block)
	{
		OwnerCharacter->SetCharacterState(ECharacterState::Combat);
		OwnerCharacter->UpdateSprintSpeed();
		OwnerCharacter->UpdateCharacterRotationSettings();
	}
}

bool UAGSDGuardComponent::IsGuardActive() const
{
	if (!OwnerCharacter) return false;
	return OwnerCharacter->GetCharacterState() == ECharacterState::Block;
}



void UAGSDGuardComponent::HandleJustGuardSuccess()
{
	if (!OwnerCharacter) return;

	SetJustGuardWindowActive(false);
	bJustGuardSuccessful = true;

	if (JustGuardSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, JustGuardSound, OwnerCharacter->GetActorLocation());
	}

	if (JustGuardParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), JustGuardParticle, OwnerCharacter->GetActorLocation());
	}

	// 역경직(Hit-Stop) 연출: 글로벌 타임 딜레이션을 낮췄다가 원복
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.1f);

	FTimerHandle ResetTimeDilationTimer;
	GetWorld()->GetTimerManager().SetTimer(
		ResetTimeDilationTimer, 
		FTimerDelegate::CreateLambda([World = GetWorld()]()
		{
			if (World)
			{
				UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
			}
		}), 
		0.05f, 
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("Just Guard Success!"));

	// 저스트 가드 성공 시 즉시 가드 상태 해제 후 패리 콤보 시작
	StopBlock();
	OwnerCharacter->StartParryCombo();
}

void UAGSDGuardComponent::ResetGuardState()
{
	SetJustGuardWindowActive(false);
	bJustGuardSuccessful = false;
}

void UAGSDGuardComponent::SetJustGuardWindowActive(bool bActive)
{
	bIsJustGuardWindow = bActive;
	UE_LOG(LogTemp, Warning, TEXT("Just Guard Window State Changed: %s"), bActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));
}

bool UAGSDGuardComponent::ProcessDamageMitigation(float IncomingDamage, float& OutMitigatedDamage)
{
	if (!OwnerCharacter) return false;

	if (bIsJustGuardWindow || IsGuardActive())
	{
		OutMitigatedDamage = 0.f;

		if (IncomingDamage >= 0.f && GuardEffect)
		{
			FVector SpawnLocation = OwnerCharacter->GetActorLocation() 
				+ (OwnerCharacter->GetActorForwardVector() * 50.f) 
				+ (OwnerCharacter->GetActorUpVector() * 40.f);

			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), 
				GuardEffect, 
				SpawnLocation, 
				FRotator::ZeroRotator, 
				FVector(0.5f), 
				true
			);
		}
		return true;
	}

	return false;
}
