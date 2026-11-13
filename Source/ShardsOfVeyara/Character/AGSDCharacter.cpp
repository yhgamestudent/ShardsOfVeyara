// Copyright Epic Games, Inc. All Rights Reserved.



#include "AGSDCharacter.h"

#include "AGSDCloseableUIInterface.h"

#include "Inventory/AGSDInventoryComponent.h"

#include "AGSDLockOnComponent.h"

#include "AGSDInteractionComponent.h"

#include "AGSDGuardComponent.h"

#include "Character/Components/AGSDComboGuideComponent.h"

#include "PickUpItem.h"

#include "Inventory/UI/AGSDPlayerHUD.h"

#include "Framework/Application/SlateApplication.h"

#include "Chest.h"

#include "Blueprint/UserWidget.h"

#include "GameplayLogSubsystem.h"

#include "Engine/LocalPlayer.h"

#include "Camera/CameraComponent.h"

#include "Components/CapsuleComponent.h"

#include "Components/SphereComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "GameFramework/SpringArmComponent.h"

#include "Camera/PlayerCameraManager.h"

#include "GameFramework/Controller.h"

#include "EnhancedInputComponent.h"

#include "EnhancedInputSubsystems.h"

#include "InputActionValue.h"

#include "Interaction.h"

#include "Struct_ItemData.h"

#include "UsableItem.h"

#include "ShardsOfVeyara.h"

#include "FadeWidget.h"
#include "ContentStreaming.h"

#include "HeartProgressBar.h"

#include "PlayerStateWidget.h"

#include "MotionWarpingComponent.h"

#include "SOVGameInstance.h"

#include "Components/ProgressBar.h"

#include "Kismet/GameplayStatics.h"

#include "BaseFlyingPet.h"

#include "Component/PetTalkComponent.h"

#include "TutorialSubsystem.h"

#include "NiagaraFunctionLibrary.h"

#include "SpearComboData.h"

#include "TextLog.h"

#include "Components/AudioComponent.h"

#include "Interface/ItemDropInterface.h"

#include "Kismet/KismetMathLibrary.h"

#include "Kismet/KismetSystemLibrary.h"

#include "EnemyBoss/BaseBossEnemy.h"

#include "BaseEnemy.h"

#include "EnemyProjectile/BaseEnemyProjectile.h"

#include "EnemyProjectile/GroundAttackProjectile.h"

#include "Combat/AGSDBomb.h"



// ─── [체력 회복 물약: 최대 체력 비율 회복] ───
// 물약 BP(BP_HealingPotionBase)는 고정 HealAmount로 HealthRecovery를 호출하지만,
// 봉헌 단계에 따라 최대 체력이 800 → 2150까지 커지므로 물약은 최대 체력 비율로 회복시킨다.
// UseEquippedItem이 물약 UseItem을 동기 호출하는 동안에만 값이 설정됨 (작물/회복 장판/침대 등은 영향 없음)
namespace PotionHealing
{
	static float PendingHealPercent = -1.0f;

	static float GetHealPercent(const FString& ItemID)
	{
		if (ItemID == TEXT("LesserHealingPotion"))   return 0.08f; // 하급: 최대 체력 8%
		if (ItemID == TEXT("GreaterHealingPotion"))  return 0.15f; // 중급: 최대 체력 15%
		if (ItemID == TEXT("SuperiorHealingPotion")) return 0.30f; // 상급: 최대 체력 30%
		return -1.0f;
	}
}

#include "EnemyBoss/SkeletonMage/BossSkeletonMage.h"

#include "CombatTimelineLogger.h"

#include "NiagaraFunctionLibrary.h"

#include "NiagaraSystem.h"

#include "NiagaraComponent.h"

#include "Components/WidgetComponent.h"

#include "Blueprint/UserWidget.h"

#include "Tribute/TributeUI.h"

#include "Tribute/Tribute.h"

#include "FarmingGameMode.h"

#if WITH_EDITOR

#include "AssetTypeActions/AssetDefinition_SoundBase.h"

#endif



AAGSDCharacter::AAGSDCharacter()

{

	// Set size for collision capsule

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

		

	// Don't rotate when the controller rotates. Let that just affect the camera.

	bUseControllerRotationPitch = false;

	bUseControllerRotationYaw = false;

	bUseControllerRotationRoll = false;



	// Configure character movement

	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);



	// 회복 나이아가라 이펙트 기본값 설정 (펫 힐 NS_Heal_02 응용)

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HealVFXFinder(TEXT("/Pet/VFXAsset/Heal/Particles/NS_Heal_02.NS_Heal_02"));

	if (HealVFXFinder.Succeeded())

	{

		HealEffect = HealVFXFinder.Object;

	}



	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint

	// instead of recompiling to adjust them

	GetCharacterMovement()->JumpZVelocity = 500.f;

	GetCharacterMovement()->AirControl = 0.35f;

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;

	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;



	// Create a camera boom (pulls in towards the player if there is a collision)

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = 400.0f;

	CameraBoom->bUsePawnControlRotation = true;



	// Create a follow camera

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));

	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	FollowCamera->bUsePawnControlRotation = false;



	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 

	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)



	//오디오 컴포넌트

	Running = CreateDefaultSubobject<UAudioComponent>(TEXT("RunningAudio"));

	Running->SetupAttachment(RootComponent);

	Running->bAutoActivate = false;



	Jumping = CreateDefaultSubobject<UAudioComponent>(TEXT("JumpingAudio"));

	Jumping->SetupAttachment(RootComponent);

	Jumping->bAutoActivate = false;



	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));



	// 오디오 리스너 컴포넌트 생성 및 부착

	AudioListenerComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AudioListenerComponent"));

	AudioListenerComponent->SetupAttachment(RootComponent);



	// 인벤토리 컴포넌트 생성

	InventoryComponent = CreateDefaultSubobject<UAGSDInventoryComponent>(TEXT("InventoryComponent"));



	// 락온 컴포넌트 생성

	LockOnComponent = CreateDefaultSubobject<UAGSDLockOnComponent>(TEXT("LockOnComponent"));



	// 상호작용 컴포넌트 생성

	InteractionComponent = CreateDefaultSubobject<UAGSDInteractionComponent>(TEXT("InteractionComponent"));



	// 가드 컴포넌트 생성

	GuardComponent = CreateDefaultSubobject<UAGSDGuardComponent>(TEXT("GuardComponent"));



	// 콤보 가이드 컴포넌트 생성

	ComboGuideComponent = CreateDefaultSubobject<UAGSDComboGuideComponent>(TEXT("ComboGuideComponent"));



	OpenedChest = nullptr;



	bIsFaceCameraPressed = false;



	TurnLeft90Montage = nullptr;

	TurnRight90Montage = nullptr;



	static ConstructorHelpers::FObjectFinder<UAnimMontage> AirborneMontageFinder(TEXT("/Game/Spear/Animation/Sequence/08_Hit/07_Hit_Large_To_Air/AS_Hit_Large_To_Air_Seq1_Montage.AS_Hit_Large_To_Air_Seq1_Montage"));

	if (AirborneMontageFinder.Succeeded())

	{

		AirborneMontage = AirborneMontageFinder.Object;

	}



	static ConstructorHelpers::FObjectFinder<UAnimMontage> CombatAirborneMontageFinder(TEXT("/Game/Spear/Animation/Sequence/08_Hit/08_Hit_Combat_Large_To_Air/AS_Hit_Combat_Large_To_Air_Seq1_Montage.AS_Hit_Combat_Large_To_Air_Seq1_Montage"));

	if (CombatAirborneMontageFinder.Succeeded())

	{

		CombatAirborneMontage = CombatAirborneMontageFinder.Object;

	}

	TurnThresholdAngle = 45.0f;

	bIsTurning = false;

	TurnDuration = 0.75f;

	TurnTimer = 0.0f;

	StartRotation = FRotator::ZeroRotator;



	// 기본 장착 소켓 매핑 데이터 세팅 (블루프린트에서 편집 가능)

	EquipSocketMappings.Add(FEquipSocketMapping(TEXT("forke"), FName("Weapon_Actor_R"), FName("Weapon_Actor_R"), EHoldingWeapon::Spear, false));

	EquipSocketMappings.Add(FEquipSocketMapping(TEXT("torch"), FName("Torch"), NAME_None, EHoldingWeapon::Torch, false));

	EquipSocketMappings.Add(FEquipSocketMapping(TEXT("potion"), FName("PotionSocket"), NAME_None, EHoldingWeapon::Potion, true));

}



void AAGSDCharacter::HandleAttackInput(FName ActionName)

{

	float CurrentTime = GetWorld()->GetTimeSeconds();

	int32 CurrentFrame = GFrameCounter;



	FInputBufferEntry NewEntry(ActionName, CurrentTime, CurrentFrame);



	InputBuffer.Add(NewEntry);

}



void AAGSDCharacter::TryInteract()

{

	// 대화 중이거나 UI가 열려있는 동안에는 상호작용 및 공격 차단

	if (IsInDialogue()) return;

	if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen()) return;

	if (ActiveCloseableUI.IsValid()) return;

	if (OpenedChest != nullptr) return;



	UTextLog::WriteTextLogByString(TEXT("상호작용 키 입력"), TEXT("단순 상호작용"));

	// 무기(Spear 등)를 장착 중이거나 공격 진행 중일 경우 우클릭(RMB) 콤보/스킬 신호 전송

	if (HoldingWeapon == EHoldingWeapon::Spear || HoldingWeapon == EHoldingWeapon::Sickle || bIsAttacking)

	{

		ProcessAttackInputWithButton(ESpearAttackInput::RMB);

	}



	if (InteractionComponent)

	{

		InteractionComponent->TryInteract();

		

		if (UWorld* World = GetWorld())

		{

			FString MapName = World->GetMapName();

			MapName.RemoveFromStart(World->StreamingLevelsPrefix);



			// 실제 상호작용한 액터 이름을 합께 기록

			AActor* InteractedActor = InteractionComponent->GetCurrentInteractableActor();

			FString ActorName = IsValid(InteractedActor) ? InteractedActor->GetName() : TEXT("None");



			if (UGameInstance* GameInst = GetGameInstance())

			{

				if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

				{

					LogSubsystem->IncrementStageInteraction(MapName, ActorName);

				}

			}

		}

	}

}







void AAGSDCharacter::ConsumeInputs(TArray<FInputBufferEntry>& Buffer, int32 Count)

{

	// 버퍼가 충분한 입력을 가지고 있는지 확인

	if (Buffer.Num() >= Count)

	{

		// Count만큼 가장 최근 입력(배열의 끝)부터 제거

		for (int32 i = 0; i < Count; ++i)

		{

			Buffer.RemoveAt(Buffer.Num() - 1);

		}

	}

}



bool AAGSDCharacter::CheckCombo(

	const TArray<FInputBufferEntry>& Buffer, 

	FName Input1, 

	FName Input2, 

	float MaxTimeBetweenInputs)

{

	// 1. 최소 길이 확인

	if (Buffer.Num() < 2)

	{

		return false;

	}



	// 2. 최근 입력 두 개 가져오기 (직접 접근)

	const FInputBufferEntry& Entry2 = Buffer[Buffer.Num() - 1]; // Input2 (가장 최근)

	const FInputBufferEntry& Entry1 = Buffer[Buffer.Num() - 2]; // Input1 (바로 이전)

    

	// 3. 순서 확인

	bool bSequenceMatch = (Entry1.InputName == Input1) && (Entry2.InputName == Input2);



	if (!bSequenceMatch)

	{

		return false;

	}



	// 4. 시간 간격 확인

	float TimeDifference = Entry2.TimeStamp - Entry1.TimeStamp;



	// 두 입력 사이의 시간 간격이 허용 시간 내에 있는지 확인

	if (TimeDifference <= MaxTimeBetweenInputs)

	{

		return true; // 커맨드 인식 성공!

	}



	return false;

}



bool AAGSDCharacter::CheckSingleInput(const TArray<FInputBufferEntry>& Buffer, FName InputName)

{

	// 1. 최소 길이 확인

	if (Buffer.Num() < 1)

	{

		return false;

	}



	// 2. 가장 최근 입력의 이름이 우리가 찾는 InputName과 일치하는지 확인

	// (시간 유효성은 이미 Tick 시작 부분의 정리 로직에서 검증되었으므로 시간 비교는 필요 없음)

	const FInputBufferEntry& LastEntry = Buffer[Buffer.Num() - 1];



	return LastEntry.InputName == InputName;

}



void AAGSDCharacter::Tick(float DeltaSeconds)

{

	Super::Tick(DeltaSeconds);



	// [핫바 선입력 적용]: 행동 중 미뤄둔 핫바 교체를 행동이 끝나는 순간 반영
	if (InventoryComponent && !(bIsAttacking || bIsRecovering || bIsRolling || bIsHitReacting || bIsKnockdown || Mining))
	{
		const FString SelectedItemID = InventoryComponent->GetCurrentHotbarItemData().ItemID;
		if (SelectedItemID != HoldingItemData.ItemID)
		{
			UE_LOG(LogTemp, Log, TEXT("[Hotbar] Applying buffered equip: %s -> %s"), *HoldingItemData.ItemID, *SelectedItemID);
			UpdateEquippedActor();
		}
	}



	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);



		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->AddPlayTime(DeltaSeconds);

				LogSubsystem->AddStagePlayTime(MapName, DeltaSeconds);

			}

		}

	}



	SetTotalDistance();

	UpdateActionDurationLogging(DeltaSeconds);



	// 플레이어의 의도된 속도 (DesiredSpeed) 계산: 이동 입력 세기 * 목표 속도

	float InputStrength = GetLastMovementInputVector().Size2D();

	DesiredSpeed = InputStrength * (bIsSprinting ? SprintSpeed : WalkSpeed);



	if (ComboGuideComponent)

	{

		ComboGuideComponent->UpdateComboGuideUI();

	}



	UpdateCharacterRotationSettings();



	// [애님 블루프린트 이전] 턴 판정 및 회전은 AnimBP에서 담당하므로 C++ 강제 쿼터니언 Slerp 회전 비활성화

	// TryStartTurn();

	// if (bIsTurning) ... (C++ 회전 보간 비활성화)



	bIsLockedOn = LockOnComponent && LockOnComponent->IsTargetLocked();



	if (GetController())

	{

		float TargetYaw = 0.0f;

		float TargetPitch = 0.0f;

		AActor* TargetActor = bIsLockedOn ? LockOnComponent->GetLockedTarget() : nullptr;

		if (TargetActor)

		{

			FVector TargetLocation = TargetActor->GetActorLocation();

			FVector CharacterLocation = GetActorLocation();

			FRotator LookAtRotation = (TargetLocation - CharacterLocation).Rotation();

			TargetYaw = LookAtRotation.Yaw;

			TargetPitch = LookAtRotation.Pitch;

		}

		else

		{

			FRotator ControlRot = GetController()->GetControlRotation();

			TargetYaw = ControlRot.Yaw;

			TargetPitch = ControlRot.Pitch;

		}

		float ActorYaw = GetActorRotation().Yaw;

		TurnYawDelta = FRotator::NormalizeAxis(TargetYaw - ActorYaw);



		// AimOffset용 각도 (-90 ~ 90 범위로 클램핑)

		AimYaw = FMath::Clamp(TurnYawDelta, -90.0f, 90.0f);

		AimPitch = FMath::Clamp(FRotator::NormalizeAxis(TargetPitch), -90.0f, 90.0f);

	}

	else

	{

		TurnYawDelta = 0.0f;

		AimYaw = 0.0f;

		AimPitch = 0.0f;

	}



	// FaceCamera 조준, 가드(Block), 락온(LockOn) 상태일 때만 AimOffset 활성화 (부드럽게 0.0 ~ 1.0 보간)

	bool bShouldAim = bIsLockedOn || bIsFaceCameraPressed || (CharacterState == ECharacterState::Block);

	AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, bShouldAim ? 1.0f : 0.0f, DeltaSeconds, 10.0f);



	// 선입력 유효 시간 초과 체크 및 해제

	if (bHasBufferedInput)

	{

		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentTime - BufferedInputTime > AttackBufferDuration)

		{

			bHasBufferedInput = false;

		}

	}



	if (AudioListenerComponent && FollowCamera)

	{

		AudioListenerComponent->SetWorldRotation(FollowCamera->GetComponentRotation());

	}

	

	HandleRotateCharacterStartAttack(DeltaSeconds);

	//HandleRotateCharacterStartGuard(DeltaSeconds);

	

	// --- [락온 업데이트] ---

	if (LockOnComponent)

	{

		LockOnComponent->UpdateLockOnState(DeltaSeconds);

	}

	// ---------------------------------------------



	float CurrentTime = GetWorld()->GetTimeSeconds();



	// 1. 유효 시간이 지난 입력 제거 (버퍼 정리)

	// 오래된 입력은 앞에서부터 제거합니다. (FIFO: First In, First Out)

	for (int32 i = InputBuffer.Num() - 1; i >= 0; --i)

	{

		if (CurrentTime - InputBuffer[i].TimeStamp > InputBufferDuration)

		{

			InputBuffer.RemoveAt(i);

		}

	}



	if (!SkillMotion && CharacterState != ECharacterState::Block)

	{

		// 2. 가장 복잡한 커맨드 (StrongAttack 콤보) 먼저 확인

		// CheckCombo는 Input1="Attack", Input2="Attack"으로 설정해야 합니다. (Attack 2번 연속 입력 가정)

		if (CheckCombo(InputBuffer, FName("Forward"), FName("Attack"), InputDifference)) 

		{

			// 커맨드 인식 성공 시

			StrongAttack();

            

			// ⭐ 수정 1: 사용된 입력 2개를 버퍼에서 제거 ⭐

			ConsumeInputs(InputBuffer, 2); 

		}

        

		// 3. StrongAttack 콤보가 인식되지 않았을 경우, 단일 Attack 입력이 남아있는지 확인

		// 이전 입력 (Attack 2번째)이 너무 늦었거나, Attack 1개만 들어왔을 경우

		else if (CheckSingleInput(InputBuffer, FName("Attack"))) // 버퍼에 입력이 남아있는 경우

		{

			Attack();  

                

				// ⭐ 수정 3: 사용된 입력 1개를 버퍼에서 제거 ⭐

			ConsumeInputs(InputBuffer, 1);

		}

	}



	// --- [UI 상태 변화 감지 및 위젯 동기화] ---

	const bool bCurrentUIActive = IsAnyUIActive();

	if (bCurrentUIActive != bLastAnyUIActive)

	{

		bLastAnyUIActive = bCurrentUIActive;

		if (bCurrentUIActive)

		{

			// UI가 열렸을 때: 상호작용 및 아이템 사용 위젯 즉시 숨김

			if (AAGSDPlayerController* PlayerController = PC ? PC : Cast<AAGSDPlayerController>(GetController()))

			{

				PlayerController->HideLeftItemUseWidget();

				PlayerController->HideInteractionWidget();

			}

		}

		else

		{

			// UI가 닫혔을 때: 손에 든 아이템에 맞춰 좌측 위젯 복원

			UpdateLeftItemUseWidget();

		}

	}



	// --- [상호작용 업데이트] ---

	if (InteractionComponent)

	{

		InteractionComponent->UpdateInteractionState(DeltaSeconds);

	}

}



void AAGSDCharacter::InitializeCombatStatsFromDataTable()

{

	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())

	{

		const FCombatStatRow* StatRow = CombatStatHandle.GetRow<FCombatStatRow>(TEXT("AAGSDCharacter::InitializeCombatStatsFromDataTable"));

		if (StatRow)

		{

			MaxHealth = StatRow->MaxHealth;

			Damage = StatRow->BaseAttackDamage;

			Health = MaxHealth;



			UE_LOG(LogTemp, Warning, TEXT("[CombatStat] Loaded Row: %s -> MaxHealth: %.1f, Damage: %.1f"),

				*CombatStatHandle.RowName.ToString(), MaxHealth, Damage);



			if (GI)

			{

				GI->MaxPlayerHealth = MaxHealth;

				GI->PlayerHealth = Health;

				GI->Damage = Damage;

				GI->bCombatStatsInitialized = true;

			}

		}

		else

		{

			UE_LOG(LogTemp, Warning, TEXT("[CombatStat Warning] Row '%s' not found in DataTable '%s'"),

				*CombatStatHandle.RowName.ToString(), *CombatStatHandle.DataTable->GetName());

		}

	}

}



void AAGSDCharacter::InitializeTributeStats()

{

	if (!TributeDataTable)

	{

		TributeDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/DataTable/DT_TributeData.DT_TributeData"));

	}



	if (GI)

	{

		// GI의 TributeLevel은 다음에 도전할 단계(1-based)이므로, 완료된 단계는 GI->TributeLevel - 1

		CurrentTributeLevel = FMath::Max(0, GI->TributeLevel - 1);

	}



	if (CurrentTributeLevel > 0 && TributeDataTable)

	{

		const FName RowName = FName(*FString::FromInt(CurrentTributeLevel));

		if (const FTributeData* Row = TributeDataTable->FindRow<FTributeData>(RowName, TEXT("InitializeTributeStats")))

		{

			Damage = Row->AttackDamage;

			MaxHealth = Row->MaxHealth;

			Health = FMath::Min(Health, MaxHealth);



			if (PlayerStateWidget)

			{

				PlayerStateWidget->SetDamageText(Damage);

			}

			if (HealthBar)

			{

				HealthBar->UpdateHealth(Health, MaxHealth);

			}



			UE_LOG(LogTemp, Log, TEXT("[Tribute] Initialized Player Tribute Level: %d -> Damage: %.1f, MaxHealth: %.1f"),

				CurrentTributeLevel, Damage, MaxHealth);

		}

	}

}



void AAGSDCharacter::ApplyTributeLevel(int32 TargetCompletedLevel)

{

	if (!TributeDataTable)

	{

		TributeDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/DataTable/DT_TributeData.DT_TributeData"));

	}



	TargetCompletedLevel = FMath::Max(0, TargetCompletedLevel);

	// 최대 봉헌 단계(데이터테이블 행 수) 초과 방지: 초과 시 폴백 공식으로 스탯이 떨어지는 문제 차단
	if (TributeDataTable && TributeDataTable->GetRowNames().Num() > 0)
	{
		TargetCompletedLevel = FMath::Min(TargetCompletedLevel, TributeDataTable->GetRowNames().Num());
	}



	if (TargetCompletedLevel == 0)

	{

		// 0단계: 기본 스펙으로 리셋

		CurrentTributeLevel = 0;

		Damage = 70.0f;

		MaxHealth = 800.0f;

		Health = MaxHealth;

	}

	else if (TributeDataTable)

	{

		CurrentTributeLevel = TargetCompletedLevel;

		const FName RowName = FName(*FString::FromInt(TargetCompletedLevel));

		if (const FTributeData* Row = TributeDataTable->FindRow<FTributeData>(RowName, TEXT("ApplyTributeLevel")))

		{

			Damage = Row->AttackDamage;

			MaxHealth = Row->MaxHealth;

			Health = MaxHealth;

		}

		else

		{

			// 데이터테이블 행을 초과했거나 미발견 시 단계당 +10 공식 폴백

			Damage = 70.0f + (TargetCompletedLevel * 10.0f);

			MaxHealth = 800.0f;

			Health = MaxHealth;

		}

	}

	else

	{

		CurrentTributeLevel = TargetCompletedLevel;

		Damage = 70.0f + (TargetCompletedLevel * 10.0f);

		MaxHealth = 800.0f;

		Health = MaxHealth;

	}



	// 1. UI 즉시 갱신

	if (PlayerStateWidget)

	{

		PlayerStateWidget->SetDamageText(Damage);

	}

	if (HealthBar)

	{

		HealthBar->UpdateHealth(Health, MaxHealth);

	}



	// 2. GameInstance 전역 동기화

	const int32 NextTargetLevel = CurrentTributeLevel + 1;

	if (GI)

	{

		GI->Damage = Damage;

		GI->MaxPlayerHealth = MaxHealth;

		GI->PlayerHealth = Health;

		GI->TributeLevel = NextTargetLevel;

		GI->CurrentLevelTributeItems.Empty();

	}



	// 3. 월드 내 ATribute 제단 액터 실시간 동기화

	if (UWorld* World = GetWorld())

	{

		TArray<AActor*> FoundAltars;

		UGameplayStatics::GetAllActorsOfClass(World, ATribute::StaticClass(), FoundAltars);

		for (AActor* AltarActor : FoundAltars)

		{

			if (ATribute* Altar = Cast<ATribute>(AltarActor))

			{

				Altar->TributeLevel = NextTargetLevel;

				Altar->SetNextTributeUI(NextTargetLevel);

			}

		}



		// 4. 분석 로그 서브시스템 갱신

		if (UGameInstance* GameInst = World->GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->UpdateCurrentTributeLevel(NextTargetLevel);

			}

		}

	}



	// 5. 화면 디버그 메시지 및 콘솔 로그 출력

	const FString Msg = FString::Printf(TEXT("[봉헌 완료: %d단계] 공격력: %.1f | 최대체력: %.1f (다음 목표: %d단계)"),

		CurrentTributeLevel, Damage, MaxHealth, NextTargetLevel);



	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

	if (GEngine)

	{

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Msg);

	}

}



void AAGSDCharacter::AdvanceTributeLevel()

{

	ApplyTributeLevel(CurrentTributeLevel + 1);

}



void AAGSDCharacter::BeginPlay()

{

	Super::BeginPlay();



	PC = Cast<AAGSDPlayerController>(GetController());

	GI = Cast<USOVGameInstance>(GetGameInstance());



	// 1. 전투 스탯 초기화 (CombatStatHandle 우선 적용)

	if (CombatStatHandle.DataTable && !CombatStatHandle.RowName.IsNone())

	{

		// 세션 최초 시작이거나 GI가 아직 초기화되지 않은 경우 데이터테이블에서 로드

		if (!GI || !GI->bCombatStatsInitialized)

		{

			InitializeCombatStatsFromDataTable();

		}

		else

		{

			// 레벨 전환 후 GI에 유지된 플레이어의 진행 상태(체력/공격력) 복원

			Health = GI->PlayerHealth;

			MaxHealth = GI->MaxPlayerHealth;

			Damage = GI->Damage;

		}

	}

	else if (GI)

	{

		// CombatStatHandle 미지정 시 기존 GI 로직 유지 (하위 호환)

		if (GI->MaxPlayerHealth != 0.f)

		{

			Health = GI->PlayerHealth;

			MaxHealth = GI->MaxPlayerHealth;

		}

		Damage = GI->Damage;

	}



	// 2. 봉헌 스탯 초기화 (TributeDataTable 기반)

	InitializeTributeStats();



	if (GetCharacterMovement())

	{

		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	}



	if (CameraBoom)

	{

		CameraBoom->bEnableCameraLag = true;

		CameraBoom->CameraLagSpeed = DefaultCameraLagSpeed;

	}



	UpdateCharacterRotationSettings();



	OnLockOnStateChanged.AddDynamic(this, &AAGSDCharacter::HandleLockOn);



	if (PC && AudioListenerComponent)

	{

		// 카메라 거리에 따른 오디오 감쇠 문제를 해결하기 위해 오디오 리스너를 AudioListenerComponent로 설정합니다.

		// 이 컴포넌트는 위치는 캐릭터를 따르고 회전은 카메라 회전으로 매 프레임 업데이트됩니다.

		PC->SetAudioListenerOverride(AudioListenerComponent, FVector::ZeroVector, FRotator::ZeroRotator);

	}



	if (GI)

	{

		bHasPet = GI->bHasPet;

		Coin = GI->Coin;

	}

	SpawnMyPetAfterTravel(); // 펫 있으면 오픈 레벨 이후 펫 스폰

	

	// 인벤토리 컴포넌트 핫바 변경 델리게이트 바인딩

	if (InventoryComponent)

	{

		InventoryComponent->OnHotbarSelectionChanged.AddDynamic(this, &AAGSDCharacter::OnHotbarSelectionChanged);

		InventoryComponent->OnInventorySlotUpdated.AddDynamic(this, &AAGSDCharacter::OnInventorySlotUpdated);

	}



	// C++ 레벨에서 PlayerHUD 생성 및 뷰포트 추가

	if (PlayerHUDClass)

	{

		APlayerController* PlayerController = Cast<APlayerController>(GetController());

		if (PlayerController && PlayerController->IsLocalController())

		{

			PlayerHUDRef = CreateWidget<UAGSDPlayerHUD>(PlayerController, PlayerHUDClass);

			if (PlayerHUDRef)

			{

				PlayerHUDRef->AddToViewport();

				PlayerHUDRef->InitializeHUD(InventoryComponent);

				UE_LOG(LogTemp, Log, TEXT("AAGSDCharacter::BeginPlay - PlayerHUDRef successfully created."));

			}

			else

			{

				UE_LOG(LogTemp, Warning, TEXT("AAGSDCharacter::BeginPlay - Failed to create PlayerHUDRef widget."));

			}

		}

		else

		{

			UE_LOG(LogTemp, Warning, TEXT("AAGSDCharacter::BeginPlay - PlayerController or LocalController is invalid."));

		}

	}

	else

	{

		UE_LOG(LogTemp, Warning, TEXT("AAGSDCharacter::BeginPlay - PlayerHUDClass is NULL. Please set WBP_PlayerHUD in BP_Farmer details panel."));

	}

	

	if (PlayerHUDRef)

	{

		HealthBar = PlayerHUDRef->WBP_HealthBar;

		PlayerStateWidget = PlayerHUDRef->GetPlayerStateWidget();

		if (PlayerStateWidget)

		{

			PlayerStateWidget->SetDamageText(Damage);

		}

	}



	if (HealthBar)

	{

		HealthBar->UpdateHealth(Health, MaxHealth);

	}

	BeginLevelStartStreamingWait();

	

	MouseSensitivity = GI->MouseSensitivity;



	// 시작 시 등 뒤 무기 거치 상태 초기 업데이트

	UpdateBackWeapon();



	// 게임 시작 시 현재 선택된 핫바 슬롯의 아이템을 즉시 장착합니다.

	UpdateEquippedActor();



	// ⭐️ 플레이어 피격/타겟팅 태그 영구 보장 (에셋 설정 누락이나 변경 방지)

	Tags.AddUnique(FName("Player"));



	LastLocation = GetActorLocation();

}



void AAGSDCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)

{

	DestroyPetBeforeTravel();



	// 현재 레벨 이름 가져오기

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);



	// 어떤 스테이지가 종료되었는지를 로그 기록

	UTextLog::WriteTextLogByString(TEXT("종료된 스테이지"), CurrentLevelName);



	// 현재 남은 체력 잔량 로그 기록

	FString HealthLog = FString::Printf(TEXT("%.1f / %.1f"), Health, MaxHealth);

	UTextLog::WriteTextLogByString(TEXT("남은 체력 잔량"), HealthLog);



	// 현재 가지고 있는 골드 량 기록

	UTextLog::WriteTextLogByFloat(TEXT("현재 골드 량"), static_cast<float>(Coin));



	// 총 이동 거리 기록 (m 단위)

	float DistanceInMeters = TotalDistance / 100.0f;

	UTextLog::WriteTextLogByFloat(TEXT("총 이동 거리"), DistanceInMeters);



	// 현재 스테이지 플레이 시간 기록 (초 단위)

	float StagePlayTime = UGameplayStatics::GetTimeSeconds(GetWorld());

	UTextLog::WriteTextLogByFloat(TEXT("스테이지 플레이 시간"), StagePlayTime);



	GetWorldTimerManager().ClearTimer(LevelStartStreamingTimerHandle);

	Super::EndPlay(EndPlayReason);



	GI->Damage = Damage;

	GI->Coin = Coin;

	GI->PlayerHealth = Health;

	GI->MaxPlayerHealth = MaxHealth;

	GI->bHasPet = bHasPet;



	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			if (MaxHealth > 0.0f)

			{

				LogSubsystem->UpdateLatestHealthPercent((Health / MaxHealth) * 100.0f);

			}

			LogSubsystem->ExportLogsToCSVFile(TEXT(""));

		}

	}

}







void AAGSDCharacter::WakeUp()

{

	if (FadeWidget)

	{

		FadeWidget->OnFadeFinished.RemoveDynamic(this, &AAGSDCharacter::WakeUp);

		FadeWidget->SetTargetOpacity(0.f);

	}



	// 1. 하루 경과 처리 (오전 6시 = 600.f 기상)

	if (AFarmingGameMode* GM = Cast<AFarmingGameMode>(GetWorld()->GetAuthGameMode()))

	{

		GM->NextDay(600.f);

	}



	// 2. 최대 체력(풀피) 회복 및 조작 활성화

	HealthRecovery(getPlayerMaxhealth());

	EnableInput(PC);



	// 3. 침대로 텔레포트

	TeleportToBed();

}



void AAGSDCharacter::Die()

{

	DisableInput(PC);

	

	// 1. 애니메이션 인스턴스 가져오기

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();



	// 2. 안전성 검사 (AnimInstance와 Montage가 존재하는지)

	if (AnimInstance && DyingMontage)

	{

		// 3. 몽타주 재생

		// PlaySlotAnimationAsDynamicMontage 등 여러 방법이 있지만, 일반 몽타주는 아래 함수가 표준입니다.

		AnimInstance->Montage_Play(DyingMontage);

		

		FTimerHandle TimerHandle;// 2. 대기 시간 (초 단위)

		GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this]()

		{

			playFadeWidget(0.0f, 1.0f);

			if (FadeWidget)

			{

				FadeWidget->OnFadeFinished.RemoveDynamic(this, &AAGSDCharacter::WakeUp);

				FadeWidget->OnFadeFinished.AddDynamic(this, &AAGSDCharacter::WakeUp);

			}

		}),

		1.0f, false

		);

	}

	

	if ( OnPlayerDead.IsBound() ) OnPlayerDead.Broadcast(); // 죽었다고 알리기

}



float AAGSDCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,

                                 class AController* EventInstigator, AActor* DamageCauser)

{

	// [회피 무적]: 구르기 중(bIsRolling)이거나 일반 무적 상태 시 피해 및 피격 리액션 완전 무효화 (구르기 끊김 원천 차단)

	if (DamageAmount > 0.f && (bIsRolling || bIsRollInvincible || !bCanBeDamage))

	{

		UE_LOG(LogTemp, Log, TEXT("[Roll] Dodged attack! (bIsRolling=%d, bIsRollInvincible=%d, bCanBeDamage=%d)"), bIsRolling ? 1 : 0, bIsRollInvincible ? 1 : 0, bCanBeDamage ? 1 : 0);

		return 0.f;

	}

	float DamageToApply = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	

	if ( DamageToApply < 0.f )

	{

		Health = FMath::Clamp(Health - DamageToApply, 0.0f, MaxHealth);

		

		if (HealthBar) 

		{

			HealthBar->UpdateHealth(Health, MaxHealth);

		}

		return DamageToApply;

	}

	

	const float IncomingDamage = DamageAmount;

	FString GuardState = TEXT("None");



	if (GuardComponent)

	{

		float MitigatedDamage = DamageToApply;

		if (GuardComponent->ProcessDamageMitigation(DamageToApply, MitigatedDamage, DamageEvent, DamageCauser))

		{

			const float MitigatedAmount = DamageToApply - MitigatedDamage;

			DamageToApply = MitigatedDamage;



			GuardState = (GuardComponent->IsJustGuardSuccessful() || GuardComponent->IsJustGuardActive() || bIsJustGuardCounterActive) ? TEXT("JustGuarded") : TEXT("Guarded");



			if (MitigatedAmount > 0.f)

			{

				if (UGameInstance* GameInst = GetGameInstance())

				{

					if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

					{

						LogSubsystem->AddDamageMitigatedByGuard(MitigatedAmount);

					}

				}

			}

		}

	}



	if (DamageToApply >= 0.f)

	{

		OnHitReceived(DamageCauser);

	}

	

	UE_LOG(LogTemp, Warning, TEXT("Player Take Damage : %f"), DamageToApply);



	if ( DamageToApply > 0.f )

	{

		if (bIsGodMode)

		{

			UE_LOG(LogTemp, Log, TEXT("[GodMode] Damage %f ignored! Health preserved at %f"), DamageToApply, Health);

		}

		else

		{

			Health = FMath::Clamp(Health - DamageToApply, 0.f, MaxHealth);

			if (HealthBar) HealthBar->UpdateHealth(Health, MaxHealth);

		}



		if (UWorld* World = GetWorld())

		{

			FString MapName = World->GetMapName();

			MapName.RemoveFromStart(World->StreamingLevelsPrefix);



			if (UGameInstance* GameInst = GetGameInstance())

			{

				if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

				{

					LogSubsystem->AddStageDamageTaken(MapName, DamageToApply);

					LogSubsystem->UpdateLatestHealthPercent((MaxHealth > 0.f) ? (Health / MaxHealth * 100.f) : 0.f);



					// ─── [플레이어 피격 정밀 위치/원인 로그 파일 기록] ───

					LogSubsystem->LogPlayerDamageTaken(

						GetActorLocation(),

						IncomingDamage,

						DamageToApply,

						Health,

						MaxHealth,

						DamageCauser,

						DamageEvent,

						GuardState

					);

				}

			}

		}



		if (Health > 0.f)

		{

			// ─── [로스트아크식 3단계 피격 리액션 처리] ───

			ProcessHitReaction(DamageToApply, DamageCauser, DamageEvent);

		}



		if ( Health <= 0.f )

		{

			// ─── [사망 원인 로그 기록 추가] ───

			FString CauseName = TEXT("알 수 없는 원인");

			if (IsValid(DamageCauser))

			{

				CauseName = DamageCauser->GetName();

			}

			UTextLog::WriteTextLogByString(TEXT("플레이어 사망원인"), CauseName);

			// ───────────────────────────────────



			if (UWorld* World = GetWorld())

			{

				FString MapName = World->GetMapName();

				MapName.RemoveFromStart(World->StreamingLevelsPrefix);



				if (UGameInstance* GameInst = GetGameInstance())

				{

					if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

					{

						LogSubsystem->RecordPlayerDeath(MapName, CauseName, GetActorLocation());

					}

				}

			}



			bCanBeDamage = false;

			Die();

		}

	}

	

	// 저스트 가드 성공 플래그 리셋 (피격 판정 완료 후 최종 리셋)

	if (GuardComponent && GuardComponent->IsJustGuardSuccessful())

	{

		GuardComponent->ResetJustGuardSuccess();

	}



	return DamageToApply;

}



FString AAGSDCharacter::GetCurrentCombatStateString() const

{

	if (bIsHitReacting || bIsKnockdown)

	{

		if (bIsKnockdown)

		{

			if (CurrentKnockdownType == EKnockdownType::AirborneFloor)

			{

				return TEXT("AirborneFloorKnockdown");

			}

			return TEXT("KneelKnockdown");

		}

		return (HitIntensityValue >= 0.5f) ? TEXT("HeavyHit") : TEXT("LightHit");

	}



	if (GuardComponent)

	{

		if (GuardComponent->IsJustGuardActive() || GuardComponent->IsJustGuardSuccessful())

		{

			return TEXT("JustGuarding");

		}

		if (GuardComponent->IsGuardActive())

		{

			return TEXT("Guarding");

		}

	}



	if (bIsRolling)

	{

		return TEXT("Rolling");

	}



	if (bIsAttacking)

	{

		return TEXT("Attacking");

	}



	if (bIsRecovering)

	{

		return TEXT("AttackRecovery");

	}



	if (GetCharacterMovement() && GetCharacterMovement()->Velocity.SizeSquared() > 100.f)

	{

		return TEXT("Moving");

	}



	return TEXT("Idle");

}



void AAGSDCharacter::InterruptCombatAction(bool bInterruptAttack, bool bInterruptGuard)

{

	// 구르기 중(bIsRolling)일 때는 회피 모션과 무적을 절대 캔슬하지 않고 보존!

	if (bIsRolling)

	{

		return;

	}



	// 1. 공격 중이면 공격 몽타주 종료 및 콤보/선입력 초기화

	if (bInterruptAttack && (bIsAttacking || bIsRecovering))

	{

		bHasBufferedInput = false;

		StopAnimMontage();

		ResetAttackState();

	}



	// 2. 가드(막기) 상태이면 가드 해제 및 모션 종료

	if (bInterruptGuard && CharacterState == ECharacterState::Block && GuardComponent)

	{

		GuardComponent->StopBlock();

	}

}



void AAGSDCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)

{

	// Set up action bindings

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		if (EnhancedInputComponent != nullptr)

		{

			if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))

			{

				UEnhancedInputLocalPlayerSubsystem* EnhancedSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

				if (EnhancedSubsystem)

				{

					EnhancedSubsystem->AddMappingContext(IMC_Farmer, 1);

				}

			}

		}

		// Attack

		if (AttackAction)

		{

			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AAGSDCharacter::UseEquippedItem);

		}



		// Secondary Attack (RMB)

		if (SecondaryAttackAction)

		{

			EnhancedInputComponent->BindAction(SecondaryAttackAction, ETriggerEvent::Started, this, &AAGSDCharacter::Input_SecondaryAttack);

		}



		// Jumping

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAGSDCharacter::Jump);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAGSDCharacter::StopJumping);



		// Sprinting

		if (SprintAction)

		{

			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AAGSDCharacter::SprintStart);

			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AAGSDCharacter::SprintEnd);

		}



		// Rolling (구르기 회피)

		if (RollAction)

		{

			EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &AAGSDCharacter::StartRoll);

		}



		// Moving

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Move);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AAGSDCharacter::StopMove);

		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Look);



		// Looking

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Look);



		// 상호작용 (IA_Interaction) 누르는 순간 TryInteract 호출하도록 바인딩

		if (Interaction)

		{

			EnhancedInputComponent->BindAction(Interaction, ETriggerEvent::Started, this, &AAGSDCharacter::TryInteract);

		}



		//

		// 가드 기능 바인딩

		if (GuardAction && GuardComponent)

		{

			EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Started, GuardComponent.Get(), &UAGSDGuardComponent::StartBlock);

			EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Completed, GuardComponent.Get(), &UAGSDGuardComponent::StopBlock);

		}



		// 락온 기능 바인딩

		if (LockOnAction && LockOnComponent)

		{

			EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, LockOnComponent.Get(), &UAGSDLockOnComponent::ToggleLockOn);

		}



		if (SwitchTargetLeftAction && LockOnComponent)

		{

			EnhancedInputComponent->BindAction(SwitchTargetLeftAction, ETriggerEvent::Started, LockOnComponent.Get(), &UAGSDLockOnComponent::SwitchTargetLeft);

		}



		if (SwitchTargetRightAction && LockOnComponent)

		{

			EnhancedInputComponent->BindAction(SwitchTargetRightAction, ETriggerEvent::Started, LockOnComponent.Get(), &UAGSDLockOnComponent::SwitchTargetRight);

		}



		// 튜토리얼 스킵 기능 바인딩

		if (SkipTutorialAction)

		{

			EnhancedInputComponent->BindAction(SkipTutorialAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::SkipTutorialPressed);

		}



		// 핫바 입력 바인딩

		if (HotbarScrollAction)

		{

			EnhancedInputComponent->BindAction(HotbarScrollAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Input_HotbarScroll);

		}



		if (SelectHotbarAction)

		{

			EnhancedInputComponent->BindAction(SelectHotbarAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Input_SelectHotbar);

			EnhancedInputComponent->BindAction(SelectHotbarAction, ETriggerEvent::Completed, this, &AAGSDCharacter::Input_SelectHotbar);

		}



		if (ToggleInventoryAction)

		{

			EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AAGSDCharacter::Input_ToggleInventory);

		}



		if (FaceCameraAction)

		{

			EnhancedInputComponent->BindAction(FaceCameraAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::FaceCameraInput);

			EnhancedInputComponent->BindAction(FaceCameraAction, ETriggerEvent::Completed, this, &AAGSDCharacter::FaceCameraInput);

			EnhancedInputComponent->BindAction(FaceCameraAction, ETriggerEvent::Canceled, this, &AAGSDCharacter::FaceCameraInput);

		}



		if (PauseAction)

		{

			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AAGSDCharacter::Input_Pause);

		}

	}

	else

	{

		UE_LOG(LogAGSD, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));

	}

}



void AAGSDCharacter::Move(const FInputActionValue& Value)

{

	// input is a Vector2D

	FVector2D MovementVector = Value.Get<FVector2D>();



	// 선입력 판정 및 카메라 기준 월드 입력 벡터 계산

	if (GetController())

	{

		const FRotator Rotation = GetController()->GetControlRotation();

		const FRotator YawRotation(0, Rotation.Yaw, 0);

		LastRawInputVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) * MovementVector.Y + 

							 FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * MovementVector.X;

	}



	// 카메라 기준 입력 방향과 캐릭터 몸체 각도 사이의 상대 각도 계산 (-180 ~ +180도)

	if (MovementVector.SizeSquared() > 0.04f && !LastRawInputVector.IsNearlyZero())

	{

		const FVector ForwardDir = GetActorForwardVector();

		const FVector RightDir = GetActorRightVector();

		const FVector InputNorm = LastRawInputVector.GetSafeNormal2D();



		const float ForwardDot = FVector::DotProduct(ForwardDir, InputNorm);

		const float RightDot = FVector::DotProduct(RightDir, InputNorm);



		InputRollDirection = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));

	}

	else

	{

		InputRollDirection = 0.0f;

	}



	if (Running)

	{

		// 입력이 있고(움직임), 마이닝 중이 아닐 때

		if (MovementVector.SizeSquared() > 0.f && !Mining

			&& !GetCharacterMovement()->IsFalling() && GetCharacterMovement()->Velocity.SizeSquared() > 9.f)

		{

			if (!Running->IsPlaying())

			{

				Running->Play();

			}

		}

		else

		{

			// 입력이 없거나 마이닝 중이면 사운드 정지

			if (Running->IsPlaying())

			{

				Running->Stop();

			}

		}

	}

	

	// 전진 키를 누르고 있을 때 콤보 입력

	if (MovementVector.Y > 0.0f) HandleAttackInput(FName("Forward"));



	// 공격 후딜레이 이동 캔슬 허용 상태에서 이동 입력이 들어오면 공격 몽타주를 캔슬하고 이동 시작

	if (bIsAttacking && bCanMoveCancel)

	{

		if (MovementVector.SizeSquared() > 0.01f)

		{

			bHasBufferedInput = false;

			StopAnimMontage();

			ResetAttackState();

		}

	}



	// 움직이면 안될 때 (채광 중이거나, 넉다운 또는 피격 경직 중일 때 이동 차단)

	if (Mining || bIsKnockdown || bIsHitReacting)	return;

	

	UpdateSprintSpeed();



	// route the input

	DoMove(MovementVector.X, MovementVector.Y);

}



void AAGSDCharacter::StopMove()

{

	LastRawInputVector = FVector::ZeroVector;

	InputRollDirection = 0.0f;

	UpdateSprintSpeed();



	if (Running && Running->IsPlaying())

	{

		Running->Stop();

	}

}



void AAGSDCharacter::Jump()

{

	if (IsInDialogue()) return;

	if (bIsAttacking || bIsRolling || Mining || bIsKnockdown || bIsHitReacting) return;

	if (CharacterState == ECharacterState::Block) return;

 	if (Jumping && !GetCharacterMovement()->IsFalling())

	{

		Jumping->Play();

		UTextLog::WriteTextLogByKeyword(TEXT("점프"));



		if (UWorld* World = GetWorld())

		{

			FString MapName = World->GetMapName();

			MapName.RemoveFromStart(World->StreamingLevelsPrefix);



			if (UGameInstance* GameInst = GetGameInstance())

			{

				if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

				{

					LogSubsystem->IncrementStageJump(MapName);

				}

			}

		}

	}

	Super::Jump();

}



void AAGSDCharacter::StopJumping()

{

	if (Mining) return;

	Super::StopJumping();

}



void AAGSDCharacter::StartRoll()

{

	if (IsInDialogue()) return;

	if (bIsRolling || GetCharacterMovement()->IsFalling()) return;

	if (CharacterState == ECharacterState::Block) return;



	// 피격 중일 때: 넉다운(Knockdown) 상태가 아니면 회피 키(Ctrl)로 즉시 캔슬하고 탈출 (Roll Cancel)

	if (bIsHitReacting || bIsKnockdown)

	{

		if (bIsKnockdown)

		{

			// 넉다운 등 바닥에 넘어진 상태면 구르기 불허

			return;

		}

		ResetHitReact();

		ClearHitStun();

	}



	// 공격 중인 경우: 오직 RollCancel(bCanRollCancel) 노티파이 구간일 때만 공격을 캔슬하고 구르기 실행

	if (bIsAttacking)

	{

		if (bCanRollCancel)

		{

			// 선입력 공격 버퍼 해제 후 공격 몽타주 중단 및 공격 상태 리셋

			bHasBufferedInput = false;

			StopAnimMontage();

			ResetAttackState();

			ReportTutorialAction(ETutorialActionType::RollCancel);

		}

		else

		{

			// RollCancel 구간이 아니라면 공격 캔슬 구르기 불허

			return;

		}

	}

	else if (Mining)

	{

		// 다른 액션/채광 중에도 RollCancel 노티파이가 있다면 캔슬 허용, 없다면 차단

		if (bCanRollCancel)

		{

			bHasBufferedInput = false;

			StopAnimMontage();

			Mining = false;

			bCanRollCancel = false;

			ReportTutorialAction(ETutorialActionType::RollCancel);

		}

		else

		{

			return;

		}

	}



	// 후딜레이(bIsRecovering) 중이면 공격 몽타주를 중단하고 회피 캔슬

	if (bIsRecovering)

	{

		bHasBufferedInput = false;

		StopAnimMontage();

		ResetAttackState();

		ReportTutorialAction(ETutorialActionType::RollCancel);

	}



	bHasBufferedInput = false;

	bIsRolling = true;

	bIsRollInvincible = true;



	// [구르기 루트 모션 보장]: 애님그래프 State Machine의 BS_Roll 블렌드스페이스 루트 모션 추출 보장

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)

	{

		AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);

	}



	GetWorldTimerManager().ClearTimer(RollInvincibleTimerHandle);

	GetWorldTimerManager().SetTimer(RollInvincibleTimerHandle, this, &AAGSDCharacter::EndRollInvincible, RollInvincibleDuration, false);

	UE_LOG(LogTemp, Log, TEXT("[Roll] Roll started: 40-frame (%.3fs) Invincibility activated"), RollInvincibleDuration);



	ReportTutorialAction(ETutorialActionType::Roll);

	UTextLog::WriteTextLogByKeyword(TEXT("구르기"));



	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);



		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->IncrementRollUsageCount(MapName);

			}

		}

	}



	GetWorldTimerManager().SetTimer(RollTimerHandle, this, &AAGSDCharacter::StopRoll, RollCooldown, false);

}



void AAGSDCharacter::EndRollInvincible()

{

	bIsRollInvincible = false;

	UE_LOG(LogTemp, Log, TEXT("[Roll] Roll Invincibility ended (40 frames elapsed)"));

}



void AAGSDCharacter::StopRoll()

{

	bIsRolling = false;

	bIsRollInvincible = false;

	GetWorldTimerManager().ClearTimer(RollInvincibleTimerHandle);



	// 구르기 종료 시 이동키(WASD)를 누르고 있다면 루트 모션 종료로 속도가 0으로 죽지 않도록 바톤 터치

	FVector InputDir = GetLastMovementInputVector();

	if (!InputDir.IsNearlyZero())

	{

		float TargetSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;

		GetCharacterMovement()->Velocity = InputDir.GetSafeNormal() * TargetSpeed;

	}

}



void AAGSDCharacter::SprintStart()

{

	bIsSprinting = true;

	UpdateSprintSpeed();



	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);



		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->IncrementStageDash(MapName);

			}

		}

	}

}



void AAGSDCharacter::SprintEnd()

{

	bIsSprinting = false;

	UpdateSprintSpeed();

}



void AAGSDCharacter::FellOutOfWorld(const UDamageType& dmgType)

{

	ApplyFallPenalty(80.0f);

	Super::FellOutOfWorld(dmgType);

}



float AAGSDCharacter::ApplyFallPenalty(float FallDamage)

{

	if (FallDamage <= 0.0f)

	{

		return 0.0f;

	}



	const float OldHealth = Health;



	// 1. 최소 HP 1 유지 (낙하로 인한 즉사 방지 안전망)

	Health = FMath::Clamp(Health - FallDamage, 1.0f, MaxHealth);

	const float ActualDamageApplied = OldHealth - Health;



	// 2. HUD 체력바 UI 실시간 갱신

	if (HealthBar)

	{

		HealthBar->UpdateHealth(Health, MaxHealth);

	}



	// 3. 피격 반응 (가드 체크 및 콤보 리셋)

	OnHitReceived();



	// 4. 낙하 통계 로깅 자동 기록

	RecordFallRespawn();



	UE_LOG(LogTemp, Log, TEXT("[AGSDCharacter] Fall Penalty Applied: -%.1f HP (Before: %.1f -> After: %.1f)"),

		ActualDamageApplied, OldHealth, Health);



	return ActualDamageApplied;

}



void AAGSDCharacter::RecordFallRespawn()

{

	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);



		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->RecordStageFallRespawn(MapName, GetActorLocation());

			}

		}

	}

}



void AAGSDCharacter::UpdateSprintSpeed()

{

	if (!GetCharacterMovement()) return;



	if (CharacterState == ECharacterState::Block)

	{

		GetCharacterMovement()->MaxWalkSpeed = 170.f;

		GetCharacterMovement()->MaxWalkSpeedCrouched = 170.f;

		return;

	}



	bool bIsMovingBackward = false;

	if (!LastRawInputVector.IsNearlyZero())

	{

		FVector ActorForward = GetActorForwardVector().GetSafeNormal2D();

		FVector InputDir = LastRawInputVector.GetSafeNormal2D();

		float Dot = FVector::DotProduct(ActorForward, InputDir);



		// 내적이 -0.2f보다 작다면 캐릭터의 전방과 이동 입력 방향이 100도 이상 벌어졌으므로 뒷걸음질(후진)로 판단

		if (Dot < -0.2f)

		{

			bIsMovingBackward = true;

		}

	}



	const bool bIsInLowGravityArea = (GetCharacterMovement()->GravityScale < 0.5f);



	if (bIsInLowGravityArea)

	{

		// 중력 왜곡 등 외부 환경에 의해 감속된 상태에서는 감속치(예: 200.f) 유지 (Shift 달리기 무단 덮어쓰기 방지)

		GetCharacterMovement()->MaxWalkSpeed = FMath::Min(GetCharacterMovement()->MaxWalkSpeed, 200.f);

	}

	else

	{

		if (bIsSprinting && !bIsMovingBackward)

		{

			GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		}

		else

		{

			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

		}

	}

}



void AAGSDCharacter::Look(const FInputActionValue& Value)

{

	// input is a Vector2D

	FVector2D LookAxisVector = Value.Get<FVector2D>();



	// route the input

	DoLook(LookAxisVector.X, LookAxisVector.Y);

}



void AAGSDCharacter::FaceCameraInput(const FInputActionValue& Value)

{

	// 대화 중이거나 인벤토리, 상자 등 UI가 열려있는 동안에는 무시합니다.

	if (IsInDialogue())

	{

		bIsFaceCameraPressed = false;

		return;

	}

	if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen())

	{

		bIsFaceCameraPressed = false;

		return;

	}

	if (ActiveCloseableUI.IsValid())

	{

		bIsFaceCameraPressed = false;

		return;

	}

	if (OpenedChest != nullptr)

	{

		bIsFaceCameraPressed = false;

		return;

	}



	bIsFaceCameraPressed = Value.Get<bool>();

	UpdateCharacterRotationSettings();

}





void AAGSDCharacter::playFadeWidget(float startOpacity, float endOpacity, float fadeSpeed)

{

	if (WBP_FadeWidget) 

	{

		if (!FadeWidget) FadeWidget = CreateWidget<UFadeWidget>(PC, WBP_FadeWidget);

                

		if (FadeWidget)

		{

			FadeWidget->SetFadeSpeed(fadeSpeed);

			FadeWidget->SetRenderOpacity(startOpacity);

			FadeWidget->SetTargetOpacity(endOpacity);

			if (!FadeWidget->IsInViewport()) FadeWidget->AddToViewport(100);

		}

	}	

}



void AAGSDCharacter::BeginLevelStartStreamingWait()

{

	// 검은 화면 유지 (1 -> 1)

	playFadeWidget(1.0f, 1.0f);



	LevelStartElapsed = 0.0f;

	GetWorldTimerManager().SetTimer(

		LevelStartStreamingTimerHandle,

		this,

		&AAGSDCharacter::CheckLevelStartStreaming,

		LevelStartPollInterval,

		true);

}



void AAGSDCharacter::CheckLevelStartStreaming()

{

	LevelStartElapsed += LevelStartPollInterval;



	int32 NumWanting = 0;

	if (!IStreamingManager::HasShutdown())

	{

		NumWanting = IStreamingManager::Get().GetNumWantingResources();

	}



	const bool bMinElapsed = LevelStartElapsed >= LevelStartMinFadeDelay;

	const bool bTimedOut = LevelStartElapsed >= LevelStartMaxFadeDelay;



	if (bMinElapsed && NumWanting == 0)

	{

		UE_LOG(LogTemp, Log, TEXT("LevelStartFade: streaming ready after %.2fs"), LevelStartElapsed);

		FinishLevelStartFadeIn();

	}

	else if (bTimedOut)

	{

		UE_LOG(LogTemp, Warning, TEXT("LevelStartFade: timed out after %.2fs (%d resources still pending)"), LevelStartElapsed, NumWanting);

		FinishLevelStartFadeIn();

	}

}



void AAGSDCharacter::FinishLevelStartFadeIn()

{

	GetWorldTimerManager().ClearTimer(LevelStartStreamingTimerHandle);

	playFadeWidget(1.0f, 0.0f);

}



ESpearAttackDirection AAGSDCharacter::GetAttackDirection()

{

	// 콤보 시작 시점(공격 중이 아닌 상태)이며, 스프린트 중이고, 평면 속도가 스프린트 설정 속도(SprintSpeed)에 근접한 경우 Sprint 전용 콤보 로드

	if (bIsSprinting && !bIsAttacking)

	{

		float CurrentSpeed = GetVelocity().Size2D();

		// 가속 상태를 고려해 스프린트 설정 속도의 90% 이상 속도일 때 대시 공격 허용

		if (CurrentSpeed >= (SprintSpeed * 0.9f))

		{

			return ESpearAttackDirection::Sprint;

		}

	}



	// [개선] GetLastMovementInputVector() 대신 직접 저장한 LastRawInputVector 사용

	// 애니메이션 도중에는 캐릭터 이동이 멈춰있어 GetLastMovementInputVector()가 0이 되기 때문

	FVector InputMoveVector = LastRawInputVector;

	if (InputMoveVector.IsNearlyZero()) return ESpearAttackDirection::Neutral;



	FVector Forward = GetActorForwardVector();

	InputMoveVector.Normalize();



	// 내적 연산 (Dot Product)

	float Dot = FVector::DotProduct(Forward, InputMoveVector);



	if (Dot > 0.5f) return ESpearAttackDirection::Forward;

	if (Dot < -0.5f) return ESpearAttackDirection::Backward;

    

	return ESpearAttackDirection::Neutral;

}



float AAGSDCharacter::GetCurrentAttackDamageMultiplier() const

{

	if (CurrentComboData && CurrentComboData->Stages.IsValidIndex(CurrentStageIndex))

	{

		return CurrentComboData->Stages[CurrentStageIndex].DamageMultiplier;

	}

	return 1.0f;

}



void AAGSDCharacter::SetCanCombo(bool b)

{

	bCanCombo = b;

	if (bCanCombo)

	{

		ResetComboWindowBuffer();



		// ComboWindow 진입 시점에 플레이어가 가드 키를 누르고(홀드) 있는 중이라면 즉시 막기로 캔슬 전환

		if (GuardComponent && GuardComponent->IsGuardKeyPressed())

		{

			GuardComponent->StartBlock();

		}

	}

}



void AAGSDCharacter::SetCanRollCancel(bool b)

{

	bCanRollCancel = b;

}



void AAGSDCharacter::EnableMovementFromAttack()

{

	if (bIsAttacking)

	{

		bCanMoveCancel = true;

		Mining = false; // 이동 잠금 조기 해제!

	}

}



void AAGSDCharacter::ResetComboWindowBuffer()

{

	bHasBufferedInput = false;

	bLMBPressedInWindow = false;

	bRMBPressedInWindow = false;

}



void AAGSDCharacter::OnComboWindowEnd()

{

	SetCanCombo(false);



	// 콤보 윈도우 종료 시점에 수집된 버퍼된 입력이 있으면 연계 실행

	if (bHasBufferedInput)

	{

		TryExecuteBufferedAttack();

	}

}



void AAGSDCharacter::ProcessAttackInputWithButton(ESpearAttackInput PressedInput)

{

	// 대화 중이거나 UI 활성 상태이면 공격 입력 완전 차단

	if (IsInDialogue()) return;

	if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen()) return;

	if (ActiveCloseableUI.IsValid()) return;

	if (OpenedChest != nullptr) return;



	// 구르기 중이거나 공중 상태인 경우 공격 불가

	if (bIsRolling || (GetCharacterMovement() && GetCharacterMovement()->IsFalling())) return;



	// 피격 경직(Heavy/Knockdown) 중에는 신규 공격 입력 차단

	if (bIsHitReacting || bIsKnockdown) return;



	// ⭐️ 막기(Block) 상태 중인 경우 일반 공격 입력 완전 차단 (사용자 기획 정책 반영)

	if (CharacterState == ECharacterState::Block)

	{

		// 저스트가드(패링) 성공 상태인 경우에만 가드를 풀고 반격 콤보 시작

		if (GuardComponent && GuardComponent->IsJustGuardSuccessful())

		{

			GuardComponent->StopBlock();

			StartParryCombo();

			return;

		}

		// 일반 가드 중에는 공격 입력 완전 무시

		return;

	}



	// 이미 콤보 결정 대기 중인 경우

	if (bIsWaitingForComboDecision)

	{

		// 이전에 입력된 버튼과 반대쪽 버튼(좌클릭과 우클릭)이 입력된 경우 -> 즉시 양클릭(Both_LMB_RMB) 콤보 확정!

		if (PendingComboInput != PressedInput && 

		   (PendingComboInput == ESpearAttackInput::LMB || PendingComboInput == ESpearAttackInput::RMB) &&

		   (PressedInput == ESpearAttackInput::LMB || PressedInput == ESpearAttackInput::RMB))

		{

			GetWorld()->GetTimerManager().ClearTimer(ComboInputDecisionTimerHandle);

			bIsWaitingForComboDecision = false;

			ConfirmAndExecuteCombo(ESpearAttackInput::Both_LMB_RMB);

		}

		// 동일한 버튼 추가 연타 시에는 기존 대기 상태 유지

		return;

	}



	// 대기 중이 아닌 경우 -> 첫 입력 수집 및 대기 타이머 가동

	PendingComboInput = PressedInput;



	// 대기 유효 시간(SimultaneousInputWindow, 에디터/블루프린트 설정 가능)이 0 이하이면 즉시 확정

	if (SimultaneousInputWindow <= 0.0f)

	{

		ConfirmAndExecuteCombo(PressedInput);

		return;

	}



	bIsWaitingForComboDecision = true;

	GetWorld()->GetTimerManager().SetTimer(

		ComboInputDecisionTimerHandle,

		this,

		&AAGSDCharacter::OnComboDecisionTimeout,

		SimultaneousInputWindow,

		false

	);

}



void AAGSDCharacter::OnComboDecisionTimeout()

{

	if (!bIsWaitingForComboDecision) return;



	bIsWaitingForComboDecision = false;

	ConfirmAndExecuteCombo(PendingComboInput);

}



void AAGSDCharacter::ConfirmAndExecuteCombo(ESpearAttackInput DeterminedInput)

{

	// 1. 콤보 결정 로그 및 화면 디버그 메시지 출력

	FString InputStr = TEXT("LMB (Left Click)");

	if (DeterminedInput == ESpearAttackInput::RMB)

	{

		InputStr = TEXT("RMB (Right Click)");

	}

	else if (DeterminedInput == ESpearAttackInput::Both_LMB_RMB)

	{

		InputStr = TEXT("Both_LMB_RMB (Both Click)");

	}



	UE_LOG(LogTemp, Log, TEXT("[Combo Decision] Determined Combo: %s"), *InputStr);

	if (GEngine)

	{

		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("[Combo Decision] %s"), *InputStr));

	}



	// 2. 콤보 연계 실행

	float CurrentTime = GetWorld()->GetTimeSeconds();



	// 공격 중이거나 복귀 중인 경우

	if (bIsAttacking || bIsRecovering) 

	{

		// 1. 공격 중 콤보 가능 구간(ComboWindow)인 경우 -> 대기 없이 즉시 다음 콤보 단계 연계 실행!

		if (bCanCombo)

		{

			ResetComboWindowBuffer();

			ExecuteNextStageWithInput(DeterminedInput);

		}

		// 2. 복귀 중인 경우

		else if (bIsRecovering)

		{

			StartNewComboWithInput(DeterminedInput);

		}

		// 3. 콤보 윈도우 열리기 전 (선입력 버퍼링)

		else

		{

			bHasBufferedInput = true;

			BufferedInputTime = CurrentTime;

			BufferedInput = DeterminedInput;

		}

		return;

	}



	// 완전히 Idle 상태인 경우 새로운 콤보 시작

	StartNewComboWithInput(DeterminedInput);

}



void AAGSDCharacter::ProcessAttackInput()

{

	ProcessAttackInputWithButton(ESpearAttackInput::LMB);

}



void AAGSDCharacter::UseEquippedItem()

{

	// 구르기 중에는 공격 및 아이템 사용 차단

	if (bIsRolling) return;



	// 가드(Block) 상태 중인 경우

	if (CharacterState == ECharacterState::Block)

	{

		// 저스트가드가 성공한 상태라면 가드를 해제하고 카운터 콤보 시작

		if (GuardComponent && GuardComponent->IsJustGuardSuccessful())

		{

			GuardComponent->StopBlock();

			StartParryCombo();

			return;

		}

		// 저스트가드 성공 상태가 아닌 일반 가드 중에는 공격 차단

		return;

	}



	// 대화 중이거나 인벤토리, 상자 등 UI가 열려있는 동안에는 무기 공격이나 아이템 사용이 나가지 않도록 차단합니다.

	if (IsInDialogue())

	{

		return;

	}

	if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen())

	{

		return;

	}

	if (ActiveCloseableUI.IsValid())

	{

		return;

	}

	if (OpenedChest != nullptr)

	{

		return;

	}



	// 1. 들고 있는 아이템이 UsableItem 인터페이스를 구현한 경우 최우선 실행

	bool bUsed = false;

	if (HoldingItemData.ItemBPClass)

	{

		AActor* DefaultActor = Cast<AActor>(HoldingItemData.ItemBPClass->GetDefaultObject());

		if (DefaultActor && DefaultActor->Implements<UUsableItem>())

		{

			const bool bIsPotion = IsPotionItem(HoldingItemData);

			const bool bIsCrop = IsCropItem(HoldingItemData);



			// 🚫 행동 중(공격/구르기/피격/넉다운/채집/공중) 포션 사용 차단
			// - 공격 모션 도중 핫바를 포션으로 바꿔 즉시 마시는 꼼수 방지
			if (bIsPotion)
			{
				const bool bIsAirborne = GetCharacterMovement() && GetCharacterMovement()->IsFalling();
				const bool bIsBusy = bIsAttacking || bIsRecovering || bIsRolling || bIsHitReacting || bIsKnockdown || Mining || bIsAirborne;
				if (bIsBusy)
				{
					UE_LOG(LogTemp, Log, TEXT("[Potion] Blocked - player is busy (Attack:%d Recover:%d Roll:%d Hit:%d Knockdown:%d Mining:%d Air:%d)"),
						bIsAttacking ? 1 : 0, bIsRecovering ? 1 : 0, bIsRolling ? 1 : 0, bIsHitReacting ? 1 : 0, bIsKnockdown ? 1 : 0, Mining ? 1 : 0, bIsAirborne ? 1 : 0);
					return;
				}
			}



			// ⏳ 글로벌 포션 쿨다운 검사: 어떤 포션이든 쿨다운 중이면 모든 포션 사용 불가

			if (bIsPotion && IsPotionOnCooldown())

			{

				UE_LOG(LogTemp, Log, TEXT("[Potion] On Cooldown! Remaining: %.1fs"), GetPotionCooldownRemaining());

				if (AAGSDPlayerController* MyPC = PC ? PC : Cast<AAGSDPlayerController>(GetController()))

				{

					MyPC->ShowWarningNotice(FText::FromString(TEXT("물약 재사용 대기 중입니다.")));

				}

				return;

			}



			// 🛡️ 체력 물약 또는 작물인 경우: 체력이 이미 가득 차 있다면 사용 차단 (아이템 낭비 및 로그 데이터 왜곡 방지)

			if (bIsPotion || bIsCrop)

			{

				if (Health >= MaxHealth)

				{

					if (AAGSDPlayerController* MyPC = PC ? PC : Cast<AAGSDPlayerController>(GetController()))

					{

						MyPC->ShowWarningNotice(FText::FromString(TEXT("체력이 이미 가득 차 있습니다.")));

					}

					return;

				}

			}



			// 체력 회복 물약이면 UseItem → HealthRecovery 동안 최대 체력 비율 회복 적용
			PotionHealing::PendingHealPercent = bIsPotion ? PotionHealing::GetHealPercent(HoldingItemData.ItemID) : -1.0f;

			IUsableItem::Execute_UseItem(DefaultActor, this);

			PotionHealing::PendingHealPercent = -1.0f;

			bUsed = true;



			// ⏳ 포션 사용 성공 시 글로벌 공유 쿨다운 시작

			if (bIsPotion && GetWorld())

			{

				LastGlobalPotionUseTime = GetWorld()->GetTimeSeconds();

			}



			// 소모품 사용 후 좌측 위젯 갱신

			UpdateLeftItemUseWidget();



			// 📜 체력 물약 등 소비 아이템 사용 로깅

			if (UGameInstance* GameInst = GetGameInstance())

			{

				if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

				{

					LogSubsystem->RecordHealthPotionUsage(HoldingItemData.ItemID);

				}

			}

		}

	}



	// 2. UsableItem이 아니고 무기(Spear)나 낫(Sickle) 등 공격 가능한 도구/무기인 경우

	if (!bUsed && (HoldingWeapon == EHoldingWeapon::Spear || HoldingWeapon == EHoldingWeapon::Sickle))

	{

		ProcessAttackInputWithButton(ESpearAttackInput::LMB);

	}

}



void AAGSDCharacter::Input_SecondaryAttack()

{

	// 대화 중이거나 구르기/UI 모드 중에는 공격 차단

	if (IsInDialogue()) return;

	if (bIsRolling) return;



	// 인벤토리, 상자 등 UI가 열려있는 동안에는 공격 차단

	if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen()) return;

	if (ActiveCloseableUI.IsValid()) return;

	if (OpenedChest != nullptr) return;



	// ★ 핵심 설계: 4타 피니시(Combo 5-4)는 상시 단독 발동하지 않고, 공격 진행 중 콤보 윈도우(피니시) 시점에만 의도적으로 발동!

	// 비공격 상태(평상시)에서는 단독 실행을 차단하여 평상시 가드(우클릭)와의 조작 충돌을 원천 차단

	if (!bIsAttacking)

	{

		return;

	}



	if (HoldingWeapon == EHoldingWeapon::Spear || HoldingWeapon == EHoldingWeapon::Sickle)

	{

		// 대기 타이머가 돌고 있다면 취소하고 즉시 Both_LMB_RMB 콤보 피니시 확정 실행

		if (bIsWaitingForComboDecision)

		{

			GetWorld()->GetTimerManager().ClearTimer(ComboInputDecisionTimerHandle);

			bIsWaitingForComboDecision = false;

		}

		ConfirmAndExecuteCombo(ESpearAttackInput::Both_LMB_RMB);

	}

}



void AAGSDCharacter::ActivateAttackRotate()

{

	if (FollowCamera)

	{

		// 목표 방향 설정 (카메라의 Yaw)

		const FRotator ControlRot = GetControlRotation();

		TargetAttackRotation = FRotator(0.f, ControlRot.Yaw, 0.f);



		// 회전 로직 초기화

		bIsRotatingToCamera = true;

		RotationTimer = 0.0f; // 타이머 리셋

	}

}



void AAGSDCharacter::HandleRotateCharacterStartAttack(float DeltaSeconds)

{

	if ( bIsRotatingToCamera )

	{

		// 1. 타이머 업데이트 및 시간 초과 체크

		RotationTimer += DeltaSeconds;

		if (RotationTimer >= MaxRotationTime)

		{

			bIsRotatingToCamera = false;

			return; // 시간 다 되면 종료

 		}



		FRotator CurrentRotation = GetActorRotation();



		// 2. 목표 도달 여부 체크 (이미 카메라 방향이라면 멈춤)

		if (CurrentRotation.Equals(TargetAttackRotation, 1.0f)) // 1도 이내 오차

		{

			bIsRotatingToCamera = false;

			return;

		}



		// 3. 일정한 속도로 회전 (RInterpToConstant)

		FRotator NewRotation = FMath::RInterpConstantTo(

			CurrentRotation, 

			TargetAttackRotation, 

			DeltaSeconds, 

			RotationSpeed

		);



		SetActorRotation(NewRotation);

	}

}



void AAGSDCharacter::StartParryCombo(AActor* Attacker)

{

	LastParryAttacker = Attacker;



	// 락온 대상이 없는 경우(노락온)에만 튕겨낸 공격자 방향으로 즉시 몸을 회전

	AActor* LockedTarget = LockOnComponent ? LockOnComponent->GetLockedTarget() : nullptr;

	if (!LockedTarget && Attacker && IsValid(Attacker))

	{

		const FVector PlayerLoc = GetActorLocation();

		const FVector AttackerLoc = Attacker->GetActorLocation();

		FRotator FaceRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLoc, AttackerLoc);

		FaceRotation.Pitch = 0.f;

		FaceRotation.Roll = 0.f;

		SetActorRotation(FaceRotation);

	}



	// 1. 가드 상태 해제 (패리 성공했으므로 공격(Combat) 상태로 전이)

	if (GuardComponent)

	{

		GuardComponent->StopBlock();

		// 주의: GuardComponent->ResetJustGuardSuccess()는 ProcessHitReaction 면역 검사가 끝난 뒤 TakeDamage 종료 시점에 리셋

	}

	else if (CharacterState == ECharacterState::Block)

	{

		SetCharacterState(ECharacterState::Combat);

		UpdateSprintSpeed();

		UpdateCharacterRotationSettings();

	}



	// 2. 기존 공격/콤보 상태 강제 클리어 및 저스트가드 반격 슈퍼아머 활성화

	ResetCombo();

	Mining = true; // 공격 시작 표시

	bIsJustGuardCounterActive = true;

	LastJustGuardSuccessWorldTime = GetWorld()->GetTimeSeconds();



	// 3. Parry 전용 콤보 로드 및 1타(튕겨내기) 재생

	CurrentComboData = GetComboDataByDirection(ESpearAttackDirection::Parry);

	if (CurrentComboData && CurrentComboData->Stages.Num() > 0)

	{

		CurrentStageIndex = 0;

		PlayStage(0);

	}

}



void AAGSDCharacter::StartNewComboWithInput(ESpearAttackInput Input)

{

	if (IsInDialogue()) return;

	if (Mining || bIsRolling) return;

	// 공중 체공 중(점프 또는 넉백 피격으로 비행 중)에는 지상 평타 입력 무시

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling()) return;



	// ⭐️ 신규 공격 개시 시 가드(Block) 상태 잔류 방지: 가드가 켜져 있다면 해제하고 Combat 상태로 정상 전이

	if (CharacterState == ECharacterState::Block)

	{

		if (GuardComponent)

		{

			GuardComponent->StopBlock();

		}

		else

		{

			SetCharacterState(ECharacterState::Combat);

		}

	}



	ESpearAttackDirection CurrentDir = GetAttackDirection();

	Mining = true;

	

	// 방향과 버튼 입력 조건에 맞는 콤보 데이터 검색

	CurrentComboData = GetComboDataByDirectionAndInput(CurrentDir, Input);



	if (CurrentComboData && CurrentComboData->Stages.Num() > 0)

	{

		CurrentStageIndex = 0;

		PlayStage(0);

	}

	else

	{

		Mining = false;

	}

}



void AAGSDCharacter::StartNewCombo()

{

	StartNewComboWithInput(ESpearAttackInput::LMB);

}



FSpearComboData* AAGSDCharacter::GetComboDataByDirectionAndInput(ESpearAttackDirection Direction, ESpearAttackInput Input)

{

	if (!SpearComboDataTable) return nullptr;



	static const FString ContextString(TEXT("Spear Attack Context"));

	TArray<FSpearComboData*> AllRows;

	SpearComboDataTable->GetAllRows<FSpearComboData>(ContextString, AllRows);



	// 1차 검색: 방향과 입력 요구사항이 정확히 일치하는 항목 검색

	for (FSpearComboData* ComboData : AllRows)

	{

		if (ComboData && ComboData->DirectionRequirement == Direction && ComboData->InputRequirement == Input)

		{

			return ComboData;

		}

	}



	// 2차 검색: 방향 무관(Any)으로 등록되어 해당 입력 조건을 만족하는 항목 검색

	for (FSpearComboData* ComboData : AllRows)

	{

		if (ComboData && ComboData->DirectionRequirement == ESpearAttackDirection::Any && ComboData->InputRequirement == Input)

		{

			return ComboData;

		}

	}



	return nullptr;

}



FSpearComboData* AAGSDCharacter::GetComboDataByDirection(ESpearAttackDirection Direction)

{

	return GetComboDataByDirectionAndInput(Direction, ESpearAttackInput::LMB);

}



void AAGSDCharacter::ExecuteNextStageWithInput(ESpearAttackInput Input)

{

	ESpearAttackDirection CurrentDir = GetAttackDirection();



	if (CurrentComboData)

	{

		// 방향 키 입력이 변경되어 현재 콤보 요구 방향과 다를 경우 해당 방향 및 버튼에 어울리는 새로운 콤보 1타로 전환 (Parry 콤보 진행 중이거나 Any 방향일 경우 제외)

		if (CurrentComboData->DirectionRequirement != ESpearAttackDirection::Parry &&

			CurrentComboData->DirectionRequirement != ESpearAttackDirection::Any &&

			CurrentComboData->DirectionRequirement != CurrentDir)

		{

			FSpearComboData* NewComboData = GetComboDataByDirectionAndInput(CurrentDir, Input);

			if (NewComboData && NewComboData->Stages.Num() > 0)

			{

				CurrentComboData = NewComboData;

				CurrentStageIndex = 0;

				PlayStage(0);

				return;

			}

		}



		int32 NextIndex = CurrentStageIndex + 1;



		// 다음 스테이지가 존재할 경우에만 요구 입력 검증 후 실행

		if (CurrentComboData->Stages.IsValidIndex(NextIndex))

		{

			const FSpearStageData& NextStage = CurrentComboData->Stages[NextIndex];



			if (NextStage.InputRequirement == Input)

			{

				CurrentStageIndex = NextIndex;

				PlayStage(NextIndex);



				// 3연타 이상 콤보 도달 시 튜토리얼 콤보 달성 리포트

				if (NextIndex >= 2)

				{

					ReportTutorialAction(ETutorialActionType::AttackCombo);

				}

			}

			else

			{

				// 입력 버튼이 다를 경우 해당 입력 버튼 및 방향에 어울리는 새로운 콤보 탐색 (Parry 콤보일 경우 제외)

				if (CurrentComboData->DirectionRequirement != ESpearAttackDirection::Parry)

				{

					FSpearComboData* NewComboData = GetComboDataByDirectionAndInput(CurrentDir, Input);

					if (NewComboData && NewComboData->Stages.Num() > 0)

					{

						CurrentComboData = NewComboData;

						CurrentStageIndex = 0;

						PlayStage(0);

					}

				}

			}

		}

		// 마지막 스테이지 다음으로 넘어가려고 하면 새로운 입력 및 방향 콤보 로드

		else

		{

			FSpearComboData* NewComboData = GetComboDataByDirectionAndInput(CurrentDir, Input);



			if (NewComboData && NewComboData->Stages.Num() > 0)

			{

				CurrentComboData = NewComboData;

				CurrentStageIndex = 0;

				PlayStage(0);

			}

			else

			{

				ResetAttackState();

			}

		}

	}

	else

	{

		StartNewComboWithInput(Input);

	}

}



void AAGSDCharacter::ExecuteNextStage()

{

	ExecuteNextStageWithInput(BufferedInput);

}



void AAGSDCharacter::TryExecuteBufferedAttack()

{

	if (!bHasBufferedInput) return;



	float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - BufferedInputTime <= AttackBufferDuration)

	{

		ESpearAttackInput InputToUse = BufferedInput;

		bHasBufferedInput = false;

		ExecuteNextStageWithInput(InputToUse);

	}

	else

	{

		bHasBufferedInput = false;

	}

}



void AAGSDCharacter::PlayStage(int32 Index)

{

	if (!CurrentComboData || !CurrentComboData->Stages.IsValidIndex(Index))

	{

		ResetAttackState();

		return;

	}



	FSpearStageData& Stage = CurrentComboData->Stages[Index];

	bool bPlaySuccess = false;



	if (Stage.AttackMontage)

	{

		UpdateMotionWarpTarget();



		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance)

		{

			float Duration = PlayAnimMontage(Stage.AttackMontage);

			if (Duration > 0.f)

			{

				// 실제 공격 몽타주 재생이 시작된 순간에만 카메라 방향으로 정렬 회전

				ActivateAttackRotate();



				bIsAttacking = true;

				bIsRecovering = false; // 새로운 공격 시작 시 복귀 상태 해제

				bCanCombo = false;

				bHasBufferedInput = false;



				if (ComboGuideComponent)

				{

					ComboGuideComponent->UpdateComboGuideUI();

				}



				// 몽타주 종료 델리게이트 바인딩

				FOnMontageEnded MontageEndedDelegate;

				MontageEndedDelegate.BindUObject(this, &AAGSDCharacter::OnAttackMontageEnded);

				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, Stage.AttackMontage);

				

				bPlaySuccess = true;



				if (UGameInstance* GameInst = GetGameInstance())

				{

					if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

					{

						FString ComboName = Stage.AttackName.IsEmpty() ? FString::Printf(TEXT("Combo_Dir%d"), (int32)CurrentComboData->DirectionRequirement) : Stage.AttackName.ToString();



						if (Index == 0)

						{

							LogSubsystem->RecordUsedCombo(ComboName);

						}

						if (Index == CurrentComboData->Stages.Num() - 1)

						{

							LogSubsystem->RecordCompletedCombo(ComboName);

						}

					}

				}

			}

		}

	}



	// 몽타주 재생에 실패했거나 에셋이 없는 경우, 상태를 즉시 초기화하여 조작 불가 방지

	if (!bPlaySuccess)

	{

		UE_LOG(LogTemp, Warning, TEXT("PlayStage failed at index %d! Resetting attack state."), Index);

		ResetAttackState();

	}

}



void AAGSDCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)

{

	// 입력 버퍼가 있다면 이미 ExecuteNextStage()에서 다음 공격이 시작되었을 것이므로 무시

	if (bInterrupted) return;



	// 현재 스테이지 데이터 참조

	if (CurrentComboData && CurrentComboData->Stages.IsValidIndex(CurrentStageIndex))

	{

			ResetAttackState();

	}

}



void AAGSDCharacter::StartRecovery(UAnimMontage* RecoveryMontage)

{

	if (!RecoveryMontage) return;



	bIsRecovering = true;

	bCanCombo = false; // 복귀 중에는 일반 콤보 창은 닫힘 (단, 새로운 입력을 통한 캔슬은 허용)

    

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	PlayAnimMontage(RecoveryMontage);



	// 복귀 완료 시 상태 최종 리셋을 위한 바인딩

	FOnMontageEnded RecoveryEndedDelegate;

	RecoveryEndedDelegate.BindUObject(this, &AAGSDCharacter::OnRecoveryFinished);

	AnimInstance->Montage_SetEndDelegate(RecoveryEndedDelegate, RecoveryMontage);

}



void AAGSDCharacter::ResetCombo()

{

	// 1. 모든 상태 플래그 초기화

	bIsAttacking = false;

	bIsRecovering = false;

	bCanCombo = false;

	bCanRollCancel = false;

	bCanMoveCancel = false;

	bHasBufferedInput = false;

	Mining = false;

    

	// 2. 콤보 데이터 초기화

	CurrentStageIndex = -1;

	CurrentComboData = nullptr;

	LastParryAttacker = nullptr;



	// 3. (추가) 수비 시스템 관련 상태도 함께 초기화 (단, 저스트 가드 반격기 진입 중에는 패링 성공 상태 보존)

	if (GuardComponent && !bIsJustGuardCounterActive)

	{

		GuardComponent->ResetGuardState();

	}



	// 몽타주가 실행 중이라면 정지 (선택 사항)

	// GetMesh()->GetAnimInstance()->StopAllMontages(0.2f);



	UE_LOG(LogTemp, Log, TEXT("Combo System Reset"));

}



void AAGSDCharacter::OnHitReceived(AActor* DamageCauser)

{

	const bool bIsUnblockable = DamageCauser && (DamageCauser->ActorHasTag(TEXT("Unblockable")) || DamageCauser->ActorHasTag(TEXT("Trap")));

	if (GuardComponent && GuardComponent->IsJustGuardActive() && !bIsUnblockable && !GuardComponent->IsOwnerAirborne())

	{

		GuardComponent->HandleJustGuardSuccess(DamageCauser);

	}

	else

	{

		// 피격 경직(콤보 리셋)을 적용해야 하는 상황에서만 콤보를 끊고 리셋

		if (ShouldApplyHitReaction())

		{

			ResetCombo();

		}

	}

}



bool AAGSDCharacter::ShouldApplyHitReaction() const

{

	// 캐릭터가 공격 중이 아니거나 복귀 중이 아닌 완전히 Idle 상태라면 피격 리액션 적용

	if (!bIsAttacking && !bIsRecovering)

	{

		return true;

	}



	// 콤보 진행 중이라면 현재 단계의 bBreakComboOnHit 설정값을 따름

	if (CurrentComboData && CurrentComboData->Stages.IsValidIndex(CurrentStageIndex))

	{

		return CurrentComboData->Stages[CurrentStageIndex].bBreakComboOnHit;

	}



	return true;

}



void AAGSDCharacter::OnRecoveryFinished(UAnimMontage* Montage, bool bInterrupted)

{

	// 다른 공격에 의해 캔슬(Interrupted)된 게 아니라 자연 종료된 경우에만 상태 리셋

	if (!bInterrupted)

	{

		ResetAttackState();

	}

}



void AAGSDCharacter::ResetAttackState()

{

	if (bIsWaitingForComboDecision)

	{

		GetWorld()->GetTimerManager().ClearTimer(ComboInputDecisionTimerHandle);

		bIsWaitingForComboDecision = false;

	}



	// 선입력된 공격이 있고, 입력된 지 설정된 시간(AttackBufferDuration) 이내인 경우에만 유효한 것으로 판정

	float CurrentTime = GetWorld()->GetTimeSeconds();

	bool bShouldTriggerBufferedAttack = bHasBufferedInput && (CurrentTime - BufferedInputTime <= AttackBufferDuration);



	bIsAttacking = false;

	bIsRecovering = false;

	bCanCombo = false;

	bCanRollCancel = false;

	bCanMoveCancel = false;

	bHasBufferedInput = false;

	CurrentStageIndex = -1;

	CurrentComboData = nullptr;

	Mining = false;



	if (ComboGuideComponent)

	{

		ComboGuideComponent->UpdateComboGuideUI();

	}



	// 노티파이 등으로 변경되었던 충돌 무시 및 무적 상태가 있다면 공격 종료/리셋 시 안전하게 원상 복구

	if (bIsCollisionIgnoredByNotify)

	{

		SetIgnorePawnCollision(false);

	}

	if (bIsInvulnerableByNotify)

	{

		SetCanBeDamaged(true);

	}



	// 선입력된 후속 공격이 없다면 반격 슈퍼아머 해제

	if (!bShouldTriggerBufferedAttack)

	{

		bIsJustGuardCounterActive = false;

	}



	// 공격 종료 시점에 선입력된 입력이 있다면 즉시 새로운 공격 실행

	if (bShouldTriggerBufferedAttack)

	{

		ProcessAttackInput();

	}

}







void AAGSDCharacter::UpdateMotionWarpTarget()

{

	if (!MotionWarpingComponent) return;



	// 오직 현재 재생 중인 콤보 스테이지의 bUseBackwardWarp가 true로 체크된 몽타주에만 후퇴 워핑 적용

	bool bIsBackwardWarp = false;

	if (CurrentComboData && CurrentComboData->Stages.IsValidIndex(CurrentStageIndex))

	{

		bIsBackwardWarp = CurrentComboData->Stages[CurrentStageIndex].bUseBackwardWarp;

	}



	// 1. 특정 몽타주/스테이지에 후퇴 워핑(bUseBackwardWarp == true)이 설정되어 있는 경우

	if (bIsBackwardWarp)

	{

		FVector PlayerLoc = GetActorLocation();

		FVector BackwardDir = -GetActorForwardVector();



		FRotator WarpRotation = GetActorRotation();

		AActor* TargetActor = LockOnComponent ? LockOnComponent->GetLockedTarget() : nullptr;

		if (TargetActor)

		{

			FVector TargetLoc = TargetActor->GetActorLocation();

			WarpRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLoc, TargetLoc);

			WarpRotation.Pitch = 0.f;

			WarpRotation.Roll = 0.f;

		}



		FVector BackwardWarpLoc = PlayerLoc + (BackwardDir * BackwardWarpDistance);

		BackwardWarpLoc.Z = PlayerLoc.Z;



		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(

			FName("WarpTarget"),

			BackwardWarpLoc,

			WarpRotation

		);

		return;

	}



	bool bIsHardLocked = false;

	AActor* TargetActor = LockOnComponent ? LockOnComponent->GetTargetForAttack(bIsHardLocked) : nullptr;



	// 락온 대상이 없고 저스트가드 반격 콤보 진행 중이라면, 방금 튕겨낸 공격자를 모션 워프 타겟으로 지정

	if (!TargetActor && bIsJustGuardCounterActive && LastParryAttacker.IsValid())

	{

		TargetActor = LastParryAttacker.Get();

		bIsHardLocked = true; // 공격자를 향해 360도 전방향 모션 워핑 허용

	}



	// 플레이어가 마우스로 바라보고 있는 실시간 컨트롤러(카메라) 수평 회전값 및 플레이어 위치

	const FRotator ControlRot = GetControlRotation();

	const FRotator CameraFacingRotation = FRotator(0.f, ControlRot.Yaw, 0.f);

	const FVector PlayerLoc = GetActorLocation();



	if (!TargetActor)

	{

		// 타겟이 없을 때는 모션 워프 타겟을 제거하여 몽타주의 루트 모션이 온전히 동작하도록 처리

		MotionWarpingComponent->RemoveWarpTarget(FName("WarpTarget"));

		return;

	}



	FVector TargetLoc = TargetActor->GetActorLocation();



	// Rebirth 소울이 활성화되어 있다면 소울 위치로 모션 워프 타겟 지정

	TArray<USphereComponent*> SphereComps;

	TargetActor->GetComponents<USphereComponent>(SphereComps);

	for (USphereComponent* Sphere : SphereComps)

	{

		if (Sphere && Sphere->GetName().Contains(TEXT("Soul")) && Sphere->GetCollisionEnabled() != ECollisionEnabled::NoCollision)

		{

			TargetLoc = Sphere->GetComponentLocation();

			break;

		}

	}



	FVector DirToTarget = (TargetLoc - PlayerLoc).GetSafeNormal2D();



	// 소프트 락온 상태에서도 모션 워핑 시 하드 락온과 동일한 거리(MaxWarpStep)로 이동되도록 적용

	float StepLimit = MaxWarpStep;



	// 1. 시야 범위 내 타겟 각도 검사

	// 하드 락온: 타겟이 고정되어 있으므로 등 뒤를 포함한 전방향(360도) 모션 워핑 허용

	// 소프트 락온: 플레이어가 마우스로 보고 있는 실시간 카메라 정면(Yaw) 시야각 범위 내에 있을 때만 모션 워핑 적용

	if (!bIsHardLocked)

	{

		float AllowedAngle = LockOnComponent ? LockOnComponent->SoftLockMaxAngle : 30.0f;

		FVector ReferenceForward = FRotationMatrix(CameraFacingRotation).GetUnitAxis(EAxis::X);

		float Dot = FVector::DotProduct(ReferenceForward, DirToTarget);

		float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));



		if (AngleDiff > AllowedAngle)

		{

			// 마우스를 돌려 허용 시야각을 벗어났다면, 모션 워프 타겟을 제거하여 기본 루트 모션 유지

			MotionWarpingComponent->RemoveWarpTarget(FName("WarpTarget"));

			return;

		}

	}



	// 2. 플레이어 캐릭터가 타겟을 바라보는 회전값 계산

	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLoc, TargetLoc);

	TargetRotation.Pitch = 0.f;

	TargetRotation.Roll = 0.f;



	// 수평(2D) 방향 및 거리 계산

	float ActualDistance = FVector::Dist2D(PlayerLoc, TargetLoc);



	FVector WarpLocation = PlayerLoc;



	if (ActualDistance > AttackStopDistance)

	{

		// 몬스터 전방 AttackStopDistance가 되는 목표 지점 계산

		FVector DesiredWarpLoc = TargetLoc - (DirToTarget * AttackStopDistance);

		

		// 해당 목표 지점까지의 워프 필요 거리

		float WarpDist = FVector::Dist2D(PlayerLoc, DesiredWarpLoc);

		if (WarpDist > StepLimit)

		{

			// 최대 허용치만큼만 앞으로 전진 워프

			WarpLocation = PlayerLoc + (DirToTarget * StepLimit);

		}

		else

		{

			WarpLocation = DesiredWarpLoc;

		}

	}

	else

	{

		// 이미 공격 사거리 이내(AttackStopDistance 이하)에 있다면 현재 위치 유지하며 회전만 보정

		WarpLocation = PlayerLoc;

	}



	// Z축 좌표는 플레이어의 현재 높이를 유지하여 땅밑 침하 또는 공중 부양 방지

	WarpLocation.Z = PlayerLoc.Z;



	// WarpTarget 업데이트

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(

		FName("WarpTarget"),

		WarpLocation,

		TargetRotation

	);

}



void AAGSDCharacter::DoMove(float Right, float Forward)

{

	// 넉다운 또는 피격 경직 중일 때는 물리적 이동 입력 완전 무시

	if (bIsKnockdown || bIsHitReacting) return;



	if (GetController() != nullptr)

	{

		FVector ForwardDirection;

		FVector RightDirection;



		AActor* TargetActor = LockOnComponent ? LockOnComponent->GetLockedTarget() : nullptr;

		if (IsValid(TargetActor))

		{

			FVector PlayerLoc = GetActorLocation();

			FVector TargetLoc = TargetActor->GetActorLocation();

			FVector DirectionToTarget = TargetLoc - PlayerLoc;

			DirectionToTarget.Z = 0.0f;



			float CurrentDistance = FVector::Dist2D(PlayerLoc, TargetLoc);



			if (DirectionToTarget.Normalize())

			{

				ForwardDirection = DirectionToTarget;

				RightDirection = FVector::CrossProduct(FVector::UpVector, ForwardDirection);



				// 락온 중 이동 시 나선형으로 늘어나는 위치 오차 피드백 제어

				bool bIsFalling = GetCharacterMovement() && GetCharacterMovement()->IsFalling();



				if (FMath::Abs(Forward) > 0.01f || bIsFalling)

				{

					// 플레이어가 전진/후진을 입력하거나 공중에 떠 있는 상태일 때는 타겟 거리를 현재 2D 거리로 실시간 동기화 (착지 덜컹거림 방지)

					TargetLockOnDistance = CurrentDistance;

				}

				else

				{

					// 락온 거리가 초기화되지 않았다면 현재 거리로 설정

					if (TargetLockOnDistance <= 0.0f)

					{

						TargetLockOnDistance = CurrentDistance;

					}



					// 거리 편차 계산

					float DistanceError = CurrentDistance - TargetLockOnDistance;

					

					// 1.5% 비례 제어로 오차를 전진 가속도 입력에 반영 (오차 100cm당 1.5의 전진 입력)

					float CorrectionForward = DistanceError * 0.015f;

					CorrectionForward = FMath::Clamp(CorrectionForward, -1.0f, 1.0f);

					Forward = CorrectionForward;

				}

			}

			else

			{

				// Fallback if players are at the exact same location

				const FRotator Rotation = GetController()->GetControlRotation();

				const FRotator YawRotation(0, Rotation.Yaw, 0);

				ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

				RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			}

		}

		else

		{

			// find out which way is forward

			const FRotator Rotation = GetController()->GetControlRotation();

			const FRotator YawRotation(0, Rotation.Yaw, 0);



			// get forward vector

			ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);



			// get right vector 

			RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		}



		// add movement 

		AddMovementInput(ForwardDirection, Forward);

		AddMovementInput(RightDirection, Right);

	}

}



void AAGSDCharacter::DoLook(float Yaw, float Pitch)

{

	// 대화 중이거나 인벤토리, 상자 등 UI가 열려있는 동안에는 마우스 드래그에 의한 카메라 회전(화면 돌아감)을 무시합니다.

	if (IsInDialogue())

	{

		return;

	}

	if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen())

	{

		return;

	}

	if (ActiveCloseableUI.IsValid())

	{

		return;

	}

	if (OpenedChest != nullptr)

	{

		return;

	}



	if (GetController() != nullptr)

	{

		// MouseSensitivity가 5일 때 (5/5 = 1)이 되어 기존 속도와 동일해집니다.

		float SensitivityMultiplier = MouseSensitivity / 5.0f;



		AddControllerYawInput(Yaw * SensitivityMultiplier);

		AddControllerPitchInput(Pitch * SensitivityMultiplier);

	}

}



void AAGSDCharacter::DoJumpStart()

{

	// signal the character to jump

	Jump();

}



void AAGSDCharacter::DoJumpEnd()

{

	// signal the character to stop jumping

	StopJumping();

}



void AAGSDCharacter::AddDamage(float addDamage)

{

	Damage += addDamage;

	if (PlayerStateWidget)

	{

		PlayerStateWidget->SetDamageText(Damage);

	}

}



void AAGSDCharacter::PlayHealEffect()

{

	if (HealEffect)

	{

		if (bAttachHealEffectToMesh && GetMesh())

		{

			if (UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAttached(

				HealEffect,

				GetMesh(),

				NAME_None,

				FVector::ZeroVector,

				FRotator::ZeroRotator,

				EAttachLocation::SnapToTarget,

				true,

				true

			))

			{

				NC->SetRelativeScale3D(HealEffectScale);

			}

		}

		else

		{

			UNiagaraFunctionLibrary::SpawnSystemAtLocation(

				GetWorld(),

				HealEffect,

				GetActorLocation(),

				FRotator::ZeroRotator,

				HealEffectScale,

				true

			);

		}

	}



	if (HealSoundEffect)

	{

		UGameplayStatics::PlaySoundAtLocation(this, HealSoundEffect, GetActorLocation());

	}

}



void AAGSDCharacter::SetHealth(float NewHealth)

{

	Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	if (HealthBar)

	{

		HealthBar->UpdateHealth(Health, MaxHealth);

	}

}



void AAGSDCharacter::HealthRecovery(float amount)

{

	// 물약 사용 중이면 BP 고정값 대신 최대 체력 비율로 회복량 결정
	if (PotionHealing::PendingHealPercent > 0.0f)
	{
		const float PercentAmount = MaxHealth * PotionHealing::PendingHealPercent;
		UE_LOG(LogTemp, Log, TEXT("[Potion] Heal %.0f%% of MaxHealth (%.0f) = %.1f (BP flat value %.1f ignored)"),
			PotionHealing::PendingHealPercent * 100.0f, MaxHealth, PercentAmount, amount);
		amount = PercentAmount;
	}

	const float PrevHealth = Health;

	Health = FMath::Clamp(Health + amount, 0.0f, MaxHealth);

	const float ActualHealed = Health - PrevHealth;



	if (HealthBar) HealthBar->UpdateHealth(Health, MaxHealth);



	// ✨ 실제 회복 시 나이아가라 이펙트 및 사운드 재생

	if (ActualHealed > 0.0f)

	{

		PlayHealEffect();

	}



	if (UCombatTimelineLogger::IsSessionActive() && ActualHealed > 0.0f)

	{

		UCombatTimelineLogger::LogPlayerHealed(

			ActualHealed,

			GetWorld()->GetTimeSeconds(),

			Health,

			MaxHealth,

			TEXT("HealthRecovery")

		);

	}



	// 📜 실시간 체력 회복 CSV 로깅

	if (ActualHealed > 0.0f)

	{

		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				const FString HealSource = HoldingItemData.ItemName.IsEmpty() ? TEXT("HealthRecovery") : HoldingItemData.ItemName.ToString();

				LogSubsystem->LogPlayerHealed(

					GetActorLocation(),

					HealSource,

					ActualHealed,

					PrevHealth,

					Health,

					MaxHealth

				);

			}

		}

	}

}



float AAGSDCharacter::GetPotionCooldownRatio() const

{

	if (!IsPotionOnCooldown() || GlobalPotionCooldown <= 0.0f)

	{

		return 0.0f;

	}



	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	const float Elapsed = CurrentTime - LastGlobalPotionUseTime;

	const float Remaining = FMath::Max(0.0f, GlobalPotionCooldown - Elapsed);

	return FMath::Clamp(Remaining / GlobalPotionCooldown, 0.0f, 1.0f);

}



float AAGSDCharacter::GetPotionCooldownRemaining() const

{

	if (!IsPotionOnCooldown() || GlobalPotionCooldown <= 0.0f)

	{

		return 0.0f;

	}



	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	const float Elapsed = CurrentTime - LastGlobalPotionUseTime;

	return FMath::Max(0.0f, GlobalPotionCooldown - Elapsed);

}



bool AAGSDCharacter::IsPotionOnCooldown() const

{

	if (GlobalPotionCooldown <= 0.0f)

	{

		return false;

	}



	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	return (CurrentTime - LastGlobalPotionUseTime) < GlobalPotionCooldown;

}



bool AAGSDCharacter::IsPotionItem(const FStruct_ItemData& ItemData) const

{

	if (ItemData.ItemID.IsEmpty())

	{

		return false;

	}



	if (ItemData.EquipHoldingState == EHoldingState::EHS_Potion || ItemData.ItemCategory == EHoldingState::EHS_Potion)

	{

		return true;

	}



	if (ItemData.ItemID.Contains(TEXT("Health")) ||

		ItemData.ItemID.Contains(TEXT("Heal")) ||

		ItemData.ItemID.Contains(TEXT("Potion")))

	{

		return true;

	}



	return false;

}



bool AAGSDCharacter::IsCropItem(const FStruct_ItemData& ItemData) const

{

	if (ItemData.ItemID.IsEmpty())

	{

		return false;

	}



	if (ItemData.EquipHoldingState == EHoldingState::EHS_Crop ||

		ItemData.ItemCategory == EHoldingState::EHS_Crop ||

		HoldingState == EHoldingState::EHS_Crop)

	{

		return true;

	}



	if (ItemData.ItemID.Contains(TEXT("Crop")))

	{

		return true;

	}



	return false;

}



bool AAGSDCharacter::IsHoldingUsableItem(FText& OutActionText) const

{

	if (HoldingItemData.ItemID.IsEmpty())

	{

		return false;

	}



	// 1. UUsableItem 인터페이스를 구현한 클래스인지 검사

	bool bIsUsable = false;

	if (HoldingItemData.ItemBPClass)

	{

		if (AActor* DefaultActor = Cast<AActor>(HoldingItemData.ItemBPClass->GetDefaultObject()))

		{

			if (DefaultActor->Implements<UUsableItem>())

			{

				bIsUsable = true;

			}

		}

	}



	// 2. 소모품 타입(EIT_Consumable)이거나 포션 상태인 경우 사용 가능으로 판단

	if (!bIsUsable)

	{

		if (HoldingItemData.ItemType == EItemType::EIT_Consumable ||

			HoldingItemData.EquipHoldingState == EHoldingState::EHS_Potion ||

			HoldingItemData.ItemCategory == EHoldingState::EHS_Potion ||

			IsPotionItem(HoldingItemData))

		{

			bIsUsable = true;

		}

	}



	if (bIsUsable)

	{

		if (IsPotionItem(HoldingItemData))

		{

			OutActionText = FText::FromString(TEXT("마시기"));

		}

		else

		{

			OutActionText = FText::FromString(TEXT("사용하기"));

		}

		return true;

	}



	return false;

}



void AAGSDCharacter::UpdateLeftItemUseWidget()

{

	AAGSDPlayerController* PlayerController = PC ? PC : Cast<AAGSDPlayerController>(GetController());

	if (!PlayerController)

	{

		return;

	}



	// ⭐️ 인벤토리가 열려있거나, 대화 중이거나, 팝업 UI/상자가 열려있는 경우 좌측 사용하기 위젯을 숨김

	if (IsAnyUIActive())

	{

		PlayerController->HideLeftItemUseWidget();

		return;

	}



	FText ActionText;

	if (IsHoldingUsableItem(ActionText))

	{

		PlayerController->ShowLeftItemUseWidget(ActionText);

	}

	else

	{

		PlayerController->HideLeftItemUseWidget();

	}

}

//--------------



void AAGSDCharacter::SetPet(ABaseFlyingPet* NewPet)

{

	Pet = NewPet;

	bHasPet = (NewPet != nullptr);



	if (Pet)

	{

		Pet->SetOwner(this);

		Pet->ReturnToPlayer();

	}

}



bool AAGSDCharacter::IsInDialogue() const

{

	// 1. 자신의 Pet 포인터 확인

	ABaseFlyingPet* TargetPet = Pet;



	// 2. 만약 Pet이 nullptr이면 월드에서 Pet 태그를 가진 액터 검색 (맵 시작 직후 스폰 타이밍 방어)

	if (!TargetPet)

	{

		if (UWorld* World = GetWorld())

		{

			TArray<AActor*> PetActors;

			UGameplayStatics::GetAllActorsWithTag(World, FName("Pet"), PetActors);

			for (AActor* Actor : PetActors)

			{

				if (ABaseFlyingPet* FoundPet = Cast<ABaseFlyingPet>(Actor))

				{

					TargetPet = FoundPet;

					break;

				}

			}

		}

	}



	// 3. 펫의 대화 컴포넌트 활성화 여부 검사

	if (TargetPet)

	{

		if (UPetTalkComponent* TalkComp = TargetPet->GetPetTalkComponent())

		{

			if (TalkComp->IsConversationActive())

			{

				return true;

			}

		}

	}



	// 4. 튜토리얼 서브시스템의 대화 대기 상태 검사

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UTutorialSubsystem* TutSub = GameInst->GetSubsystem<UTutorialSubsystem>())

		{

			if (TutSub->IsWaitingForDialogue())

			{

				return true;

			}

		}

	}



	// 5. [철통 방어] 마우스 커서가 켜져 있는데 인벤토리/상자/기타UI가 열려있지 않다면 -> 대화 UI가 열려있는 상태로 간주

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))

	{

		if (PlayerController->bShowMouseCursor && !ActiveCloseableUI.IsValid() && !(PlayerHUDRef && PlayerHUDRef->IsInventoryOpen()) && OpenedChest == nullptr)

		{

			return true;

		}

	}



	return false;

}



bool AAGSDCharacter::IsAnyUIActive() const

{

	return IsInDialogue() ||

		(PlayerHUDRef && PlayerHUDRef->IsInventoryOpen()) ||

		ActiveCloseableUI.IsValid() ||

		(OpenedChest != nullptr);

}



void AAGSDCharacter::SetMyPet_Implementation(AActor* NewPet)

{

	SetPet(Cast<ABaseFlyingPet>(NewPet));

}



void AAGSDCharacter::MasterToPetBigConversation_Implementation(FName DialogueID)

{

	if ( Pet )

	{

		Pet->StartBigConversation(DialogueID);

	}

}



void AAGSDCharacter::DestroyPetBeforeTravel()

{

	if ( bHasPet && Pet )

	{

		Pet->Destroy();

		Pet = nullptr;

	}

}



void AAGSDCharacter::SpawnMyPetAfterTravel()

{

	if ( bHasPet )

	{

		UE_LOG(LogTemp, Warning, TEXT("SpawnMyPetAfterTravel"));

		FActorSpawnParameters SpawnParams;

		SpawnParams.Owner = this;

		SpawnParams.Instigator = GetInstigator();



		FVector SpawnLocation = GetActorLocation() + FVector(0.f, -180.f, 0.f); // 캐릭터 뒤쪽에 스폰

		FRotator SpawnRotation = FRotator::ZeroRotator;



		if (ABaseFlyingPet* SpawnedPet = GetWorld()->SpawnActor<ABaseFlyingPet>(DefaultPetClass, SpawnLocation, SpawnRotation, SpawnParams))

		{

			SetPet(SpawnedPet);

		}

	}

}



// C:\Users\YJH\Documents\GitHub\AGSD\Source\AGSD\AGSDCharacter.cpp



void AAGSDCharacter::HandleEnemyDeadAndDropItem_Implementation(AActor* DeadActor)

{

	// 1. 퀘스트 / 튜토리얼 서브시스템에 몬스터 처치 보고 (아이템 드롭 테이블 유무와 상관없이 항상 실행)

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UTutorialSubsystem* TutSub = GameInst->GetSubsystem<UTutorialSubsystem>())

		{

			TutSub->ReportMonsterDefeated(DeadActor);

		}

	}



	if (DeadActor && DeadActor->Implements<UItemDropInterface>())

	{

		// 1. 죽은 에너미에게서 고유 ID 받아오기 (데이터 테이블 Row Name)

		FName EnemyID = IItemDropInterface::Execute_GetItemDropTableEnemyID(DeadActor);

		

		// 데이터 테이블이 할당 안되거나 게임 인스턴스 할당이 없으면 종료

		if (!EnemyDropDataTable || !GI) return;



		// 2. 에너미 ID(Row Name)를 기반으로 해당 에너미의 모든 드롭 배열 가져오기

		FEnemyDropData* DropData = EnemyDropDataTable->FindRow<FEnemyDropData>(EnemyID, TEXT("EnemyDropInfo"));

		if (!DropData) return;



		// 3. 에너미가 설정한 아이템 배열을 순회

		for (const FEnemyDropItemInfo& ItemInfo : DropData->DropItems)

		{

			// 스폰할 클래스가 비어있으면 패스

			if (!ItemInfo.DropItemClass) continue;



			// 드롭 확률 체크 (0.0 ~ 1.0 사이의 값으로, 1.0이면 무조건 드롭, 0.5면 50% 확률로 드롭)

			if (FMath::FRand() > ItemInfo.DropChance) continue;

			

			// 4. 최초 1회성 드롭 여부 확인

			if (ItemInfo.bIsOneTimeDrop == true)

			{

				FString UniqueKey = EnemyID.ToString() + TEXT("_") + ItemInfo.DropItemClass->GetName();

				if (GI->AlreadyDroppedItems.Contains(UniqueKey))

				{

					continue;

				}

				else

				{

					GI->AlreadyDroppedItems.Add(UniqueKey);

				}

			}



			// --- 추가/수정된 로직: 드롭 개수 결정 ---

			int32 FinalDropAmount = ItemInfo.DropAmount;

			if (ItemInfo.bUseRandomDropAmount)

			{

				// Min과 Max 사이에서 무작위 개수 결정 (Min, Max 모두 포함)

				FinalDropAmount = FMath::RandRange(ItemInfo.DropAmountMin, ItemInfo.DropAmountMax);

			}



			if (FinalDropAmount <= 0) continue;



			TSubclassOf<AActor> ItemClassToSpawn = ItemInfo.DropItemClass;

			int32 RemainingToSpawn = FinalDropAmount;



			// 5. 자동 루팅(Auto-Loot) 시도: 드롭 아이템이 APickUpItem 파생 클래스인 경우 플레이어 인벤토리에 즉시 자동 투입

			if (InventoryComponent && ItemClassToSpawn && ItemClassToSpawn->IsChildOf(APickUpItem::StaticClass()))

			{

				if (const APickUpItem* DefaultItem = ItemClassToSpawn->GetDefaultObject<APickUpItem>())

				{

					FString TargetItemID = DefaultItem->GetItemID();

					if (!TargetItemID.IsEmpty())

					{

						int32 OutRemainingQty = 0;

						FStruct_ItemData OutItemData;

						bool bAdded = InventoryComponent->AddItemByID(TargetItemID, FinalDropAmount, OutRemainingQty, OutItemData);

						if (bAdded)

						{

							int32 AcquiredQty = FinalDropAmount - OutRemainingQty;

							RemainingToSpawn = OutRemainingQty; // 가방이 가득 찬 경우 못 들어간 잔여 수량만 월드 스폰



							// 플레이어 HUD에 아이템 획득 알림 팝업 출력

							if (PlayerHUDRef)

							{

								PlayerHUDRef->AddItemNotification(OutItemData, AcquiredQty);

							}



							// 아이템 획득 TextLog 기록

							UTextLog::WriteTextLogByStringAndFloat(TEXT("아이템 획득"), OutItemData.ItemName.ToString(), static_cast<float>(AcquiredQty));



							// ─── [플레이어 아이템 파밍 실시간 CSV 로그 기록 (몬스터 드롭)] ───

							if (UGameInstance* GameInst = GetGameInstance())

							{

								if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

								{

									const FString DeadActorName = DeadActor ? DeadActor->GetName() : TEXT("Enemy");

									LogSubsystem->LogPlayerItemLooted(

										GetActorLocation(),

										TEXT("MonsterDrop"),

										DeadActorName,

										TargetItemID,

										OutItemData.ItemName.ToString(),

										AcquiredQty

									);

								}

							}

						}

					}

				}

			}



			// 6. 인벤토리가 가득 찼거나 비-PickUpItem 액터인 경우, 남은 수량(RemainingToSpawn)만큼 월드 바닥에 스폰 (Fallback 안전망)

			if (RemainingToSpawn > 0)

			{

				for (int32 i = 0; i < RemainingToSpawn; ++i)

				{

					// 적의 현재 위치

					FVector SpawnLocation = DeadActor->GetActorLocation();



					// 캐릭터 발 밑 근처에서 약간의 랜덤 위치 오프셋을 주면 아이템들이 겹치지 않아 보기 좋습니다.

					FVector RandomOffset = FVector(FMath::RandRange(-20.f, 20.f), FMath::RandRange(-20.f, 20.f), 0.f);

					SpawnLocation += RandomOffset;



					FActorSpawnParameters SpawnParams;

					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;



					GetWorld()->SpawnActor<AActor>(ItemClassToSpawn, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

				}

			}

		}

	}

}



void AAGSDCharacter::StopAnimMontage(UAnimMontage* AnimMontage)

{

	// 언리얼 엔진 기본 블렌드아웃(몽타주 설정 시간, 약 0.25초)을 적용하여 걷기/달리기/구르기와 자연스럽게 보간

	Super::StopAnimMontage(AnimMontage);

	

	// 1. 선입력 버퍼를 비워 ResetAttackState() 내의 즉시 공격 트리거 방지

	bHasBufferedInput = false;



	// 2. 공격 상태 및 콤보 데이터 초기화

	ResetAttackState();

	ResetCombo();



	// 3. 모션워핑 타겟 해제하여 의도치 않은 전진 보정 방지

	if (MotionWarpingComponent)

	{

		MotionWarpingComponent->RemoveWarpTarget(FName("WarpTarget"));

	}

}



void AAGSDCharacter::StopAnimMontageImmediate(float BlendOutTime)

{

	// 피격 넉백 상황 전용: 블렌드아웃 대기 없이 모든 몽타주와 루트 모션을 즉시 강제 정지

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)

	{

		AnimInstance->StopAllMontages(BlendOutTime);

	}



	Super::StopAnimMontage(nullptr);

	

	bHasBufferedInput = false;

	ResetAttackState();

	ResetCombo();



	if (MotionWarpingComponent)

	{

		MotionWarpingComponent->RemoveWarpTarget(FName("WarpTarget"));

	}

}



void AAGSDCharacter::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)

{

	// 에어본 넉다운 몽타주 재생 중의 런치(보스 돌진 등 데미지 직후 밀치기):
	// 몽타주를 정지하면 녹다운 모션이 끊기므로 유지하고, 공중 감속만 꺼서 수평으로 날아가게 함
	const bool bInAirborneKnockdown = bIsKnockdown && CurrentKnockdownType == EKnockdownType::AirborneFloor && bIsAirborneMontageActive;

	if (bInAirborneKnockdown && LaunchVelocity.SizeSquared() > 0.f)
	{
		SuspendFallingBrakingForKnockback();
		Super::LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
		UE_LOG(LogTemp, Log, TEXT("[Airborne] Launch during airborne knockdown (montage kept): %s"), *LaunchVelocity.ToString());
		return;
	}

	// 피격 넉백으로 캐릭터가 튕겨 나갈 때, 공격 루트 모션이나 모션 워핑이 넉백 궤적을 덮어쓰지 않도록 즉시 정지

	if (LaunchVelocity.SizeSquared() > 0.f)

	{

		StopAnimMontageImmediate(0.0f);

	}



	Super::LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);

}



//--------------



void AAGSDCharacter::HandleLockOn(bool bLockOn)

{

	if (PC)

	{

		UpdateCharacterRotationSettings();



		if (!bLockOn)

		{

			UE_LOG(LogTemp, Warning, TEXT("LockOn False - Releasing LockOn"));

			if (LockOnComponent && LockOnComponent->IsTargetLocked())

			{

				LockOnComponent->ToggleLockOn();

			}

		}

	}

}



void AAGSDCharacter::SkipTutorialPressed()

{

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);

	if (TutorialMapNames.Contains(CurrentLevelName))

	{

		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->IncrementTutorialFullSkip();

			}

		}

		OnSkipTutorialTriggered();

	}

}



void AAGSDCharacter::OnSkipTutorialTriggered_Implementation()

{

	// 페이드 아웃 시작 (투명도 0.0 -> 1.0)

	playFadeWidget(0.0f, 1.0f, 6.0f);



	if (FadeWidget)

	{

		// 중복 바인딩 방지 후 페이드 아웃 완료 이벤트 바인딩

		FadeWidget->OnFadeFinished.RemoveDynamic(this, &AAGSDCharacter::ExecuteTutorialSkipLevelTransition);

		FadeWidget->OnFadeFinished.AddDynamic(this, &AAGSDCharacter::ExecuteTutorialSkipLevelTransition);

	}

	else

	{

		// 페이드 위젯이 유효하지 않을 경우 즉시 이동 (예외 처리)

		ExecuteTutorialSkipLevelTransition();

	}

}



void AAGSDCharacter::ExecuteTutorialSkipLevelTransition()

{

	if (FadeWidget)

	{

		// 이벤트 바인딩 해제

		FadeWidget->OnFadeFinished.RemoveDynamic(this, &AAGSDCharacter::ExecuteTutorialSkipLevelTransition);

	}



	// 튜토리얼 스킵 시 펫 획득 맵(Tutorial_Sky_Island)을 건너뛰므로 펫 보유 상태를 직접 지급합니다.
	// 거점 레벨 블루프린트는 bHasPet이 true일 때만 거점 튜토리얼을 시작하고, 펫 대화도 펫이 있어야 진행됩니다.
	// (EndPlay에서 캐릭터의 bHasPet이 GameInstance로 복사되고, 다음 맵 BeginPlay에서 SpawnMyPetAfterTravel로 펫이 스폰됨)
	bHasPet = true;
	if (USOVGameInstance* SkipGI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		SkipGI->bHasPet = true;
	}



	// 블루프린트에서 타겟 맵 이름이 설정되어 있다면 해당 맵으로 이동

	if (TargetSkipMapName != NAME_None)

	{

		UGameplayStatics::OpenLevel(this, TargetSkipMapName);

	}

	else

	{

		// 설정되지 않았을 경우 기본값

		UGameplayStatics::OpenLevel(this, FName("Farm_Sky_Island"));

	}

}



void AAGSDCharacter::SetHoldingActorHidden(bool bNewHidden)

{

	if (IsValid(HoldingActor))

	{

		HoldingActor->SetActorHiddenInGame(bNewHidden);

	}



	// 손에 든 무기를 숨길 때 등 뒤에 거치된 무기들도 함께 원자적으로 숨김/복원

	SetBackWeaponHidden(bNewHidden);

}



void AAGSDCharacter::SetBackWeaponHidden(bool bNewHidden)

{

	bBackWeaponsHidden = bNewHidden;



	for (auto& Pair : BackWeaponActors)

	{

		if (IsValid(Pair.Value))

		{

			Pair.Value->SetActorHiddenInGame(bNewHidden);

		}

	}

}



// ═══════════════════════════════════════════════════

// 인벤토리 - 장착 액터 갱신

// ═══════════════════════════════════════════════════



void AAGSDCharacter::OnHotbarSelectionChanged(int32 PreviousIndex, int32 NewIndex)

{

	// [핫바 선입력]: 행동(공격/구르기/피격/넉다운/상호작용) 중에는 핫바 하이라이트만 이동하고
	// 실제 장착 교체는 행동 종료 시 Tick에서 적용 (공격 모션 도중 무기가 바뀌는 문제 방지)
	if (bIsAttacking || bIsRecovering || bIsRolling || bIsHitReacting || bIsKnockdown || Mining)
	{
		UE_LOG(LogTemp, Log, TEXT("[Hotbar] Selection %d -> %d buffered (player busy)"), PreviousIndex, NewIndex);
		return;
	}

	UpdateEquippedActor();

}



void AAGSDCharacter::OnInventorySlotUpdated(int32 SlotIndex)

{

	// 인벤토리 상태 변경에 따라 등 뒤 무기를 먼저 업데이트하여 에셋을 정상 스폰 및 동기화합니다.

	UpdateBackWeapon();



	// 현재 선택된 핫바 슬롯의 데이터가 변경된 경우 장착 아이템 갱신

	if (InventoryComponent && SlotIndex == InventoryComponent->GetCurrentHotbarIndex())

	{

		// 행동 중이면 장착 갱신을 미룸 (Tick에서 행동 종료 시 적용)
		if (bIsAttacking || bIsRecovering || bIsRolling || bIsHitReacting || bIsKnockdown || Mining)
		{
			return;
		}

		UpdateEquippedActor();

	}

}



void AAGSDCharacter::UpdateEquippedActor()

{

	if (!InventoryComponent) return;



	const FStruct_ItemData NewItemData = InventoryComponent->GetCurrentHotbarItemData();



	// 같은 아이템이면 갱신 불필요

	if (HoldingItemData.ItemID == NewItemData.ItemID && !NewItemData.ItemID.IsEmpty())

	{

		return;

	}



	// 장착 아이템이 교체되는데 가드(Block) 상태라면 가드를 먼저 해제하여 내부 플래그를 정상 정리

	if (CharacterState == ECharacterState::Block && GuardComponent)

	{

		GuardComponent->StopBlock();

	}



	// 1. 기존 장착 액터 파괴

	if (IsValid(HoldingActor))

	{

		HoldingActor->Destroy();

		HoldingActor = nullptr;

	}



	// 2. 새 아이템 데이터 갱신

	HoldingItemData = NewItemData;



	// 빈 슬롯이면 장착 해제 상태

	if (NewItemData.ItemID.IsEmpty())

	{

		HoldingState = EHoldingState::EHS_None;

		HoldingWeapon = EHoldingWeapon::None;

		UpdateBackWeapon();

		UpdateCharacterStateFromEquip();



		if (LockOnComponent && LockOnComponent->IsTargetLocked())

		{

			LockOnComponent->ToggleLockOn();

		}

		UpdateLeftItemUseWidget();

		return;

	}



	// 아이템 ID에 따른 부착 소켓 및 장착 무기 상태 결정

	FName AttachSocketName = NAME_None;

	EHoldingWeapon TargetHoldingWeapon = EHoldingWeapon::None;



	// 방법 1 적용: 장착 상태가 포션(EHS_Potion)이면 ID와 무관하게 무조건 PotionSocket에 부착

	if (NewItemData.EquipHoldingState == EHoldingState::EHS_Potion)

	{

		AttachSocketName = FName("PotionSocket");

		TargetHoldingWeapon = EHoldingWeapon::Potion;

	}

	else

	{

		FString TargetItemID = NewItemData.ItemID.ToLower();



		// 설정된 맵핑 배열을 순회하며 조건 체크 (ID 기반 무기/도구)

		for (const FEquipSocketMapping& Mapping : EquipSocketMappings)

		{

			FString MapItemID = Mapping.ItemID.ToLower();



			if (Mapping.bContainsCheck)

			{

				if (TargetItemID.Contains(MapItemID))

				{

					AttachSocketName = Mapping.SocketName;

					TargetHoldingWeapon = Mapping.HoldingWeaponState;

					break;

				}

			}

			else

			{

				if (TargetItemID == MapItemID)

				{

					AttachSocketName = Mapping.SocketName;

					TargetHoldingWeapon = Mapping.HoldingWeaponState;

					break;

				}

			}

		}

	}



	// 3. 상태 갱신

	HoldingState = NewItemData.EquipHoldingState;

	HoldingWeapon = TargetHoldingWeapon;



	// HoldingWeapon이 Spear가 아니면 기존 LockOn 해제

	if (HoldingWeapon != EHoldingWeapon::Spear && LockOnComponent && LockOnComponent->IsTargetLocked())

	{

		LockOnComponent->ToggleLockOn();

	}



	// 부착할 액터가 없는 아이템은 여기서 장착 상태만 유지한 채 안전하게 종료

	if (AttachSocketName == NAME_None)

	{

		UpdateBackWeapon();

		UpdateCharacterStateFromEquip();

		UpdateLeftItemUseWidget();

		return;

	}



	// 4. 새 액터 스폰 및 소켓 부착 (Forke는 이미 UpdateBackWeapon에서 스폰되었으므로 스폰 제외하고 기존 액터 이동)

	if (NewItemData.ItemID.ToLower() == TEXT("forke"))

	{

		if (BackWeaponActors.Contains(EHoldingWeapon::Spear) && IsValid(BackWeaponActors[EHoldingWeapon::Spear]))

		{

			HoldingActor = BackWeaponActors[EHoldingWeapon::Spear];

			BackWeaponActors.Remove(EHoldingWeapon::Spear);



			if (HoldingActor)

			{

				HoldingActor->AttachToComponent(

					GetMesh(),

					FAttachmentTransformRules::SnapToTargetNotIncludingScale,

					AttachSocketName

				);

			}

		}

	}

	else if (NewItemData.ItemBPClass)

	{

		UWorld* World = GetWorld();

		if (World)

		{

			FActorSpawnParameters SpawnParams;

			SpawnParams.Owner = this;

			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;



			HoldingActor = World->SpawnActor<AActor>(NewItemData.ItemBPClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);



			if (HoldingActor)

			{

				// APickUpItem 타입인 경우, 손에 들렸을 때의 콜리전/물리 끄기 함수 호출

				if (APickUpItem* PickUpItem = Cast<APickUpItem>(HoldingActor))

				{

					PickUpItem->DisableCollisionForHolding();

				}



				// 물리 시뮬레이션 및 충돌로 인해 소켓 부착이 해제되거나 캐릭터가 밀리는 현상 방지

				if (USceneComponent* RootComp = HoldingActor->GetRootComponent())

				{

					RootComp->SetMobility(EComponentMobility::Movable);

				}



				TArray<UPrimitiveComponent*> PrimitiveComponents;

				HoldingActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

				for (UPrimitiveComponent* PrimComp : PrimitiveComponents)

				{

					if (PrimComp)

					{

						PrimComp->SetSimulatePhysics(false);

						PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

					}

				}



				HoldingActor->AttachToComponent(

					GetMesh(),

					FAttachmentTransformRules::SnapToTargetNotIncludingScale,

					AttachSocketName

				);

			}

		}

	}



	// 손 장착 무기 상태가 변경되었으므로 등 뒤 무기도 갱신

	UpdateBackWeapon();

	UpdateCharacterStateFromEquip();

	UpdateLeftItemUseWidget();

}



void AAGSDCharacter::Input_HotbarScroll(const FInputActionValue& Value)

{

	// 넉다운/피격 중에도 핫바 선택은 받고, 실제 장착은 행동 종료 후 적용 (선입력 - OnHotbarSelectionChanged/Tick 참고)

	float ScrollValue = Value.Get<float>();

	if (FMath::IsNearlyZero(ScrollValue)) return;



	if (InventoryComponent)

	{

		// 휠을 아래로 내릴 때(ScrollValue < 0) 슬롯 번호가 올라가도록(Forward=true) 수정

		InventoryComponent->CycleHotbar(ScrollValue < 0.0f);

	}

}



void AAGSDCharacter::Input_SelectHotbar(const FInputActionValue& Value)

{

	float KeyValue = Value.Get<float>();



	// 입력이 0이거나 거의 없는 경우(키를 뗐을 때) 이전 입력값을 리셋합니다.

	if (FMath::IsNearlyZero(KeyValue))

	{

		LastHotbarInputIndex = -1;

		return;

	}



	// 넉다운/피격 중에도 핫바 선택은 받고, 실제 장착은 행동 종료 후 적용 (선입력 - OnHotbarSelectionChanged/Tick 참고)

	int32 TargetIndex = FMath::RoundToInt(KeyValue);



	// 입력 인덱스 보정 (1~9는 0~8로 변환, 10 또는 0은 9번 핫바 슬롯으로 매핑)

	if (TargetIndex >= 1 && TargetIndex <= 9)

	{

		TargetIndex = TargetIndex - 1;

	}

	else if (TargetIndex == 10 || TargetIndex == 0)

	{

		TargetIndex = 9;

	}



	// [중복 방지] 이미 처리된 키 입력이라면 무시 (한 번 누를 때 한 번만 실행)

	if (TargetIndex == LastHotbarInputIndex)

	{

		return;

	}



	// 상태 업데이트

	LastHotbarInputIndex = TargetIndex;



	if (InventoryComponent)

	{

		// [스왑 기능] 

		bool bSwapped = false;

		if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen() && InventoryComponent->HoveredSlotIndex != -1)

		{

			InventoryComponent->SwapSlots(TargetIndex, InventoryComponent->HoveredSlotIndex);

			bSwapped = true;

		}

		

		// 상자가 열려있는 경우의 스왑 처리 (상자 슬롯 또는 플레이어 가방 슬롯 대상)

		if (!bSwapped && OpenedChest && OpenedChest->GetInventoryComponent())

		{

			UAGSDInventoryComponent* ChestInv = OpenedChest->GetInventoryComponent();

			if (InventoryComponent->HoveredSlotIndex != -1)

			{

				InventoryComponent->SwapSlots(TargetIndex, InventoryComponent->HoveredSlotIndex);

			}

			else if (ChestInv->HoveredSlotIndex != -1)

			{

				UAGSDInventoryComponent::CrossInventorySwap(InventoryComponent, TargetIndex, ChestInv, ChestInv->HoveredSlotIndex);

			}

		}

		

		// 항상 해당 슬롯을 선택 상태로 변경

		InventoryComponent->SelectHotbar(TargetIndex);

	}

}



FString AAGSDCharacter::SubItemAmount()

{

	if (!InventoryComponent)

	{

		UE_LOG(LogTemp, Warning, TEXT("SubItemAmount: 인벤토리 컴포넌트가 존재하지 않습니다."));

		return FString();

	}



	int32 HotbarIndex = InventoryComponent->GetCurrentHotbarIndex();

	FStruct_InventorySlotData SlotData = InventoryComponent->GetSlotData(HotbarIndex);



	if (SlotData.IsEmpty || SlotData.ItemData.ItemID.IsEmpty())

	{

		return FString();

	}



	FString ConsumedItemID = SlotData.ItemData.ItemID;

	InventoryComponent->RemoveItem(HotbarIndex, 1);



	return ConsumedItemID;

}





void AAGSDCharacter::Input_ToggleInventory()

{

	// 현재 드래그 중인 상태라면 인벤토리 닫기를 무시합니다.

	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().IsDragDropping())

	{

		if (PlayerHUDRef && PlayerHUDRef->IsInventoryOpen())

		{

			return;

		}

	}



	// 상자 등이 열려있어 bCanOpenChest가 false이고, 인벤토리 UI가 현재 열려있지 않은 경우 인벤토리 열기를 차단합니다.

	if (!bCanOpenChest && PlayerHUDRef && !PlayerHUDRef->IsInventoryOpen())

	{

		return;

	}



	if (PlayerHUDRef)

	{

		PlayerHUDRef->ToggleInventory();

		UE_LOG(LogTemp, Log, TEXT("AAGSDCharacter::Input_ToggleInventory - Inventory Toggle Successful."));

	}

	else

	{

		UE_LOG(LogTemp, Warning, TEXT("AAGSDCharacter::Input_ToggleInventory - PlayerHUDRef is invalid."));

	}

}



void AAGSDCharacter::Input_Pause()

{

	// 현재 드래그 중인 상태라면 UI 닫기 처리를 무시합니다.

	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().IsDragDropping())

	{

		return;

	}



	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (!PlayerController) return;



	// 1. 활성화된 닫기 가능 UI가 있을 경우 인터페이스를 통해 닫기

	if (ActiveCloseableUI.IsValid() && ActiveCloseableUI->GetClass()->ImplementsInterface(UUIClosable::StaticClass()))

	{

		IUIClosable::Execute_CloseUI(ActiveCloseableUI.Get());

		return;

	}



	// 2. 활성화된 UI가 없을 경우 기존 일시정지 로직 수행

	if (bCanOpenChest)

	{

		if (PauseMenuClass)

		{

			if (!PauseMenuWidgetRef)

			{

				PauseMenuWidgetRef = CreateWidget<UUserWidget>(PlayerController, PauseMenuClass);

			}



			if (PauseMenuWidgetRef)

			{

				if (!PauseMenuWidgetRef->IsInViewport())

				{

					PauseMenuWidgetRef->AddToViewport();

				}



				PlayerController->SetShowMouseCursor(true);



				FInputModeGameAndUI InputMode;

				InputMode.SetWidgetToFocus(PauseMenuWidgetRef->TakeWidget());

				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

				PlayerController->SetInputMode(InputMode);



				UGameplayStatics::SetGamePaused(GetWorld(), true);



				if (UGameInstance* GameInst = GetGameInstance())

				{

					if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

					{

						LogSubsystem->IncrementPauseCount();

					}

				}

			}

		}

	}

}



void AAGSDCharacter::RemovePauseUI()

{

	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (!PlayerController) return;



	if (PauseMenuWidgetRef)

	{

		if (PauseMenuWidgetRef->IsInViewport())

		{

			PauseMenuWidgetRef->RemoveFromParent();

		}

		PauseMenuWidgetRef = nullptr;

	}



	PlayerController->SetShowMouseCursor(false);



	FInputModeGameOnly InputMode;

	PlayerController->SetInputMode(InputMode);



	UGameplayStatics::SetGamePaused(GetWorld(), false);

}



void AAGSDCharacter::RegisterCloseableUI(UUserWidget* NewUI)

{

	if (NewUI && NewUI->GetClass()->ImplementsInterface(UUIClosable::StaticClass()))

	{

		ActiveCloseableUI = NewUI;



		// 📜 UI 이름 및 클래스에 따라 행동 카테고리 자동 지정

		FString UIName = NewUI->GetName();

		if (UIName.Contains(TEXT("Pause")))

		{

			SetCurrentActionCategory(TEXT("PauseMenu"));

		}

		else if (UIName.Contains(TEXT("Alchemy")) || UIName.Contains(TEXT("Potion")))

		{

			SetCurrentActionCategory(TEXT("PotionCrafting"));

		}

		else if (UIName.Contains(TEXT("Tribute")) || UIName.Contains(TEXT("Altar")))

		{

			SetCurrentActionCategory(TEXT("TributeAltar"));

		}

		else if (UIName.Contains(TEXT("Chest")) || UIName.Contains(TEXT("Inventory")))

		{

			SetCurrentActionCategory(TEXT("Inventory"));



			if (UWorld* World = GetWorld())

			{

				FString MapName = World->GetMapName();

				MapName.RemoveFromStart(World->StreamingLevelsPrefix);

				if (UGameInstance* GameInst = GetGameInstance())

				{

					if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

					{

						LogSubsystem->IncrementInventoryOpenCount(MapName);

					}

				}

			}

		}

		else

		{

			// 위 특수 UI 외의 모든 팝업/상점/대장간/요리/NPC UI는 'NPC_Service'로 자동 배정!

			SetCurrentActionCategory(TEXT("NPC_Service"));

		}



		UE_LOG(LogTemp, Log, TEXT("RegisterCloseableUI - Successfully registered: %s (ActionCategory: %s)"), *NewUI->GetName(), *CurrentActionCategory);



		// ⭐️ 팝업 UI가 열리면 아이템 사용 안내 및 상호작용 안내 위젯 즉시 숨김

		if (AAGSDPlayerController* PlayerController = PC ? PC : Cast<AAGSDPlayerController>(GetController()))

		{

			PlayerController->HideLeftItemUseWidget();

			PlayerController->HideInteractionWidget();

		}

	}

}



void AAGSDCharacter::UnregisterCloseableUI(UUserWidget* UI)

{

	if (ActiveCloseableUI.Get() == UI)

	{

		ActiveCloseableUI = nullptr;

		SetCurrentActionCategory(TEXT(""));

		UE_LOG(LogTemp, Log, TEXT("UnregisterCloseableUI - Successfully unregistered"));



		// ⭐️ UI가 닫히면 손에 든 아이템에 맞춰 좌측 위젯 복원

		UpdateLeftItemUseWidget();

	}

}



void AAGSDCharacter::UpdateCharacterRotationSettings()

{

	if (!GetCharacterMovement()) return;



	bIsLockedOn = LockOnComponent && LockOnComponent->IsTargetLocked();



	if (bIsLockedOn || bIsFaceCameraPressed || CharacterState == ECharacterState::Block)

	{

		GetCharacterMovement()->bOrientRotationToMovement = false;



		// 이동 입력(WASD)이 있거나 실제로 걷는 중일 때는 적/카메라를 정면으로 바라보며 걷도록 회전 활성화

		// 가만히 서 있을 때는 회전을 꺼서 AimOffset(0~90도)과 제자리 턴(90도 이상)이 작동하도록 지원

		bool bIsMovingOrInput = (GetCharacterMovement()->Velocity.SizeSquared2D() > 100.0f) || !LastRawInputVector.IsNearlyZero();

		GetCharacterMovement()->bUseControllerDesiredRotation = bIsMovingOrInput && (bIsLockedOn || bIsFaceCameraPressed);

	}

	else

	{

		GetCharacterMovement()->bOrientRotationToMovement = true;

		GetCharacterMovement()->bUseControllerDesiredRotation = false;

	}

}



void AAGSDCharacter::TryStartTurn()

{

	bIsLockedOn = LockOnComponent && LockOnComponent->IsTargetLocked();



	// 락온 중이거나, 이미 턴 중이거나, 공격/스킬 모션 중일 때는 패스

	if (bIsLockedOn || bIsTurning || bIsAttacking || SkillMotion)

	{

		return;

	}



	// FaceCamera 조준 키를 누르지 않았고, 동시에 가드(Block) 상태도 아니라면 패스 (가드 중이거나 조준 키를 누른 경우 턴 허용)

	if (!bIsFaceCameraPressed && CharacterState != ECharacterState::Block)

	{

		return;

	}



	// 캐릭터가 이동 중일 때는 제자리 턴을 하지 않고 움직임 회전을 따르도록 패스

	if (GetCharacterMovement() && GetCharacterMovement()->Velocity.SizeSquared2D() > 100.0f)

	{

		return;

	}



	float AbsYaw = FMath::Abs(TurnYawDelta);

	if (AbsYaw >= TurnThresholdAngle)

	{

		bIsTurning = true;

		StartRotation = GetActorRotation(); // 턴 시작 각도 기록

		TurnTimer = TurnDuration; // 블루프린트에서 수정한 회전 지속 시간 적용

	}

}



void AAGSDCharacter::UpdateBackWeapon()

{

	if (!InventoryComponent) return;



	UWorld* World = GetWorld();

	if (!World) return;



	// 인벤토리의 모든 슬롯 데이터를 가져옴

	const TArray<FStruct_InventorySlotData>& InventorySlots = InventoryComponent->GetAllSlots();



	// 등 뒤 거치 상태를 결정하기 위해 현재 인벤토리에 어떤 아이템 ID들이 있는지 수집

	TSet<FString> InventoryItemIDs;

	for (const FStruct_InventorySlotData& Slot : InventorySlots)

	{

		if (!Slot.ItemData.ItemID.IsEmpty() && Slot.ItemData.CurrentQuantity > 0)

		{

			InventoryItemIDs.Add(Slot.ItemData.ItemID.ToLower());

		}

	}



	// 매핑 설정을 순회하며 등 뒤에 메야 하는 각 무기류의 스폰/해제 결정

	for (const FEquipSocketMapping& Mapping : EquipSocketMappings)

	{

		// 등 뒤 소켓 이름이 지정되지 않은 경우 패스

		if (Mapping.BackSocketName == NAME_None)

		{

			continue;

		}



		FString MapItemID = Mapping.ItemID.ToLower();

		EHoldingWeapon WeaponType = Mapping.HoldingWeaponState;



		// 인벤토리에 해당 무기 ID가 존재하는지 체크

		bool bHasInInventory = false;

		FStruct_ItemData FoundItemData;



		if (Mapping.bContainsCheck)

		{

			for (const FString& InvItemID : InventoryItemIDs)

			{

				if (InvItemID.Contains(MapItemID))

				{

					bHasInInventory = true;

					// 실제 아이템 데이터 탐색

					for (const FStruct_InventorySlotData& Slot : InventorySlots)

					{

						if (Slot.ItemData.ItemID.ToLower().Contains(MapItemID))

						{

							FoundItemData = Slot.ItemData;

							break;

						}

					}

					break;

				}

			}

		}

		else

		{

			if (InventoryItemIDs.Contains(MapItemID))

			{

				bHasInInventory = true;

				// 실제 아이템 데이터 탐색

				for (const FStruct_InventorySlotData& Slot : InventorySlots)

				{

					if (Slot.ItemData.ItemID.ToLower() == MapItemID)

					{

						FoundItemData = Slot.ItemData;

						break;

					}

				}

			}

		}



		// 손에 장착하고 있는 상태인지 여부 체크

		// 현재 쥐고 있는 아이템 ID가 이 매핑의 ID와 일치(또는 포함)하는지 확인

		bool bIsCurrentlyHolding = false;

		if (HoldingWeapon == WeaponType)

		{

			bIsCurrentlyHolding = true;

		}



		// 조건 A: 인벤토리에 있고, 손에 쥐고 있지 않을 때 -> 등 뒤에 거치되어야 함

		if (bHasInInventory && !bIsCurrentlyHolding)

		{

			// 아직 액터가 스폰되지 않았다면 스폰 후 부착

			if (!BackWeaponActors.Contains(WeaponType) || !IsValid(BackWeaponActors[WeaponType]))

			{

				if (FoundItemData.ItemBPClass)

				{

					FActorSpawnParameters SpawnParams;

					SpawnParams.Owner = this;

					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;



					AActor* NewBackActor = World->SpawnActor<AActor>(FoundItemData.ItemBPClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

					if (NewBackActor)

					{

						// 물리 및 콜리전 비활성화 (손에 들었을 때와 유사)

						if (APickUpItem* PickUpItem = Cast<APickUpItem>(NewBackActor))

						{

							PickUpItem->DisableCollisionForHolding();

						}



						if (USceneComponent* RootComp = NewBackActor->GetRootComponent())

						{

							RootComp->SetMobility(EComponentMobility::Movable);

						}



						TArray<UPrimitiveComponent*> PrimitiveComponents;

						NewBackActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

						for (UPrimitiveComponent* Comp : PrimitiveComponents)

						{

							if (Comp)

							{

								Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

								Comp->SetSimulatePhysics(false);

							}

						}



						// 등 뒤 소켓에 부착

						FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);

						NewBackActor->AttachToComponent(GetMesh(), AttachRules, Mapping.BackSocketName);



						// 현재 등 뒤 무기가 숨김 상태라면 새로 스폰된 액터도 숨김 처리

						NewBackActor->SetActorHiddenInGame(bBackWeaponsHidden);



						BackWeaponActors.Add(WeaponType, NewBackActor);

					}

				}

			}

		}

		// 조건 B: 인벤토리에 없거나, 손에 쥐고 있을 때 -> 등 뒤에서 떼어내야 함

		else

		{

			// 액터가 스폰되어 있다면 파괴 및 맵에서 제거

			if (BackWeaponActors.Contains(WeaponType))

			{

				AActor* ExistentActor = BackWeaponActors[WeaponType];

				if (IsValid(ExistentActor))

				{

					ExistentActor->Destroy();

				}

				BackWeaponActors.Remove(WeaponType);

			}

		}

	}

}



void AAGSDCharacter::SetCharacterState(ECharacterState NewState)

{

	if (CharacterState == NewState) return;



	ECharacterState OldState = CharacterState;

	CharacterState = NewState;



	FString OldStateStr = OldState == ECharacterState::Idle ? TEXT("Idle") : (OldState == ECharacterState::Combat ? TEXT("Combat") : TEXT("Block"));

	FString NewStateStr = NewState == ECharacterState::Idle ? TEXT("Idle") : (NewState == ECharacterState::Combat ? TEXT("Combat") : TEXT("Block"));



	UE_LOG(LogTemp, Log, TEXT("Character State Changed: %s -> %s"), *OldStateStr, *NewStateStr);

}



void AAGSDCharacter::UpdateCharacterStateFromEquip()

{

	if (HoldingWeapon == EHoldingWeapon::Spear)

	{

		ReportTutorialAction(ETutorialActionType::EquipWeapon);



		if (CharacterState != ECharacterState::Block)

		{

			SetCharacterState(ECharacterState::Combat);

		}

	}

	else

	{

		SetCharacterState(ECharacterState::Idle);

	}

}



// ─── [로그 데이터 관련: 이동 거리 및 맵별 체류 시간 측정] ───

void AAGSDCharacter::SetTotalDistance()

{

	FVector CurrentLocation = GetActorLocation();



	float FrameDistance = FVector::Distance(CurrentLocation, LastLocation);



	if (FrameDistance > 0.1f)

	{

		TotalDistance += FrameDistance;

		LastLocation = CurrentLocation;



		if (UWorld* World = GetWorld())

		{

			FString MapName = World->GetMapName();

			MapName.RemoveFromStart(World->StreamingLevelsPrefix);



			if (UGameInstance* GameInst = GetGameInstance())

			{

				if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

				{

					LogSubsystem->AddStageMovementDistance(MapName, FrameDistance / 100.f);

				}

			}

		}

	}

}



// ─── [Task 2 UI & 튜토리얼 로깅 헬퍼 함수] ───



void AAGSDCharacter::LogDialogueFullSkip()

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->IncrementDialogueFullSkip();

		}

	}

}



void AAGSDCharacter::LogDialogueLineSkip()

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->IncrementDialogueLineSkip();

		}

	}

}



void AAGSDCharacter::LogDialogueLineRead()

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->IncrementDialogueLineRead();

		}

	}

}



void AAGSDCharacter::LogDialogueLogRecheck()

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->IncrementDialogueLogRecheck();

		}

	}

}



void AAGSDCharacter::LogOptionChange()

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->IncrementOptionChange();

		}

	}

}



void AAGSDCharacter::LogPotionUsage(const FString& PotionType)

{

	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);

		

		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->RecordPotionUsage(MapName, PotionType);

			}

		}

	}

}



void AAGSDCharacter::LogNPCDialogue(const FString& NPCName)

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->RecordNPCDialogue(NPCName);

		}

	}

}



// ─── [Task 3 보스전 & 기믹 로깅 헬퍼 함수] ───



void AAGSDCharacter::LogBossEnter(const FString& BossName)

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->RecordBossEnter(BossName);

		}

	}

}



void AAGSDCharacter::LogBossDeath(const FString& BossName)

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->RecordBossDeath(BossName);

		}

	}

}



void AAGSDCharacter::LogBossClear(const FString& BossName, float BattleTimeSeconds, int32 LootCount)

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->RecordBossClear(BossName, BattleTimeSeconds, LootCount);

		}

	}

}



bool AAGSDCharacter::IsBossBattleActive() const

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			return LogSubsystem->IsBossBattleActive();

		}

	}

	return false;

}



void AAGSDCharacter::SetBossBattleActive(bool bActive, const FString& BossName)

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->SetBossBattleActive(bActive, BossName);

		}

	}

}



void AAGSDCharacter::LogGimmickClear(const FString& GimmickName)

{

	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);



		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->IncrementStageGimmickClear(MapName, GimmickName);

			}

		}

	}

}



void AAGSDCharacter::LogBossPatternDodge()

{

	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);



		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->IncrementBossPatternDodge(MapName);

			}

		}

	}

}



bool AAGSDCharacter::LogExportCSV(const FString& FilePath)

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			return LogSubsystem->ExportLogsToCSVFile(FilePath);

		}

	}

	return false;

}



void AAGSDCharacter::AddCoin(int32 Amount)

{

	if (Amount <= 0) return;



	Coin += Amount;

	if (GI)

	{

		GI->Coin = Coin;

	}



	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->AddAcquiredCoins(Amount);



			// 골드 파밍 실시간 CSV 로그 기록

			LogSubsystem->LogPlayerItemLooted(

				GetActorLocation(),

				TEXT("Gold"),

				TEXT("GoldReward"),

				TEXT("Gold"),

				TEXT("골드"),

				Amount

			);

		}

	}

}



bool AAGSDCharacter::ConsumeCoin(int32 Amount)

{

	if (Amount <= 0) return false;

	if (Coin < Amount) return false;



	Coin -= Amount;

	if (GI)

	{

		GI->Coin = Coin;

	}



	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

		{

			LogSubsystem->AddConsumedCoins(Amount);

		}

	}



	return true;

}



// ─── [로그 데이터: 세분화된 행동 소요 시간 자동 판별 및 누적] ───



void AAGSDCharacter::SetCurrentActionCategory(const FString& NewCategory)

{

	CurrentActionCategory = NewCategory;

}



void AAGSDCharacter::SetActionNPCService(bool bActive)

{

	SetCurrentActionCategory(bActive ? TEXT("NPC_Service") : TEXT(""));

}



void AAGSDCharacter::UpdateActionDurationLogging(float DeltaSeconds)

{

	if (DeltaSeconds <= 0.0f) return;



	FString DeterminedCategory;



	// 1. 인벤토리 상태 특수 처리: 인벤토리가 열려있더라도 걷거나 달리는 중이면 Walk/Sprint로 배정하여 물리적 정합성 유지!

	if (CurrentActionCategory.Equals(TEXT("Inventory"), ESearchCase::IgnoreCase))

	{

		if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())

		{

			DeterminedCategory = TEXT("Airborne");

		}

		else if (GetVelocity().Size2D() > 15.0f)

		{

			DeterminedCategory = bIsSprinting ? TEXT("Sprint") : TEXT("Walk");

		}

		else

		{

			DeterminedCategory = TEXT("Inventory");

		}

	}

	// 2. 다른 명시적 UI / 상호작용 카테고리가 있는 경우 (Dialogue, PotionCrafting, TributeAltar, NPC_Service 등)

	else if (!CurrentActionCategory.IsEmpty())

	{

		DeterminedCategory = CurrentActionCategory;

	}

	else

	{

		// 3. 캐릭터 몽타주 및 전투 상태 판별

		UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

		UAnimMontage* CurrentMontage = AnimInst ? AnimInst->GetCurrentActiveMontage() : nullptr;

		FString MontageName = CurrentMontage ? CurrentMontage->GetName() : TEXT("");



		if (bIsRolling ||

			CharacterState == ECharacterState::Block || 

			MontageName.Contains(TEXT("Attack")) ||  

			MontageName.Contains(TEXT("Combo")) || 

			MontageName.Contains(TEXT("Skill")) ||

			MontageName.Contains(TEXT("Parry")) ||

			MontageName.Contains(TEXT("Hit")))

		{

			DeterminedCategory = TEXT("Combat");

		}

		else if (MontageName.Contains(TEXT("Plant")) || MontageName.Contains(TEXT("Weed")))

		{

			DeterminedCategory = TEXT("Weeding");

		}

		else if (MontageName.Contains(TEXT("Harvest")) || MontageName.Contains(TEXT("Farm")) || MontageName.Contains(TEXT("Water")))

		{

			DeterminedCategory = TEXT("Farming");

		}

		else if (MontageName.Contains(TEXT("Door")) || MontageName.Contains(TEXT("Switch")))

		{

			DeterminedCategory = TEXT("GimmickObject");

		}

		else if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())

		{

			// 4. 공중 체공 / 점프 / 낙하

			DeterminedCategory = TEXT("Airborne");

		}

		else if (GetVelocity().Size2D() > 15.0f)

		{

			// 5. 지상 이동 (걷기 / 달리기 세분화)

			DeterminedCategory = bIsSprinting ? TEXT("Sprint") : TEXT("Walk");

		}

		else 

		{

			// 6. 지상 정지 관찰 / 고민 (Idle)

			DeterminedCategory = TEXT("Idle");

		}

	}



	// 6. 서브시스템에 해당 카테고리 소요 시간 누적

	if (UWorld* World = GetWorld())

	{

		FString MapName = World->GetMapName();

		MapName.RemoveFromStart(World->StreamingLevelsPrefix);



		if (UGameInstance* GameInst = GetGameInstance())

		{

			if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())

			{

				LogSubsystem->AddStageActionDuration(DeterminedCategory, DeltaSeconds, MapName);

			}

		}

	}

}



void AAGSDCharacter::SetIgnorePawnCollision(bool bIgnore)

{

	bIsCollisionIgnoredByNotify = bIgnore;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())

	{

		Capsule->SetCollisionResponseToChannel(ECC_Pawn, bIgnore ? ECR_Ignore : ECR_Block);

		Capsule->SetCollisionResponseToChannel(ECC_PhysicsBody, bIgnore ? ECR_Ignore : ECR_Block);



		if (bIgnore)

		{

			bool bIsHardLocked = false;

			AActor* TargetActor = LockOnComponent ? LockOnComponent->GetTargetForAttack(bIsHardLocked) : nullptr;

			if (IsValid(TargetActor))

			{

				IgnoredTargetActorOnNotify = TargetActor;

				Capsule->IgnoreActorWhenMoving(TargetActor, true);

				MoveIgnoreActorAdd(TargetActor);

			}

		}

		else

		{

			if (IgnoredTargetActorOnNotify.IsValid())

			{

				AActor* CachedTarget = IgnoredTargetActorOnNotify.Get();

				Capsule->IgnoreActorWhenMoving(CachedTarget, false);

				MoveIgnoreActorRemove(CachedTarget);

				IgnoredTargetActorOnNotify.Reset();

			}

		}

	}

}



void AAGSDCharacter::SetCanBeDamaged(bool bInCanBeDamage)

{

	bIsInvulnerableByNotify = !bInCanBeDamage;

	bCanBeDamage = bInCanBeDamage;

}



void AAGSDCharacter::ReportTutorialAction(ETutorialActionType ActionType, int32 Count)

{

	if (UGameInstance* GameInst = GetGameInstance())

	{

		if (UTutorialSubsystem* TutSub = GameInst->GetSubsystem<UTutorialSubsystem>())

		{

			TutSub->ReportTutorialAction(ActionType, Count);

		}

	}

}



// ─────────────────────────────────────────────────────────────────────────────

// [로스트아크식 3단계 피격 리액션 시스템 구현]

// ─────────────────────────────────────────────────────────────────────────────



void AAGSDCharacter::ProcessHitReaction(float DamageAmount, AActor* DamageCauser, struct FDamageEvent const& DamageEvent)

{

	// 1. 공격자가 보스인지 판별 (보스 본체 또는 보스가 소유한 투사체 등)

	const bool bIsFromBoss = DamageCauser && (

		DamageCauser->IsA(ABaseBossEnemy::StaticClass()) || 

		(DamageCauser->GetOwner() && DamageCauser->GetOwner()->IsA(ABaseBossEnemy::StaticClass()))

	);



	// 1-1. 상호작용 폭탄(BP_Bomb) 폭발도 거리 기반 피격 리액션 대상
	AAGSDBomb* const SourceBomb = Cast<AAGSDBomb>(DamageCauser);

	// 2. 잡몹(보스가 아닌 일반 몬스터)의 공격은 피격 모션을 출력하지 않음 (상시 슈퍼아머)

	if (!bIsFromBoss && !SourceBomb)

	{

		UE_LOG(LogTemp, Log, TEXT("[HitReaction] Minion damage (%.1f from %s) - Hit reaction skipped (Super Armor)"), 

			DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"));

		return;

	}



	// 3. 구르기(회피) 중에는 피격 리액션을 실행하지 않음 (구르기 모션 끊김 방지)

	if (bIsRolling)

	{

		UE_LOG(LogTemp, Log, TEXT("[HitReaction] Attack received during Roll - Hit reaction skipped"));

		return;

	}



	// ⭐️ 3-1. 저스트 가드(패링) 성공 및 반격 콤보 중: 넉다운 및 모든 피격 리액션 100% 완전 면역 (슈퍼아머)!

	const bool bIsParrying = (GuardComponent && !GuardComponent->IsOwnerAirborne() && (GuardComponent->IsJustGuardActive() || GuardComponent->IsJustGuardSuccessful())) ||

	                         (CurrentComboData && CurrentComboData->DirectionRequirement == ESpearAttackDirection::Parry);

	const bool bIsJustGuardCounterAttack = bIsJustGuardCounterActive || 

	                         ((bIsAttacking || Mining) && (GetWorld()->GetTimeSeconds() - LastJustGuardSuccessWorldTime <= JustGuardSuperArmorDuration));



	if (bIsParrying || bIsJustGuardCounterAttack)

	{

		UE_LOG(LogTemp, Log, TEXT("[HitReaction] Just Guard Counter Attack SUPER ARMOR! - Completely immune to Knockdown & Hit Reaction"));

		return;

	}



	// 4. 피격 상대 각도 계산 (-180.0 ~ +180.0도)

	if (DamageCauser)

	{

		const FVector ToCauser = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

		const FVector Forward = GetActorForwardVector().GetSafeNormal2D();

		const FVector Right = GetActorRightVector().GetSafeNormal2D();



		const float DotFwd = FVector::DotProduct(Forward, ToCauser);

		const float DotRight = FVector::DotProduct(Right, ToCauser);



		HitDirectionAngle = FMath::RadiansToDegrees(FMath::Atan2(DotRight, DotFwd));

	}



	// 5. 피격 대미지 갱신

	LastHitDamage = DamageAmount;



	// 5-1. 지속 장판 피해(DoT) 판정: 조작 불가 스턴(HitStun) 및 경직 모션 완전 면제! (체력만 감소 후 자유 이동)

	const bool bIsDamageZone = DamageCauser && (

		DamageCauser->ActorHasTag(FName("DamageZone")) || 

		DamageCauser->ActorHasTag(FName("DoT"))

	);



	if (bIsDamageZone)

	{

		UE_LOG(LogTemp, Verbose, TEXT("[HitReaction] DamageZone DoT damage received (%.1f) - Stun/Flinch skipped"), DamageAmount);

		return;

	}



	// [미니언 공격 배제]: 소환 미니언 등의 일반 몬스터 공격에 대해서는 플레이어 슈퍼아머 유지 (피격 리액션/스턴 면제)

	const bool bIsMinion = DamageCauser && (DamageCauser->IsA(ABaseEnemy::StaticClass()) && !DamageCauser->IsA(ABaseBossEnemy::StaticClass()));

	if (bIsMinion)

	{

		UE_LOG(LogTemp, Verbose, TEXT("[HitReaction] Minion attack received (Damage: %.1f) - Player Super Armor maintained"), DamageAmount);

		return;

	}



	// 6. 스킬 및 투사체로부터 연출 강도(HitIntensity) 및 넉다운 설정(KnockdownType, KnockdownDuration) 추출

	float ResolvedIntensity = -1.0f;

	EKnockdownType TargetKnockdownType = EKnockdownType::None; // 기본값은 None (보스 명시적 설정 우선)

	float TargetKnockdownDuration = KnockdownDuration; // 기본값 (1.2초)

	bool bHasExplicitReactionSettings = false;

	FVector PendingLaunchVelocity = FVector::ZeroVector; // 넉다운 시 날려보낼 속도 (폭탄 등)



	if (SourceBomb)

	{

		// 근거리: 에어본 넉다운 + 날아감 / 원거리: 일반 피격 (강도 1 → 0 거리 보간)
		bool bBombKnockdown = false;
		float BombIntensity = 0.0f;
		if (!SourceBomb->GetExplosionReaction(GetActorLocation(), bBombKnockdown, BombIntensity, PendingLaunchVelocity))
		{
			// 반경 밖에서 데미지만 들어온 경우에도 최약 피격으로 처리
			BombIntensity = 0.0f;
		}

		ResolvedIntensity = BombIntensity;
		if (bBombKnockdown)
		{
			TargetKnockdownType = EKnockdownType::AirborneFloor;
			TargetKnockdownDuration = SourceBomb->GetKnockdownDuration();
		}
		bHasExplicitReactionSettings = true;

		UE_LOG(LogTemp, Log, TEXT("[HitReaction] Bomb explosion: Knockdown=%d, Intensity=%.2f"), bBombKnockdown ? 1 : 0, BombIntensity);

	}

	else if (ABaseEnemyProjectile* Proj = Cast<ABaseEnemyProjectile>(DamageCauser))

	{

		ResolvedIntensity = Proj->HitIntensity;

		if (Proj->KnockdownType != EKnockdownType::None)

		{

			TargetKnockdownType = Proj->KnockdownType;

			TargetKnockdownDuration = Proj->KnockdownDuration;

		}

		bHasExplicitReactionSettings = true;

	}

	else if (AGroundAttackProjectile* GroundProj = Cast<AGroundAttackProjectile>(DamageCauser))

	{

		ResolvedIntensity = GroundProj->HitIntensity;

		if (GroundProj->KnockdownType != EKnockdownType::None)

		{

			TargetKnockdownType = GroundProj->KnockdownType;

			TargetKnockdownDuration = GroundProj->KnockdownDuration;

		}

		bHasExplicitReactionSettings = true;

	}

	else if (ABaseBossEnemy* Boss = Cast<ABaseBossEnemy>(DamageCauser))

	{

		if (Boss->bHasExplicitSkillReaction)

		{

			ResolvedIntensity = Boss->CurrentSkillHitIntensity;

			// 보스가 명시적으로 설정한 넉다운 타입을 무조건 최우선 적용 (None이면 진짜 넉다운 없음!)

			TargetKnockdownType = Boss->CurrentSkillKnockdownType;

			TargetKnockdownDuration = (Boss->CurrentSkillKnockdownDuration > 0.0f) ? Boss->CurrentSkillKnockdownDuration : KnockdownDuration;

			bHasExplicitReactionSettings = true;

		}

	}

	else if (DamageCauser && DamageCauser->GetOwner() && DamageCauser->GetOwner()->IsA(ABaseBossEnemy::StaticClass()) && !DamageCauser->IsA(ABaseEnemy::StaticClass()))

	{

		if (ABaseBossEnemy* OwnerBoss = Cast<ABaseBossEnemy>(DamageCauser->GetOwner()))

		{

			if (OwnerBoss->bHasExplicitSkillReaction)

			{

				ResolvedIntensity = OwnerBoss->CurrentSkillHitIntensity;

				TargetKnockdownType = OwnerBoss->CurrentSkillKnockdownType;

				TargetKnockdownDuration = (OwnerBoss->CurrentSkillKnockdownDuration > 0.0f) ? OwnerBoss->CurrentSkillKnockdownDuration : KnockdownDuration;

				bHasExplicitReactionSettings = true;

			}

		}

	}



	// 보스의 명시적 설정이 없을 때만 대미지 120 이상 시 에어본 Fallback 적용

	if (!bHasExplicitReactionSettings && DamageAmount >= 120.0f)

	{

		TargetKnockdownType = EKnockdownType::AirborneFloor;

	}



	// ⭐️ 6-0. 넉다운(에어본/무릎) 진행 중 넉다운 없는 후속 타격: 대미지만 받고 넉다운 모션 유지 (저글링 상태)
	// - 예: 마법검사 띄우기 → 공중 공격 연타가 에어본 몽타주를 끊고 일반 경직으로 바뀌는 문제 방지
	if (bIsKnockdown && TargetKnockdownType == EKnockdownType::None)
	{
		UE_LOG(LogTemp, Log, TEXT("[HitReaction] Hit during knockdown (%.1f) - Knockdown montage preserved"), DamageAmount);
		return;
	}



	// ⭐️ 6-1. 단순 가드(막기) 상태 판정:

	// - 공격 중(bIsAttacking, Mining)이 아닐 때만 유효 가드로 인정! (공격 중 가드 경직 면역 원천 차단)

	// - 넉다운 강공격을 맞으면 가드가 깨지며(StopBlock) 넉다운 발생

	// - 일반 공격을 맞으면 가드로 든든하게 막아내며 피격 리액션 스킵

	if (CharacterState == ECharacterState::Block && !bIsAttacking && !Mining
		&& !(GuardComponent && GuardComponent->IsOwnerAirborne())) // 공중에서는 가드로 막지 못함

	{

		const bool bIsUnblockable = DamageCauser && (DamageCauser->ActorHasTag(TEXT("Unblockable")) || DamageCauser->ActorHasTag(TEXT("Trap")));

		if (TargetKnockdownType != EKnockdownType::None || bIsUnblockable)

		{

			if (GuardComponent)

			{

				GuardComponent->StopBlock(); // 가드 강제 해제 (가드 브레이크 / 가드 무시)

			}

			UE_LOG(LogTemp, Log, TEXT("[HitReaction] Guard Broken by Heavy/Unblockable attack!"));

		}

		else

		{

			UE_LOG(LogTemp, Log, TEXT("[HitReaction] Normal attack blocked by Guard - Hit reaction skipped"));

			return;

		}

	}



	// ⭐️ 무경직 피격 판정 (보스가 경직 강도 음수 및 넉다운 없음을 명시한 경우):

	// 피격 강도가 0.0f인 경우에도 일반 피격 모션을 정상 재생하며, 음수(< 0.0f)일 때만 스킵!

	if (bHasExplicitReactionSettings && ResolvedIntensity < 0.0f && TargetKnockdownType == EKnockdownType::None)

	{

		UE_LOG(LogTemp, Log, TEXT("[HitReaction] Negative-intensity attack received (%.1f) - Stun skipped!"), DamageAmount);

		return;

	}



	// 7. 넉다운 분기 처리 (무릎 꿇기 vs 에어본 후 바닥 엎어짐)

	if (TargetKnockdownType != EKnockdownType::None)

	{

		CurrentKnockdownType = TargetKnockdownType;

		bIsKnockdown = true;

		bKnockdownStandUp = false;



		// ⭐️ 핵심: 넉다운 상태에서는 일반 피격 리액션(bIsHitReacting)을 꺼서

		// 애님그래프 메인의 Blend Poses(bIsHitReacting)가 MainState(Knockdown)를 가로채지 못하도록 보장!

		bIsHitReacting = false;



		// 이전 넉다운/피격 타이머 완전 정리 (새 피격 도중 이전 세이프티가 터지는 참사 차단!)

		GetWorldTimerManager().ClearTimer(HitReactTimerHandle);

		GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

		GetWorldTimerManager().ClearTimer(AirborneApexTimerHandle);

		GetWorldTimerManager().ClearTimer(AirborneKnockdownPauseTimerHandle);

		GetWorldTimerManager().ClearTimer(AirborneKnockdownResumeTimerHandle);

		GetWorldTimerManager().ClearTimer(KneelDurationTimerHandle);

		EndHitStunTracking();

		StartKnockdownTracking();



		StopAnimMontage();

		bIsAttacking = false;

		bIsRecovering = false;

		bCanCombo = false;

		bHasBufferedInput = false;



		// 넉백 속도가 지정된 경우(폭탄 등) 몽타주 재생 전에 런치 (LaunchCharacter가 몽타주를 정지시키므로 순서 중요)
		if (!PendingLaunchVelocity.IsNearlyZero())
		{
			BeginKnockbackFlight(PendingLaunchVelocity);
		}



		// 7-1. 에어본(AirborneFloor)의 경우 몽타주(Start -> Loop -> Landed -> End -> Knockdown(Pause) -> Resume) 파이프라인 가동

		if (TargetKnockdownType == EKnockdownType::AirborneFloor)

		{

			UAnimMontage* SelectedAirborneMontage = GetAirborneMontageToPlay();

			if (SelectedAirborneMontage)

			{

				CurrentActiveAirborneMontage = SelectedAirborneMontage;

				bIsAirborneMontageActive = true;

				CurrentTargetKnockdownDuration = TargetKnockdownDuration;



				// ⭐️ 핵심: 에어본 체공 비행 동안 애니메이션의 제자리 루트 모션을 무시하고 물리 포물선 속도 보장!

				if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)

				{

					AnimInst->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);

					UE_LOG(LogTemp, Log, TEXT("[Airborne] RootMotionMode set to IgnoreRootMotion for physics flight!"));

				}



				PlayAnimMontage(SelectedAirborneMontage);



				// Start 정점(0.58초)에서 물리 낙하 모드로 전환

				GetWorldTimerManager().ClearTimer(AirborneApexTimerHandle);

				GetWorldTimerManager().SetTimer(AirborneApexTimerHandle, this, &AAGSDCharacter::OnAirborneApexReached, 0.58f, false);



				// ⭐️ 이중 안전장치: 정상 기상(체공+바닥대기+기상 약 4.2s)을 방해하지 않도록 5.5s 세이프티 가동!

				const float SafeSafetyDuration = FMath::Max(5.5f, TargetKnockdownDuration + 4.0f);

				GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

				GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::FinishKnockdown, SafeSafetyDuration, false);



				ApplyHitStun(SafeSafetyDuration);

			}

			else

			{

				// 에어본 몽타주 미할당 시 애님그래프 스테이트 머신 기상 타이머 가동

				GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

				GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::TriggerKnockdownStandUp, TargetKnockdownDuration, false);

				ApplyHitStun(TargetKnockdownDuration + 1.5f);

			}

		}

		// 7-2. 무릎 꿇기(Kneel)의 경우 단일 몽타주(Start -> Loop 체류 -> End 기상 점프) 파이프라인 가동

		else if (TargetKnockdownType == EKnockdownType::Kneel)

		{

			if (KneelMontage)

			{

				CurrentTargetKnockdownDuration = TargetKnockdownDuration;

				// ⭐️ Kneel 시작 당시의 무장 상태를 기억하여 기상 시 동일 트랙 보장

				bKneelStartedWithWeapon = HasEquippedWeapon();

				const FName SectionName = GetKneelStartSectionName();



				UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

				if (AnimInstance)

				{

					FOnMontageEnded EndDelegate;

					EndDelegate.BindUObject(this, &AAGSDCharacter::OnKneelMontageEnded);

					AnimInstance->Montage_SetEndDelegate(EndDelegate, KneelMontage);

				}



				PlayAnimMontage(KneelMontage, 1.0f, SectionName);



				// Start(무릎 꿇는 모션 약 0.8초) + TargetKnockdownDuration(Loop 체류 시간) 대기 타이머 가동

				const float StartMotionEstimatedDuration = 0.8f;

				const float TotalWaitDuration = StartMotionEstimatedDuration + ((TargetKnockdownDuration > 0.0f) ? TargetKnockdownDuration : 1.2f);

				GetWorldTimerManager().ClearTimer(KneelDurationTimerHandle);

				GetWorldTimerManager().SetTimer(KneelDurationTimerHandle, this, &AAGSDCharacter::OnKneelDurationExpired, TotalWaitDuration, false);



				// ⭐️ 이중 안전장치: OnKneelMontageEnded 누락 시에도 강제 스턴 해제

				const float KneelSafetyDuration = TotalWaitDuration + 2.0f;

				GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

				GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::FinishKnockdown, KneelSafetyDuration, false);



				ApplyHitStun(KneelSafetyDuration);

				UE_LOG(LogTemp, Log, TEXT("[HitReaction] Kneel Montage Triggered! StartSection: %s, TotalWait: %.2fs (Safety: %.2fs)"),

					*SectionName.ToString(), TotalWaitDuration, KneelSafetyDuration);

			}

			else

			{

				// Kneel 몽타주 미할당 시 기존 애님그래프 스테이트 머신 기상 타이머 가동

				GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

				GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::TriggerKnockdownStandUp, TargetKnockdownDuration, false);

				ApplyHitStun(TargetKnockdownDuration + 1.5f);

			}

		}

		else

		{

			// 기타 넉다운 타입 애님그래프 스테이트 머신 기상 타이머 가동

			GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

			GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::TriggerKnockdownStandUp, TargetKnockdownDuration, false);

			ApplyHitStun(TargetKnockdownDuration + 1.5f);

		}



		UE_LOG(LogTemp, Log, TEXT("[HitReaction] State Machine KNOCKDOWN Triggered! Type: %d, Duration: %.2fs (WeaponEquipped: %d)"),

			static_cast<int32>(TargetKnockdownType), TargetKnockdownDuration, HasEquippedWeapon() ? 1 : 0);

		return;

	}



	// 8. 일반 피격 연출 강도 결정

	if (ResolvedIntensity < 0.0f)

	{

		ResolvedIntensity = (DamageAmount >= 15.0f) ? 1.0f : 0.0f;

	}



	HitIntensityValue = FMath::Clamp(ResolvedIntensity, 0.0f, 1.0f);



	// [연타 스턴 락 방지]: 이미 피격 중이면 타이머를 연장하지 않고 기존 타이머 유지

	if (bIsHitReacting)

	{

		UE_LOG(LogTemp, Log, TEXT("[HitReaction] Already HitReacting: Anti-stun-lock active (Timer preserved)"));

		return;

	}



	// [보스 공격 강제 캔슬]: 공격 중(bIsAttacking)이라도 보스 공격에 맞으면 즉시 캔슬하고 비틀거림!

	StopAnimMontage();

	bIsAttacking = false;

	bIsRecovering = false;

	bCanCombo = false;

	bHasBufferedInput = false;



	bIsHitReacting = true;

	StartHitStunTracking();



	GetWorldTimerManager().ClearTimer(HitReactTimerHandle);

	GetWorldTimerManager().SetTimer(HitReactTimerHandle, this, &AAGSDCharacter::ResetHitReact, HitReactDuration, false);



	UE_LOG(LogTemp, Log, TEXT("[HitReaction] 2D BS Hit Triggered! Angle: %.1f, Intensity: %.1f (Damage: %.1f, Duration: %.3fs)"),

		HitDirectionAngle, HitIntensityValue, LastHitDamage, HitReactDuration);

}



UAnimMontage* AAGSDCharacter::GetAirborneMontageToPlay(bool bForceInAir) const

{

	const bool bHasWeapon = HasEquippedWeapon();



	// 1. 공중에 떠 있는 상태인지 판별 (강제 지정 또는 현재 에어본 중이거나 Falling 상태)

	bool bIsCurrentlyInAir = bForceInAir || bIsAirborneMontageActive;

	if (!bIsCurrentlyInAir && GetCharacterMovement())

	{

		bIsCurrentlyInAir = GetCharacterMovement()->IsFalling();

	}



	// 2. 공중 상태일 때의 몽타주 반환 (설정되지 않았을 경우 지상 몽타주로 안전하게 fallback)

	if (bIsCurrentlyInAir)

	{

		if (bHasWeapon && CombatInAirAirborneMontage)

		{

			return CombatInAirAirborneMontage;

		}

		if (!bHasWeapon && InAirAirborneMontage)

		{

			return InAirAirborneMontage;

		}

		if (bHasWeapon && CombatAirborneMontage)

		{

			return CombatAirborneMontage;

		}

		return AirborneMontage;

	}



	// 3. 지상 상태일 때의 몽타주 반환

	if (bHasWeapon && CombatAirborneMontage)

	{

		return CombatAirborneMontage;

	}

	return AirborneMontage;

}



FName AAGSDCharacter::GetKneelStartSectionName() const

{

	if (HasEquippedWeapon())

	{

		return FName("Start_Combat");

	}

	return FName("Start_Unarmed");

}



FName AAGSDCharacter::GetKneelEndSectionName() const

{

	// ⭐️ Kneel 시작 당시 무장을 들고 있었으면 무조건 End_Combat, 비무장이었으면 무조건 End_Unarmed 반환

	if (bKneelStartedWithWeapon)

	{

		return FName("End_Combat");

	}

	return FName("End_Unarmed");

}



void AAGSDCharacter::ResetHitReact()

{

	bIsAirborneMontageActive = false;

	CurrentActiveAirborneMontage = nullptr;

	GetWorldTimerManager().ClearTimer(AirborneApexTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownPauseTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownResumeTimerHandle);

	GetWorldTimerManager().ClearTimer(KneelDurationTimerHandle);



	bIsHitReacting = false;

	bIsKnockdown = false;

	bKnockdownStandUp = false;

	bKneelStartedWithWeapon = false;

	CurrentKnockdownType = EKnockdownType::None;



	// [루트 모션 모드 복원]: 구르기(BS_Roll) 블렌드스페이스 이동 복원

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)

	{

		AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);

	}



	GetWorldTimerManager().ClearTimer(HitReactTimerHandle);

	GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

	EndHitStunTracking();

	EndKnockdownTracking();

	UE_LOG(LogTemp, Log, TEXT("[HitReaction] Hit reaction reset"));

}



void AAGSDCharacter::SuspendFallingBrakingForKnockback()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// 공중 감속(BrakingDecelerationFalling)이 수평 넉백 속도를 즉시 죽이지 않도록 비행 중에는 0으로
	if (!bKnockbackFlightActive)
	{
		SavedBrakingDecelerationFalling = MoveComp->BrakingDecelerationFalling;
		bKnockbackFlightActive = true;
	}
	MoveComp->BrakingDecelerationFalling = 0.0f;
}

void AAGSDCharacter::BeginKnockbackFlight(const FVector& LaunchVelocity)
{
	SuspendFallingBrakingForKnockback();

	LaunchCharacter(LaunchVelocity, true, true);

	UE_LOG(LogTemp, Log, TEXT("[Airborne] Knockback flight launched: %s"), *LaunchVelocity.ToString());
}

void AAGSDCharacter::EndKnockbackFlight()
{
	if (!bKnockbackFlightActive)
	{
		return;
	}

	bKnockbackFlightActive = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->BrakingDecelerationFalling = SavedBrakingDecelerationFalling;
	}
}

void AAGSDCharacter::Landed(const FHitResult& Hit)

{

	Super::Landed(Hit);

	// 넉백 비행 종료: 공중 감속 복원 (착지 후 미끄러짐은 지상 마찰/감속이 처리)
	EndKnockbackFlight();



	if (bIsAirborneMontageActive)

	{

		bIsAirborneMontageActive = false;

		GetWorldTimerManager().ClearTimer(AirborneApexTimerHandle);



		UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;



		// ⭐️ 지면 착지 즉시 루트 모션 모드 복원 (Everything으로 복원하여 구르기 블렌드스페이스 루트 모션 보장)

		if (AnimInstance)

		{

			AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);

		}



		UAnimMontage* MontageToJump = CurrentActiveAirborneMontage ? CurrentActiveAirborneMontage : GetAirborneMontageToPlay();

		if (AnimInstance && MontageToJump)

		{

			CurrentActiveAirborneMontage = MontageToJump;



			// 1. 몽타주 종료 델리게이트 바인딩 (Knockdown 기상까지 모두 완료되었을 때 호출)

			FOnMontageEnded EndDelegate;

			EndDelegate.BindUObject(this, &AAGSDCharacter::OnAirborneMontageEnded);

			AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToJump);



			// 2. 지면 착지 즉시 End(바닥 충돌 및 엎어짐) 섹션 실행 (에셋의 End -> Knockdown 연결 보존!)

			PlayAnimMontage(MontageToJump, 1.0f, FName("End"));



			// 3. End 섹션 길이 계산: 끝나기 직전 프레임(약 0.08초 전)에 일시정지 타이머 가동

			float EndSectionLength = 0.4f;

			const int32 EndSecIndex = MontageToJump->GetSectionIndex(FName("End"));

			if (EndSecIndex != INDEX_NONE)

			{

				EndSectionLength = MontageToJump->GetSectionLength(EndSecIndex);

			}

			const float PauseDelay = FMath::Max(0.05f, EndSectionLength - 0.08f);



			GetWorldTimerManager().ClearTimer(AirborneKnockdownPauseTimerHandle);

			GetWorldTimerManager().SetTimer(AirborneKnockdownPauseTimerHandle, this, &AAGSDCharacter::OnAirborneKnockdownPauseReached, PauseDelay, false);



			UE_LOG(LogTemp, Log, TEXT("[Airborne] Landed on ground -> Playing End section of %s (Length: %.3fs, Pause in %.3fs)"),

				*MontageToJump->GetName(), EndSectionLength, PauseDelay);

		}

	}

}



void AAGSDCharacter::OnAirborneKnockdownPauseReached()

{

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	UAnimMontage* ActiveMontage = CurrentActiveAirborneMontage ? CurrentActiveAirborneMontage : GetAirborneMontageToPlay();

	if (AnimInstance && ActiveMontage)

	{

		// End 섹션 끝 프레임(바닥에 완전히 엎어진 포즈)에서 몽타주 일시정지!

		AnimInstance->Montage_Pause(ActiveMontage);

		UE_LOG(LogTemp, Log, TEXT("[Airborne] End section finished -> Montage paused on floor for Knockdown duration (%.2fs)"), CurrentTargetKnockdownDuration);



		// 지정된 넉다운 시간 동안 바닥 대기 타이머 가동

		const float SafeWaitDuration = (CurrentTargetKnockdownDuration > 0.0f) ? CurrentTargetKnockdownDuration : 1.2f;

		GetWorldTimerManager().ClearTimer(AirborneKnockdownResumeTimerHandle);

		GetWorldTimerManager().SetTimer(AirborneKnockdownResumeTimerHandle, this, &AAGSDCharacter::OnAirborneKnockdownResume, SafeWaitDuration, false);

	}

}



void AAGSDCharacter::OnAirborneKnockdownResume()

{

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	UAnimMontage* ActiveMontage = CurrentActiveAirborneMontage ? CurrentActiveAirborneMontage : GetAirborneMontageToPlay();

	

	if (AnimInstance && ActiveMontage)

	{

		// ⭐️ 일시정지만 해제하면 에셋의 End -> Knockdown 연결을 따라 자연스럽게 기상(누워있다 일어나는 모션)으로 진행!

		AnimInstance->Montage_Resume(ActiveMontage);

		UE_LOG(LogTemp, Log, TEXT("[Airborne] Knockdown duration expired -> Montage resumed to Knockdown section of %s"), *ActiveMontage->GetName());

	}

	else

	{

		FinishKnockdown();

	}

}



void AAGSDCharacter::OnAirborneMontageEnded(UAnimMontage* Montage, bool bInterrupted)

{

	UE_LOG(LogTemp, Log, TEXT("[Airborne] Airborne Montage ended (Interrupted: %d) -> Instant Knockdown Finish!"), bInterrupted ? 1 : 0);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownPauseTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownResumeTimerHandle);

	CurrentActiveAirborneMontage = nullptr;

	FinishKnockdown();

}



void AAGSDCharacter::OnAirborneApexReached()

{

	if (bIsAirborneMontageActive && GetCharacterMovement())

	{

		// 루트 모션 위치 제어에서 물리 중력 낙하로 전환

		GetCharacterMovement()->SetMovementMode(MOVE_Falling);

		UE_LOG(LogTemp, Log, TEXT("[Airborne] Apex reached -> Falling mode activated"));

	}

}



void AAGSDCharacter::TriggerKnockdownStandUp()

{

	bKnockdownStandUp = true;

	UE_LOG(LogTemp, Log, TEXT("[HitReaction] Knockdown duration finished -> Stand up triggered (bKnockdownStandUp = true)"));



	// 기상 애니메이션 끝의 FinishKnockdown 애님 노티파이를 대기 (노티파이 누락 대비 2.0초 세이프티 타이머 설정)

	GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

	GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::FinishKnockdown, 2.0f, false);

}



void AAGSDCharacter::FinishKnockdown()

{

	EndKnockbackFlight(); // 안전장치: 착지 누락 시에도 공중 감속 복원

	EndKnockdownTracking();

	bIsKnockdown = false;

	bKnockdownStandUp = false;

	CurrentKnockdownType = EKnockdownType::None;



	// ⭐️ 넉다운 종료 시 루트 모션 모드 확실히 복원 (Everything으로 복원하여 구르기 블렌드스페이스 정상화)

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)

	{

		AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);

	}



	GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownPauseTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownResumeTimerHandle);

	GetWorldTimerManager().ClearTimer(KneelDurationTimerHandle);

	ClearHitStun();

	ResetHitReact();

	UE_LOG(LogTemp, Log, TEXT("[HitReaction] Knockdown finish complete -> Restored locomotion state"));

}



void AAGSDCharacter::OnKneelDurationExpired()

{

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	if (AnimInstance && KneelMontage && AnimInstance->Montage_IsPlaying(KneelMontage))

	{

		const FName TargetEndSection = GetKneelEndSectionName();



		// ⭐️ JumpToSection은 0초 하드 점프라 뚝 끊기지만, PlayAnimMontage로 End 섹션을 새로 재생하면

		// 몽타주 내장 Blend In Time(0.2~0.25초)을 통해 현재 Loop 포즈에서 End 기상 포즈로 부드럽게 크로스페이드(블렌드 인)됩니다!

		PlayAnimMontage(KneelMontage, 1.0f, TargetEndSection);



		// 2중 안전장치: End 섹션 완료 후 뒤의 섹션으로 넘어가지 않고 즉시 종료되도록 보장

		AnimInstance->Montage_SetNextSection(TargetEndSection, NAME_None, KneelMontage);



		FOnMontageEnded EndDelegate;

		EndDelegate.BindUObject(this, &AAGSDCharacter::OnKneelMontageEnded);

		AnimInstance->Montage_SetEndDelegate(EndDelegate, KneelMontage);



		UE_LOG(LogTemp, Log, TEXT("[Kneel] Kneel Duration expired -> Smooth Blend-In to End section: %s"), *TargetEndSection.ToString());

	}

	else

	{

		FinishKnockdown();

	}

}



void AAGSDCharacter::OnKneelMontageEnded(UAnimMontage* Montage, bool bInterrupted)

{

	if (Montage == KneelMontage)

	{

		GetWorldTimerManager().ClearTimer(KneelDurationTimerHandle);

		FinishKnockdown();

		ClearHitStun();

		UE_LOG(LogTemp, Log, TEXT("[Kneel] Kneel Montage Ended -> Stun cleared immediately! (Interrupted: %d)"), bInterrupted ? 1 : 0);

	}

}



void AAGSDCharacter::ApplyHitStun(float Duration)

{

	// 넉다운 상태 중에는 일반 피격 리액션(bIsHitReacting)을 켜지 않음

	if (!bIsKnockdown)

	{

		bIsHitReacting = true;

		StartHitStunTracking();

	}

	GetWorldTimerManager().ClearTimer(HitStunTimerHandle);

	GetWorldTimerManager().SetTimer(HitStunTimerHandle, this, &AAGSDCharacter::ClearHitStun, Duration, false);

}



void AAGSDCharacter::ClearHitStun()

{

	bIsHitReacting = false;

	EndHitStunTracking();

	ResetHitReact();

	GetWorldTimerManager().ClearTimer(HitStunTimerHandle);

}



void AAGSDCharacter::ToggleGodMode()

{

	bIsGodMode = !bIsGodMode;

	const FString StatusStr = bIsGodMode ? TEXT("ON (무적 활성화)") : TEXT("OFF (무적 해제)");

	if (GEngine)

	{

		GEngine->AddOnScreenDebugMessage(-1, 3.5f, bIsGodMode ? FColor::Green : FColor::Red, FString::Printf(TEXT("[치트] God Mode: %s"), *StatusStr));

	}

	UE_LOG(LogTemp, Warning, TEXT("[Cheat] God Mode is now: %s"), *StatusStr);

}



void AAGSDCharacter::Cheat_Airborne(float UpwardForce, float BackwardForce, float Duration)

{

	UE_LOG(LogTemp, Warning, TEXT("[Cheat] Cheat_Airborne Triggered! (Up: %.1f, Back: %.1f, Duration: %.2fs)"), UpwardForce, BackwardForce, Duration);

	if (GEngine)

	{

		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("[치트] Airborne 발동 (Up: %.0f, Back: %.0f, Duration: %.1fs)"), UpwardForce, BackwardForce, Duration));

	}



	// 1. 진행 중인 몽타주 및 공격 액션 즉시 취소

	StopAnimMontage();

	bIsAttacking = false;

	bIsRecovering = false;

	bCanCombo = false;

	bHasBufferedInput = false;



	// 2. 캐릭터 후방 + 상방 물리 런치 (힘이 부여된 경우에만 실행)

	if (UpwardForce > 0.0f || BackwardForce > 0.0f)

	{

		FVector BackwardDir = -GetActorForwardVector();

		BackwardDir.Z = 0.0f;

		BackwardDir.Normalize();

		FVector LaunchVel = (BackwardDir * BackwardForce) + FVector(0.0f, 0.0f, UpwardForce);

		LaunchCharacter(LaunchVel, true, true);

	}



	// 3. 에어본 넉다운 상태 및 플래그 갱신

	CurrentKnockdownType = EKnockdownType::AirborneFloor;

	bIsKnockdown = true;

	bKnockdownStandUp = false;

	bIsHitReacting = false;



	// 4. 피격 추적 및 타이머 재설정

	GetWorldTimerManager().ClearTimer(HitReactTimerHandle);

	GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneApexTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownPauseTimerHandle);

	GetWorldTimerManager().ClearTimer(AirborneKnockdownResumeTimerHandle);

	GetWorldTimerManager().ClearTimer(KneelDurationTimerHandle);

	EndHitStunTracking();

	StartKnockdownTracking();



	// 5. 에어본 몽타주가 존재하면 Start -> Loop -> End 몽타주 파이프라인 가동

	UAnimMontage* SelectedMontage = GetAirborneMontageToPlay();

	if (SelectedMontage)

	{

		CurrentActiveAirborneMontage = SelectedMontage;

		bIsAirborneMontageActive = true;

		CurrentTargetKnockdownDuration = (Duration > 0.0f) ? Duration : KnockdownDuration;



		// ⭐️ 에어본 체공 비행 동안 애니메이션 루트 모션을 무시하고 물리 포물선 속도 보장

		if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)

		{

			AnimInst->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);

		}



		PlayAnimMontage(SelectedMontage);



		// Start 정점(0.58초)에서 물리 낙하 모드로 전환하는 타이머

		GetWorldTimerManager().ClearTimer(AirborneApexTimerHandle);

		GetWorldTimerManager().SetTimer(AirborneApexTimerHandle, this, &AAGSDCharacter::OnAirborneApexReached, 0.58f, false);



		// ⭐️ 이중 안전장치: 최대 5.5초 뒤 강제 스턴 해제

		const float SafeSafetyDuration = FMath::Max(5.5f, CurrentTargetKnockdownDuration + 4.0f);

		GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

		GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::FinishKnockdown, SafeSafetyDuration, false);



		ApplyHitStun(SafeSafetyDuration);

	}

	else

	{

		// Fallback: 몽타주가 없을 경우 기존 타이머 기반 전이

		const float SafeDuration = (Duration > 0.0f) ? Duration : KnockdownDuration;

		GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

		GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::TriggerKnockdownStandUp, SafeDuration, false);

		ApplyHitStun(SafeDuration + 1.5f);

	}

}



void AAGSDCharacter::Cheat_Kneel(float Duration)

{

	UE_LOG(LogTemp, Warning, TEXT("[Cheat] Cheat_Kneel Triggered! (Duration: %.2fs)"), Duration);

	if (GEngine)

	{

		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("[치트] Kneel 발동 (Duration: %.1fs)"), Duration));

	}



	// 1. 진행 중인 몽타주 및 공격 액션 즉시 취소

	StopAnimMontage();

	bIsAttacking = false;

	bIsRecovering = false;

	bCanCombo = false;

	bHasBufferedInput = false;



	// 2. 무릎 꿇기 넉다운 상태 및 플래그 갱신

	CurrentKnockdownType = EKnockdownType::Kneel;

	bIsKnockdown = true;

	bKnockdownStandUp = false;

	bIsHitReacting = false;



	// 3. 피격 추적 및 타이머 재설정

	GetWorldTimerManager().ClearTimer(HitReactTimerHandle);

	EndHitStunTracking();

	StartKnockdownTracking();



	const float SafeDuration = (Duration > 0.0f) ? Duration : KnockdownDuration;

	CurrentTargetKnockdownDuration = SafeDuration;



	// 4. Kneel 몽타주가 존재하면 무장/비무장 섹션 분기 몽타주 파이프라인 가동

	if (KneelMontage)

	{

		bKneelStartedWithWeapon = HasEquippedWeapon();

		const FName SectionName = GetKneelStartSectionName();



		UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

		if (AnimInstance)

		{

			FOnMontageEnded EndDelegate;

			EndDelegate.BindUObject(this, &AAGSDCharacter::OnKneelMontageEnded);

			AnimInstance->Montage_SetEndDelegate(EndDelegate, KneelMontage);

		}



		PlayAnimMontage(KneelMontage, 1.0f, SectionName);



		// Start 모션(약 0.8초) + Loop 체류 시간(SafeDuration) 대기 타이머 가동

		const float StartMotionEstimatedDuration = 0.8f;

		const float TotalWaitDuration = StartMotionEstimatedDuration + SafeDuration;

		GetWorldTimerManager().ClearTimer(KneelDurationTimerHandle);

		GetWorldTimerManager().SetTimer(KneelDurationTimerHandle, this, &AAGSDCharacter::OnKneelDurationExpired, TotalWaitDuration, false);



		// ⭐️ 이중 안전장치: 넉다운 체류 후 최대 2초 뒤 강제 스턴 해제

		const float KneelSafetyDuration = TotalWaitDuration + 2.0f;

		GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

		GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::FinishKnockdown, KneelSafetyDuration, false);



		ApplyHitStun(KneelSafetyDuration);

		UE_LOG(LogTemp, Log, TEXT("[Cheat] Kneel Montage Triggered! StartSection: %s, TotalWait: %.2fs (Safety: %.2fs)"),

			*SectionName.ToString(), TotalWaitDuration, KneelSafetyDuration);

	}

	else

	{

		// Fallback: 몽타주 미할당 시 애님그래프 스테이트 머신 기상 타이머 가동

		GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);

		GetWorldTimerManager().SetTimer(KnockdownTimerHandle, this, &AAGSDCharacter::TriggerKnockdownStandUp, SafeDuration, false);



		// 5. 조작 불가 스턴 적용

		ApplyHitStun(SafeDuration + 5.0f);

	}

}



void AAGSDCharacter::StartHitStunTracking()

{

	if (CurrentHitStunStartTime < 0.0f)

	{

		CurrentHitStunStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

		HitStunCount++;

	}

}



void AAGSDCharacter::EndHitStunTracking()

{

	if (CurrentHitStunStartTime >= 0.0f)

	{

		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : CurrentHitStunStartTime;

		TotalHitStunDuration += FMath::Max(0.0f, Now - CurrentHitStunStartTime);

		CurrentHitStunStartTime = -1.0f;

	}

}



void AAGSDCharacter::StartKnockdownTracking()

{

	if (CurrentKnockdownStartTime < 0.0f)

	{

		CurrentKnockdownStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

		KnockdownCount++;

	}

}



void AAGSDCharacter::EndKnockdownTracking()

{

	if (CurrentKnockdownStartTime >= 0.0f)

	{

		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : CurrentKnockdownStartTime;

		TotalKnockdownDuration += FMath::Max(0.0f, Now - CurrentKnockdownStartTime);

		CurrentKnockdownStartTime = -1.0f;

	}

}



float AAGSDCharacter::GetParticipantHitStunDuration() const

{

	float Duration = TotalHitStunDuration;

	if (CurrentHitStunStartTime >= 0.0f && GetWorld())

	{

		Duration += FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - CurrentHitStunStartTime);

	}

	return Duration;

}



float AAGSDCharacter::GetParticipantKnockdownDuration() const

{

	float Duration = TotalKnockdownDuration;

	if (CurrentKnockdownStartTime >= 0.0f && GetWorld())

	{

		Duration += FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - CurrentKnockdownStartTime);

	}

	return Duration;

}



void AAGSDCharacter::ResetParticipantStunMetrics()

{

	TotalHitStunDuration = 0.0f;

	TotalKnockdownDuration = 0.0f;

	HitStunCount = 0;

	KnockdownCount = 0;

	CurrentHitStunStartTime = -1.0f;

	CurrentKnockdownStartTime = -1.0f;

}







void AAGSDCharacter::ResetCameraLag()

{

	if (CameraBoom)

	{

		// 스프링 암의 이전 프레임 위치/회전 랙 메모리를 현재 위치로 즉시 덮어씌움

		const FVector CurrentArmOrigin = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;

		CameraBoom->PreviousArmOrigin = CurrentArmOrigin;

		CameraBoom->PreviousDesiredLoc = CurrentArmOrigin;

		CameraBoom->PreviousDesiredRot = CameraBoom->GetTargetRotation();



		// 월드 트랜스폼 즉시 갱신

		CameraBoom->UpdateComponentToWorld();

	}

}



bool AAGSDCharacter::TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest, bool bNoCheck)

{

	const bool bSuccess = Super::TeleportTo(DestLocation, DestRotation, bIsATest, bNoCheck);

	if (bSuccess && !bIsATest)

	{

		ResetCameraLag();

	}

	return bSuccess;

}

