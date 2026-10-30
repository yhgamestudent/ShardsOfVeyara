#include "Character/Components/AGSDGuardComponent.h"
#include "Character/AGSDCharacter.h"
#include "Combat/AGSDUnblockableDamageType.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayLogSubsystem.h"
#include "EnemyBoss/BaseBossEnemy.h"

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

	if (UGameInstance* GameInst = OwnerCharacter->GetGameInstance())
	{
		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())
		{
			LogSubsystem->IncrementGuardUsageCount();
			LogSubsystem->RecordInteractionAction(TEXT("Combat_Guard"));
		}
	}
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



void UAGSDGuardComponent::HandleJustGuardSuccess(AActor* DamageCauser)
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

	// 🛡️ 저스트 가드(패링) 성공 행동 횟수 로깅
	if (UGameInstance* GameInst = OwnerCharacter->GetGameInstance())
	{
		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())
		{
			LogSubsystem->RecordInteractionAction(TEXT("Combat_Parry"));
		}
	}

	// 공격 원인 액터에서 실제 공격자(Pawn/Character) 추출
	AActor* ActualAttacker = DamageCauser;
	if (ActualAttacker)
	{
		if (AActor* InstigatorActor = ActualAttacker->GetInstigator())
		{
			ActualAttacker = InstigatorActor;
		}
		else if (AActor* OwnerActor = ActualAttacker->GetOwner())
		{
			ActualAttacker = OwnerActor;
		}
	}

	// 🛡️ 보스/공격자에게 패링 성공 통지 (공격 취소 및 비틀거림 경직 유도)
	if (ABaseBossEnemy* BossEnemy = Cast<ABaseBossEnemy>(ActualAttacker))
	{
		BossEnemy->OnAttackParried(OwnerCharacter);
	}

	// 저스트 가드 성공 시 즉시 가드 상태 해제 후 패리 콤보 시작
	StopBlock();
	OwnerCharacter->LastJustGuardSuccessWorldTime = GetWorld()->GetTimeSeconds();
	OwnerCharacter->bIsJustGuardCounterActive = true;
	OwnerCharacter->StartParryCombo(ActualAttacker);
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

bool UAGSDGuardComponent::CanBlockDamage(const FDamageEvent& DamageEvent, AActor* DamageCauser) const
{
	// 1. 데미지 타입 검사: UAGSDUnblockableDamageType 파생 클래스는 가드/패링 불가
	TSubclassOf<UDamageType> DamageTypeClass = DamageEvent.DamageTypeClass;
	if (!DamageTypeClass)
	{
		DamageTypeClass = UDamageType::StaticClass();
	}
	if (DamageTypeClass && DamageTypeClass->IsChildOf(UAGSDUnblockableDamageType::StaticClass()))
	{
		return false;
	}

	// 2. 액터 태그 검사: 만약 블루프린트에서 "Unblockable" 또는 "Trap" 태그가 지정된 액터인 경우도 가드 불가
	if (DamageCauser && (DamageCauser->ActorHasTag(TEXT("Unblockable")) || DamageCauser->ActorHasTag(TEXT("Trap"))))
	{
		return false;
	}

	return true;
}

bool UAGSDGuardComponent::ProcessDamageMitigation(float IncomingDamage, float& OutMitigatedDamage, const FDamageEvent& DamageEvent, AActor* DamageCauser)
{
	if (!OwnerCharacter) return false;

	// 가드 불가능한 피해(환경 함정, 가드 불가 공격 등)는 가드/패링 처리 스킵
	if (!CanBlockDamage(DamageEvent, DamageCauser))
	{
		return false;
	}

	// 1. 저스트 가드(패링) 타이밍: 100% 완전 방어 (대미지 0)
	// 1. 저스트 가드(패링) 타이밍 또는 반격 활성 중: 100% 완전 방어 (대미지 0)
	if (bIsJustGuardWindow || bJustGuardSuccessful || (OwnerCharacter && OwnerCharacter->bIsJustGuardCounterActive))
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

		OwnerCharacter->ReportTutorialAction(ETutorialActionType::Guard);
		return true;
	}
	// 2. 일반 가드 상태: 설정된 비율(70%)만큼 대미지 경감 (30% 칩 데미지 관통)
	else if (IsGuardActive())
	{
		const float ClampedRate = FMath::Clamp(GuardDamageMitigationRate, 0.0f, 1.0f);
		OutMitigatedDamage = IncomingDamage * (1.0f - ClampedRate);

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

		OwnerCharacter->ReportTutorialAction(ETutorialActionType::Guard);
		return true;
	}

	return false;
}

bool UAGSDGuardComponent::ProcessDamageMitigation(float IncomingDamage, float& OutMitigatedDamage)
{
	FDamageEvent DefaultDamageEvent;
	return ProcessDamageMitigation(IncomingDamage, OutMitigatedDamage, DefaultDamageEvent, nullptr);
}

