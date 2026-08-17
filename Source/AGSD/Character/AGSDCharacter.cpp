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
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Interaction.h"
#include "Struct_ItemData.h"
#include "UsableItem.h"
#include "AGSD.h"
#include "FadeWidget.h"
#include "HeartProgressBar.h"
#include "PlayerStateWidget.h"
#include "MotionWarpingComponent.h"
#include "SOVGameInstance.h"
#include "Components/ProgressBar.h"
#include "Kismet/GameplayStatics.h"
#include "BaseFlyingPet.h"
#include "NiagaraFunctionLibrary.h"
#include "SpearComboData.h"
#include "TextLog.h"
#include "Components/AudioComponent.h"
#include "Interface/ItemDropInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
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
	UTextLog::WriteTextLogByString(TEXT("상호작용 키 입력"), TEXT("단순 상호작용"));
	// 무기(Spear 등)를 장착 중이거나 공격 진행 중일 경우 우클릭(RMB) 콤보/스킬 신호 전송
	if (HoldingWeapon == EHoldingWeapon::Spear || HoldingWeapon == EHoldingWeapon::Sickle || bIsAttacking)
	{
		ProcessAttackInputWithButton(ESpearAttackInput::RMB);
	}

	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
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

	SetTotalDistance();

	if (ComboGuideComponent)
	{
		ComboGuideComponent->UpdateComboGuideUI();
	}

	UpdateCharacterRotationSettings();
	TryStartTurn();

	// 턴이 활성화된 상태일 때 설정한 지속 시간 동안 마우스 회전을 향해 쿼터니언 Slerp 보간 회전
	if (bIsTurning)
	{
		// 회전 중 이동을 시작하면 턴을 즉시 종료하고 일반 이동 회전으로 전환
		if (GetCharacterMovement() && GetCharacterMovement()->Velocity.SizeSquared2D() > 100.0f)
		{
			bIsTurning = false;
			TurnTimer = 0.0f;
		}
		else
		{
			TurnTimer -= DeltaSeconds;
			if (TurnTimer <= 0.0f)
			{
				bIsTurning = false;
				TurnTimer = 0.0f;
			}
			else if (GetController() && TurnDuration > 0.0f)
			{
				// 경과 시간에 따른 보간 비율 Alpha 계산 (0.0 ~ 1.0)
				float Alpha = FMath::Clamp((TurnDuration - TurnTimer) / TurnDuration, 0.0f, 1.0f);

				FRotator TargetRot = GetControlRotation();
				TargetRot.Pitch = 0.0f;
				TargetRot.Roll = 0.0f;

				// 쿼터니언을 이용해 최단 경로로 구면 선형 보간 (Slerp)
				FQuat StartQuat = FQuat(StartRotation);
				FQuat TargetQuat = FQuat(TargetRot);
				FQuat NewQuat = FQuat::Slerp(StartQuat, TargetQuat, Alpha);

				SetActorRotation(NewQuat);
			}
		}
	}

	if (GetController())
	{
		float TargetYaw = 0.0f;
		AActor* TargetActor = LockOnComponent ? LockOnComponent->GetLockedTarget() : nullptr;
		if (TargetActor)
		{
			FVector TargetLocation = TargetActor->GetActorLocation();
			FVector CharacterLocation = GetActorLocation();
			FRotator LookAtRotation = (TargetLocation - CharacterLocation).Rotation();
			TargetYaw = LookAtRotation.Yaw;
		}
		else
		{
			TargetYaw = GetController()->GetControlRotation().Yaw;
		}
		float ActorYaw = GetActorRotation().Yaw;
		TurnYawDelta = FRotator::NormalizeAxis(TargetYaw - ActorYaw);
	}
	else
	{
		TurnYawDelta = 0.0f;
	}

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

	// --- [상호작용 업데이트] ---
	if (InteractionComponent)
	{
		InteractionComponent->UpdateInteractionState(DeltaSeconds);
	}
}

void AAGSDCharacter::BeginPlay()
{
	Super::BeginPlay();

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

	PC = Cast<AAGSDPlayerController>(GetController());
	
	if (PC && AudioListenerComponent)
	{
		// 카메라 거리에 따른 오디오 감쇠 문제를 해결하기 위해 오디오 리스너를 AudioListenerComponent로 설정합니다.
		// 이 컴포넌트는 위치는 캐릭터를 따르고 회전은 카메라 회전으로 매 프레임 업데이트됩니다.
		PC->SetAudioListenerOverride(AudioListenerComponent, FVector::ZeroVector, FRotator::ZeroRotator);
	}

	GI = Cast<USOVGameInstance>(GetGameInstance());

	if (GI)
	{
		bHasPet = GI->bHasPet;
		Coin = GI->Coin;
		Damage = GI->Damage;
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
	}

	if (HealthBar)
	{
		if (GI && GI->MaxPlayerHealth != 0.f)
		{
			Health = GI->PlayerHealth;
			MaxHealth = GI->MaxPlayerHealth;
		}
		HealthBar->HealthProgressBar->SetPercent(Health / MaxHealth);
	}
	playFadeWidget(1.0f, 0.0f);
	
	MouseSensitivity = GI->MouseSensitivity;

	// 시작 시 등 뒤 무기 거치 상태 초기 업데이트
	UpdateBackWeapon();

	// 게임 시작 시 현재 선택된 핫바 슬롯의 아이템을 즉시 장착합니다.
	UpdateEquippedActor();

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

	Super::EndPlay(EndPlayReason);

	GI->Damage = Damage;
	GI->Coin = Coin;
	GI->PlayerHealth = Health;
	GI->MaxPlayerHealth = MaxHealth;
	GI->bHasPet = bHasPet;
}



void AAGSDCharacter::WakeUp()
{
	FadeWidget->OnFadeFinished.RemoveDynamic(this, &AAGSDCharacter::WakeUp);
	HealthRecovery(10.f);
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
	if (DamageAmount > 0.f && !bCanBeDamage) return 0.f;
	float DamageToApply = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if ( DamageToApply < 0.f )
	{
		Health = FMath::Clamp(Health - DamageToApply, 0.0f, MaxHealth);
		
		if (HealthBar && HealthBar->HealthProgressBar) 
		{
			HealthBar->HealthProgressBar->SetPercent(Health / MaxHealth);
		}
		return DamageToApply;
	}
	
	if (DamageToApply >= 0.f)
	{
		OnHitReceived();
	}
	
	if (GuardComponent)
	{
		float MitigatedDamage = DamageToApply;
		if (GuardComponent->ProcessDamageMitigation(DamageToApply, MitigatedDamage))
		{
			DamageToApply = MitigatedDamage;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Player Take Damage : %f"), DamageToApply);

	if ( DamageToApply > 0.f )
	{
		Health = FMath::Clamp(Health - DamageToApply, 0, MaxHealth);
		if (HealthBar) HealthBar->HealthProgressBar->SetPercent(Health / MaxHealth);
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

			bCanBeDamage = false;
			Die();
		}
	}
	
	return DamageToApply;
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

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AAGSDCharacter::StopMove);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAGSDCharacter::Look);

		// 우클릭(IA_Interaction) 누르는 순간 TryInteract 호출하도록 바인딩
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

	// 선입력 판정을 위한 원시 입력 벡터 저장 (Mining 중에도 업데이트하여 애니메이션 중 방향 감지 가능)
	if (GetController())
	{
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		LastRawInputVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) * MovementVector.Y + 
							 FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * MovementVector.X;
	}

	// 움직이면 안될 때
	if (Mining)	return;
	
	UpdateSprintSpeed();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AAGSDCharacter::StopMove()
{
	LastRawInputVector = FVector::ZeroVector;
	UpdateSprintSpeed();

	if (Running && Running->IsPlaying())
	{
		Running->Stop();
	}
}

void AAGSDCharacter::Jump()
{
	if (Mining) return;
	if (CharacterState == ECharacterState::Block) return;
 	if (Jumping && !GetCharacterMovement()->IsFalling())
	{
		Jumping->Play();
		UTextLog::WriteTextLogByKeyword(TEXT("점프"));
	}
	Super::Jump();
}

void AAGSDCharacter::StopJumping()
{
	if (Mining) return;
	Super::StopJumping();
}

void AAGSDCharacter::SprintStart()
{
	bIsSprinting = true;
	UpdateSprintSpeed();
}

void AAGSDCharacter::SprintEnd()
{
	bIsSprinting = false;
	UpdateSprintSpeed();
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

	if (bIsSprinting && !bIsMovingBackward)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
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
	// 인벤토리, 상자 등 UI가 열려있는 동안에는 무시합니다.
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
	// 공중 상태 체크
	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling()) return;

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

	// 2. 캐릭터 회전 및 콤보 연계 실행
	float CurrentTime = GetWorld()->GetTimeSeconds();
	ActivateAttackRotate();

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

	// 인벤토리, 상자 등 UI가 열려있는 동안에는 무기 공격이나 아이템 사용이 나가지 않도록 차단합니다.
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
			IUsableItem::Execute_UseItem(DefaultActor, this);
			bUsed = true;
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
	// 가드(Block) 상태 중에는 우클릭 일반 공격 차단
	if (CharacterState == ECharacterState::Block)
	{
		return;
	}

	// 인벤토리, 상자 등 UI가 열려있는 동안에는 우클릭 공격 차단
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

	if (HoldingWeapon == EHoldingWeapon::Spear || HoldingWeapon == EHoldingWeapon::Sickle)
	{
		ProcessAttackInputWithButton(ESpearAttackInput::RMB);
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

void AAGSDCharacter::StartParryCombo()
{
	// 1. 가드 상태 해제 (패리 성공했으므로 공격(Combat) 상태로 전이)
	if (GuardComponent)
	{
		GuardComponent->StopBlock();
		GuardComponent->ResetJustGuardSuccess();
	}
	else if (CharacterState == ECharacterState::Block)
	{
		SetCharacterState(ECharacterState::Combat);
		UpdateSprintSpeed();
		UpdateCharacterRotationSettings();
	}

	// 2. 기존 공격/콤보 상태 강제 클리어
	ResetCombo();
	Mining = true; // 공격 시작 표시

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
	if (Mining) return;
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

	// 방향과 입력 요구사항이 모두 일치하는 항목 검색
	for (FSpearComboData* ComboData : AllRows)
	{
		if (ComboData && ComboData->DirectionRequirement == Direction && ComboData->InputRequirement == Input)
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
		// 방향 키 입력이 변경되어 현재 콤보 요구 방향과 다를 경우 해당 방향 및 버튼에 어울리는 새로운 콤보 1타로 전환 (Parry 콤보 진행 중일 경우 제외)
		if (CurrentComboData->DirectionRequirement != ESpearAttackDirection::Parry &&
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
	bHasBufferedInput = false;
	Mining = false;
    
	// 2. 콤보 데이터 초기화
	CurrentStageIndex = -1;
	CurrentComboData = nullptr;

	// 3. (추가) 수비 시스템 관련 상태도 함께 초기화
	if (GuardComponent)
	{
		GuardComponent->ResetGuardState();
	}

	// 몽타주가 실행 중이라면 정지 (선택 사항)
	// GetMesh()->GetAnimInstance()->StopAllMontages(0.2f);

	UE_LOG(LogTemp, Log, TEXT("Combo System Reset"));
}

void AAGSDCharacter::OnHitReceived()
{
	if (GuardComponent && GuardComponent->IsJustGuardActive())
	{
		GuardComponent->HandleJustGuardSuccess();
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
	bHasBufferedInput = false;
	CurrentStageIndex = -1;
	CurrentComboData = nullptr;
	Mining = false;

	if (ComboGuideComponent)
	{
		ComboGuideComponent->UpdateComboGuideUI();
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

	AActor* TargetActor = LockOnComponent ? LockOnComponent->GetLockedTarget() : nullptr;
	if (!TargetActor)
	{
		// 락온 타겟이 없을 때 모션 워프 타겟 제거
		MotionWarpingComponent->RemoveWarpTarget(FName("WarpTarget"));
		return;
	}

		FVector PlayerLoc = GetActorLocation();
		FVector TargetLoc = TargetActor->GetActorLocation();

		// 1. 플레이어 캐릭터 전방 120도 시야 범위(좌우 60도) 내에 타겟이 있는지 검사
		FVector Forward = GetActorForwardVector();
		FVector DirToTarget = (TargetLoc - PlayerLoc).GetSafeNormal2D();
		float Dot = FVector::DotProduct(Forward, DirToTarget);
		float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(Dot));

		if (AngleDiff > MaxAngleDiff)
		{
			// 정면 시야각 120도를 벗어났다면(예: 뒤를 돌아서 공격 시 등), 모션 워프 타겟을 제거하여 급회전 현상 방지
			MotionWarpingComponent->RemoveWarpTarget(FName("WarpTarget"));
			return;
		}

		// 2. 플레이어 캐릭터가 락온 타겟을 바라보는 회전값 계산
		FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLoc, TargetLoc);
		TargetRotation.Pitch = 0.f;
		TargetRotation.Roll = 0.f;

		// 수평(2D) 방향 및 거리 계산
		float ActualDistance = FVector::Dist2D(PlayerLoc, TargetLoc);

		FVector WarpLocation = PlayerLoc;
		const float AttackStopDistance = 150.0f; // 몬스터 전방 150.0f 남겨두기
		const float MaxWarpStep = 350.0f;        // 공격 시 순간이동 방지를 위한 최대 워프 전진 제한 거리

		if (ActualDistance > AttackStopDistance)
		{
			// 몬스터 전방 150.0f가 되는 목표 지점 계산
			FVector DesiredWarpLoc = TargetLoc - (DirToTarget * AttackStopDistance);
			
			// 해당 목표 지점까지의 워프 필요 거리
			float WarpDist = FVector::Dist2D(PlayerLoc, DesiredWarpLoc);
			if (WarpDist > MaxWarpStep)
			{
				// 최대 허용치만큼만 앞으로 전진 워프
				WarpLocation = PlayerLoc + (DirToTarget * MaxWarpStep);
			}
			else
			{
				WarpLocation = DesiredWarpLoc;
			}
		}
		else
		{
			// 이미 공격 사거리 이내(150.0f 이하)에 있다면 현재 위치 유지하며 회전만 보정
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
	// 인벤토리, 상자 등 UI가 열려있는 동안에는 마우스 드래그에 의한 카메라 회전(화면 돌아감)을 무시합니다.
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

void AAGSDCharacter::HealthRecovery(float amount)
{
	Health = FMath::Clamp(Health + amount, 0, MaxHealth);
	if (HealthBar) HealthBar->HealthProgressBar->SetPercent(Health / MaxHealth);
}
//--------------

void AAGSDCharacter::SetMyPet_Implementation(AActor* NewPet)
{
	if (NewPet)
	{
		Pet = Cast<ABaseFlyingPet>(NewPet);
		bHasPet = true; // 이제 펫이 있다고 표시
	}
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

		GetWorld()->SpawnActor<ABaseFlyingPet>(DefaultPetClass, SpawnLocation, SpawnRotation, SpawnParams);
	}
}

// C:\Users\YJH\Documents\GitHub\AGSD\Source\AGSD\AGSDCharacter.cpp

void AAGSDCharacter::HandleEnemyDeadAndDropItem_Implementation(AActor* DeadActor)
{
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

			// 5. 결정된 개수(FinalDropAmount)만큼 실제로 스폰시키기
			for (int32 i = 0; i < FinalDropAmount; ++i)
			{
				TSubclassOf<AActor> ItemClassToSpawn = ItemInfo.DropItemClass;
				
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

void AAGSDCharacter::StopAnimMontage(UAnimMontage* AnimMontage)
{
	Super::StopAnimMontage(AnimMontage);
	
	//ResetAttackState();
	bIsAttacking = false;
	bIsRecovering = false;
	bCanCombo = false;
	bHasBufferedInput = false;
	CurrentStageIndex = -1;
	CurrentComboData = nullptr;
	Mining = false;
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

// ═══════════════════════════════════════════════════
// 인벤토리 - 장착 액터 갱신
// ═══════════════════════════════════════════════════

void AAGSDCharacter::OnHotbarSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
	UpdateEquippedActor();
}

void AAGSDCharacter::OnInventorySlotUpdated(int32 SlotIndex)
{
	// 인벤토리 상태 변경에 따라 등 뒤 무기를 먼저 업데이트하여 에셋을 정상 스폰 및 동기화합니다.
	UpdateBackWeapon();

	// 현재 선택된 핫바 슬롯의 데이터가 변경된 경우 장착 아이템 갱신
	if (InventoryComponent && SlotIndex == InventoryComponent->GetCurrentHotbarIndex())
	{
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
}

void AAGSDCharacter::Input_HotbarScroll(const FInputActionValue& Value)
{
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
		UE_LOG(LogTemp, Log, TEXT("RegisterCloseableUI - Successfully registered: %s"), *NewUI->GetName());
	}
}

void AAGSDCharacter::UnregisterCloseableUI(UUserWidget* UI)
{
	if (ActiveCloseableUI.Get() == UI)
	{
		ActiveCloseableUI = nullptr;
		UE_LOG(LogTemp, Log, TEXT("UnregisterCloseableUI - Successfully unregistered"));
	}
}

void AAGSDCharacter::UpdateCharacterRotationSettings()
{
	if (!GetCharacterMovement()) return;

	bool bIsLockedOn = LockOnComponent && LockOnComponent->IsTargetLocked();

	if (bIsLockedOn || bIsFaceCameraPressed || CharacterState == ECharacterState::Block)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = bIsLockedOn;
	}
	else
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
	}
}

void AAGSDCharacter::TryStartTurn()
{
	bool bIsLockedOn = LockOnComponent && LockOnComponent->IsTargetLocked();

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

// ─── [로그 데이터 관련: 이동 거리 측정] ───
void AAGSDCharacter::SetTotalDistance()
{
	FVector CurrentLocation = GetActorLocation();

	float FrameDistance = FVector::Distance(CurrentLocation, LastLocation);

	if (FrameDistance > 0.1f)
	{
		TotalDistance += FrameDistance;
		LastLocation = CurrentLocation;
	}
}
