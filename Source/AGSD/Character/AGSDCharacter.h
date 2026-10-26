// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "HoldingState.h"
#include "AGSDPlayerController.h"
#include "FadeWidget.h"
#include "HealthBar.h"
#include "HoldingWeapon.h"
#include "InputBufferEntry.h"
#include "PlayerStateWidget.h"
#include "SOVGameInstance.h"
#include "Interface/PetConversationInterface.h"
#include "Interface/PlayerDeadInterface.h"
#include "Interface/ItemDropInterface.h"
#include "Interface/InteractionInterface.h"
#include "InteractionOwnerInterface.h"
#include "SpearComboData.h"
#include "ECharacterState.h"
#include "TutorialTypes.h"
#include "CombatStatData.h"
#include "CombatLogParticipantInterface.h"
#include "EnemyProjectile/BaseEnemyProjectile.h"
#include "AGSDCharacter.generated.h"

class UAGSDComboGuideComponent;

USTRUCT(BlueprintType)
struct FEquipSocketMapping
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip Config")
	FString ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip Config")
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip Config")
	FName BackSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip Config")
	EHoldingWeapon HoldingWeaponState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip Config")
	bool bContainsCheck;

	FEquipSocketMapping()
		: ItemID(TEXT("")), SocketName(NAME_None), BackSocketName(NAME_None), HoldingWeaponState(EHoldingWeapon::None), bContainsCheck(false)
	{}

	FEquipSocketMapping(const FString& InItemID, const FName& InSocketName, const FName& InBackSocketName, EHoldingWeapon InState, bool bInContains = false)
		: ItemID(InItemID), SocketName(InSocketName), BackSocketName(InBackSocketName), HoldingWeaponState(InState), bContainsCheck(bInContains)
	{}
};

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class AACultivationPlot;
class UMotionWarpingComponent;
class UAGSDInventoryComponent;
class UAGSDLockOnComponent;
class UAGSDInteractionComponent;
class UAGSDGuardComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AGSD_API AAGSDCharacter : public ACharacter, public IPetConversationInterface, public IPlayerDeadInterface
	, public IItemDropInterface, public IInteractionInterface, public IInteractionOwnerInterface
	, public ICombatLogParticipantInterface
{
	GENERATED_BODY()

	friend class UAGSDGuardComponent;
	friend class UAGSDComboGuideComponent;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** Audio listener target component that follows character position but rotates with camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* AudioListenerComponent;
	
protected:
	/** Attack Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* AttackAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* SprintAction;

	/** Roll Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* RollAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* Interaction;

	// 가드 액션
	UPROPERTY(EditAnywhere, Category="Input")
	const UInputAction* GuardAction;

	// 락온 액션
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* LockOnAction;

	// 락온 타겟 전환 액션 (좌/우)
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* SwitchTargetLeftAction;

	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* SwitchTargetRightAction;

	// 핫바 휠 스크롤 액션
	UPROPERTY(EditAnywhere, Category="Input|Hotbar")
	class UInputAction* HotbarScrollAction;

	// 핫바 숫자키 선택 액션 (단일 액션에 Scalar Modifier가 적용된 형태)
	UPROPERTY(EditAnywhere, Category="Input|Hotbar")
	class UInputAction* SelectHotbarAction;

	// 인벤토리 창 개폐 토글 액션
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* ToggleInventoryAction;

	// 카메라 방향 바라보기 액션
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* FaceCameraAction;
	
	// 락온 시 유지할 목표 거리
	float TargetLockOnDistance = 0.0f;

	// 핫바 및 인벤토리 입력 핸들러
	void Input_HotbarScroll(const FInputActionValue& Value);
	void Input_SelectHotbar(const FInputActionValue& Value);
	void Input_ToggleInventory();

	// 모션 워핑 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Warping")
	class UMotionWarpingComponent* MotionWarpingComponent;

	// 모션 워핑 타겟 업데이트 함수
	UFUNCTION(BlueprintCallable, Category = "Combat|Warping")
	void UpdateMotionWarpTarget();

	UPROPERTY(EditAnywhere, Category = "Combat|Warping")
	float MaxAngleDiff = 90.f;

	// 후퇴 공격 시 뒤로 멀어질 모션 워핑 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Warping")
	float BackwardWarpDistance = 350.0f;

	// 락온 시 공격 전진 최대 워프 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Warping")
	float MaxWarpStep = 350.0f;

	// 비락온(소프트 락온) 시 공격 전진 최대 워프 거리 (하드 락온과 동일하게 350.0f 적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Warping")
	float SoftLockMaxWarpStep = 350.0f;

	// 공격 시 적 전방에 남겨둘 정지 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Warping")
	float AttackStopDistance = 150.0f;
	
	AAGSDPlayerController* PC;
	
	UPROPERTY(VisibleAnywhere, Category="Input Buffer")
	TArray<FInputBufferEntry> InputBuffer;

	UPROPERTY(EditDefaultsOnly, Category="Input Buffer")
	float InputBufferDuration = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, Category="Input Buffer")
	float InputDifference = 0.3f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input Buffer")
	float AttackBufferDuration = 0.2f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UFadeWidget> WBP_FadeWidget;

	UPROPERTY(BlueprintReadOnly)
	class UFadeWidget* FadeWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerState")
	int Coin;

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void AddCoin(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	bool ConsumeCoin(int32 Amount);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	bool bIsRecovering = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	bool bCanCombo = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	bool bCanRollCancel = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	bool bCanMoveCancel = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	bool bHasBufferedInput = false;

	// 선입력된 공격의 발생 시점 (유효 시간 체크용)
	float BufferedInputTime = 0.0f;

	// USTRUCT 포인터는 UPROPERTY로 관리할 수 없으므로 매크로 제거
	FSpearComboData* CurrentComboData;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	int32 CurrentStageIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerState")
	UDataTable* SpearComboDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="PlayerState")
	FStruct_ItemData HoldingItemData;
	
private:
	UFUNCTION(BlueprintCallable)
	void HandleAttackInput(FName ActionName);
	
public:
	/** Constructor */
	AAGSDCharacter();	

	FORCEINLINE FStruct_ItemData GetHoldingItemData() const { return HoldingItemData; }
	FORCEINLINE USceneComponent* GetAudioListenerComponent() const { return AudioListenerComponent; }

	// G 키 입력 및 외부 상호작용 위임 함수
	void TryInteract();
	float getDamage() const {return Damage;};

	UPROPERTY(EditAnywhere, Category="Input")
	class UInputMappingContext* IMC_Farmer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* SkipTutorialAction;

	// 튜토리얼 스킵이 활성화될 맵 이름들 (예: Tutorial_Village, Tutorial_Sky_Island)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial")
	TArray<FString> TutorialMapNames;

	// 튜토리얼 스킵 완료 시 이동할 타겟 맵 이름 (예: Farm_Sky_Island)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial")
	FName TargetSkipMapName;

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PlayerState")
	ECharacterState CharacterState = ECharacterState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HoldingState")
	EHoldingState HoldingState = EHoldingState::EHS_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HoldingState")
	EHoldingWeapon HoldingWeapon = EHoldingWeapon::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="HoldingState")
	AActor* HoldingActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HoldingState|Config")
	TArray<FEquipSocketMapping> EquipSocketMappings;

	// 무기 종류별 등 뒤/허리에 부착된 액터들을 관리할 맵 (예: Spear -> BackWeaponActor)
	UPROPERTY()
	TMap<EHoldingWeapon, AActor*> BackWeaponActors;

	// 등 뒤 무기 거치 상태를 동적으로 갱신하는 함수
	void UpdateBackWeapon();

	/** 상호작용(잡초 뽑기, 요리 등) 중 손에 든 무기/도구를 임시로 숨기거나 복원 */
	UFUNCTION(BlueprintCallable, Category = "HoldingState")
	void SetHoldingActorHidden(bool bNewHidden);

	// ── 플레이어 UI 및 HUD ──

	/** 생성할 PlayerHUD 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	TSubclassOf<class UAGSDPlayerHUD> PlayerHUDClass;

	/** 생성 및 캐싱된 PlayerHUD 레퍼런스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	class UAGSDPlayerHUD* PlayerHUDRef;

	// 일시정지 액션
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* PauseAction;

	/** 생성할 일시정지 메뉴 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	TSubclassOf<class UUserWidget> PauseMenuClass;

	/** 생성 및 캐싱된 일시정지 메뉴 위젯 레퍼런스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	class UUserWidget* PauseMenuWidgetRef;

	/** 현재 퍼즈 키로 닫을 수 있는 활성화된 UI 참조 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerUI")
	TWeakObjectPtr<class UUserWidget> ActiveCloseableUI;

	/** 현재 열려있는 상자 액터 레퍼런스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	class AChest* OpenedChest;

	// 일시정지 해제 UI
	UFUNCTION(BlueprintCallable, Category = "PlayerUI")
	void RemovePauseUI();

	// 액티브 UI 등록
	UFUNCTION(BlueprintCallable, Category = "PlayerUI")
	void RegisterCloseableUI(class UUserWidget* NewUI);

	// 액티브 UI 해제
	UFUNCTION(BlueprintCallable, Category = "PlayerUI")
	void UnregisterCloseableUI(class UUserWidget* UI);

	// ── 인벤토리 컴포넌트 (C++ 마이그레이션) ──

	/** C++ 인벤토리 컴포넌트 (BP에서 추가하거나 C++에서 CreateDefaultSubobject로 생성) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UAGSDInventoryComponent> InventoryComponent;

	/** 락온 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAGSDLockOnComponent> LockOnComponent;

	/** 상호작용 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAGSDInteractionComponent> InteractionComponent;

	/** 가드 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAGSDGuardComponent> GuardComponent;

	/** 콤보 가이드 UI 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAGSDComboGuideComponent> ComboGuideComponent;

	/** 핫바 선택 변경 시 장착 액터를 갱신합니다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateEquippedActor();

	/** 핫바 선택 변경 델리게이트 콜백 */
	UFUNCTION()
	void OnHotbarSelectionChanged(int32 PreviousIndex, int32 NewIndex);

	/** 인벤토리 슬롯 업데이트 콜백 */
	UFUNCTION()
	void OnInventorySlotUpdated(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FString SubItemAmount();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interaction")
	void StrongAttack();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interaction")
	void Attack();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FORCEINLINE void SetCanOpenChest(bool boolean) { bCanOpenChest = boolean; };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanOpenChest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool SkillMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Mining = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Weeding = false;

	void Die();
	UFUNCTION(BlueprintCallable)
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void FellOutOfWorld(const UDamageType& dmgType) override;

	/** 하늘섬 등에서 낙하하여 체크포인트로 복귀할 때 체력 페널티를 적용합니다. 최소 HP 1을 보장하여 즉사를 방지합니다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	float ApplyFallPenalty(float FallDamage = 80.0f);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog")
	void RecordFallRespawn();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Tutorial")
	void LogDialogueFullSkip();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Tutorial")
	void LogDialogueLineSkip();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Tutorial")
	void LogDialogueLineRead();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|UI")
	void LogDialogueLogRecheck();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|UI")
	void LogOptionChange();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Reward")
	void LogPotionUsage(const FString& PotionType);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|NPC")
	void LogNPCDialogue(const FString& NPCName);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Boss")
	void LogBossEnter(const FString& BossName);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Boss")
	void LogBossDeath(const FString& BossName);

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Boss")
	void LogBossClear(const FString& BossName, float BattleTimeSeconds = 0.f, int32 LootCount = 0);

	UFUNCTION(BlueprintPure, Category = "GameplayLog|Boss")
	bool IsBossBattleActive() const;

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Boss")
	void SetBossBattleActive(bool bActive, const FString& BossName = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Gimmick")
	void LogGimmickClear(const FString& GimmickName = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Gimmick")
	void LogBossPatternDodge();

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Export")
	bool LogExportCSV(const FString& FilePath = TEXT(""));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USOVGameInstance* GI;
	
	FORCEINLINE AAGSDPlayerController* getPlayerController() const {return PC;};
	FORCEINLINE USOVGameInstance* getPlayerGameInstance() const {return GI;};

	FORCEINLINE float getPlayerMaxhealth() const { return MaxHealth;}

	/** 전투 로그용 플레이어 현재 전투/행동 상태 문자열 반환 (JustGuarding, Guarding, Rolling, Attacking, AttackRecovery, Moving, Idle) */
	UFUNCTION(BlueprintPure, Category = "Combat|Log")
	FString GetCurrentCombatStateString() const;

	// ICombatLogParticipantInterface 구현
	virtual float GetParticipantHealth() const override { return Health; }
	virtual float GetParticipantMaxHealth() const override { return MaxHealth; }
	virtual FString GetParticipantCombatState() const override { return GetCurrentCombatStateString(); }
	virtual void InterruptCombatAction(bool bInterruptAttack, bool bInterruptGuard) override;
	virtual float GetParticipantHitStunDuration() const override;
	virtual float GetParticipantKnockdownDuration() const override;
	virtual int32 GetParticipantHitStunCount() const override { return HitStunCount; }
	virtual int32 GetParticipantKnockdownCount() const override { return KnockdownCount; }
	virtual void ResetParticipantStunMetrics() override;

	void SetCanCombo(bool b);
	void SetCanRollCancel(bool b);
	FORCEINLINE bool CanRollCancel() const { return bCanRollCancel; }
	void EnableMovementFromAttack();
	FORCEINLINE bool CanMoveCancel() const { return bCanMoveCancel; }
	
	FORCEINLINE bool HasBufferedInput() {return bHasBufferedInput;}

	/** 튜토리얼 서브시스템에 플레이어 행동을 보고하는 헬퍼 함수 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ReportTutorialAction(ETutorialActionType ActionType, int32 Count = 1);

public:
	/** 봉헌 데이터테이블 (DT_TributeData) - 단계별 스탯 및 요구치 바인딩 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Tribute")
	TObjectPtr<class UDataTable> TributeDataTable;

	/** 현재 플레이어의 봉헌 완료 단계 (0: 미봉헌 기본, 1~20: 봉헌 완료 단계) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Tribute")
	int32 CurrentTributeLevel = 0;

	/** 봉헌 데이터테이블 및 GameInstance로부터 스탯 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Tribute")
	void InitializeTributeStats();

	/** 특정 봉헌 완료 단계를 즉시 적용하는 치트/디버그 함수 (콘솔 창: ApplyTributeLevel 3 또는 SetTributeLevel 3) */
	UFUNCTION(Exec, BlueprintCallable, Category = "Combat|Tribute")
	void ApplyTributeLevel(int32 TargetCompletedLevel);

	UFUNCTION(Exec, Category = "Combat|Tribute")
	void SetTributeLevel(int32 TargetCompletedLevel) { ApplyTributeLevel(TargetCompletedLevel); }

	/** 제단 등에서 봉헌 1단계 성장을 진행할 때 호출 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Tribute")
	void AdvanceTributeLevel();
	
protected:
	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerState")
	float MaxHealth = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerState")
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState")
	float Damage = 100.0f;

	/** Kalivra 밸런싱 데이터테이블(DT_CombatStats) 행 바인딩 (설정 시 MaxHealth, Damage 등을 자동 초기화) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	FDataTableRowHandle CombatStatHandle;

	/** 데이터테이블로부터 플레이어 기본 스탯을 읽어와 초기화합니다 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void InitializeCombatStatsFromDataTable();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement")
	float WalkSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement")
	float SprintSpeed = 600.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState|Movement")
	bool bIsSprinting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState|Movement")
	float DesiredSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState|Movement|Roll")
	bool bIsRolling = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement|Roll")
	float RollCooldown = 0.9f;

	FTimerHandle RollTimerHandle;

	/** 구르기 회피 무적 지속 시간 (60fps 기준 40프레임 = 약 0.667초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement|Roll", meta = (ToolTip = "구르기 시작 시 무적이 유지되는 시간 (초)"))
	float RollInvincibleDuration = 40.0f / 60.0f;

	/** 현재 구르기 무적 상태 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState|Movement|Roll")
	bool bIsRollInvincible = false;

	FTimerHandle RollInvincibleTimerHandle;

	void EndRollInvincible();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState|Movement|Roll")
	float InputRollDirection = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement|Camera")
	float DefaultCameraLagSpeed = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState|Movement|Animation")
	float TurnYawDelta = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState|Movement|Animation")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState|Movement|Animation")
	float AimPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState|Movement|Animation")
	float AimOffsetAlpha = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState|Movement|Camera")
	bool bIsFaceCameraPressed = false;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState|Movement|LockOn")
	bool bIsLockedOn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement|Animation")
	class UAnimMontage* TurnLeft90Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement|Animation")
	class UAnimMontage* TurnRight90Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement|Animation")
	float TurnThresholdAngle = 45.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState|Movement|Animation")
	bool bIsTurning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerState|Movement|Animation")
	float TurnDuration = 0.75f;

	float TurnTimer = 0.0f;
	FRotator StartRotation;

	void UpdateCharacterRotationSettings();
	void TryStartTurn();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState")
	bool bCanBeDamage = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	class UHealthBar* HealthBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	TSubclassOf<class UHealthBar> WBP_HealthBar;

	FORCEINLINE float getHealth() const {return Health;};
	
	// 선입력 및 방향 판정을 위한 원시 입력 벡터 (이동 제한 상태에서도 업데이트됨)
	FVector LastRawInputVector = FVector::ZeroVector;

	// 핫바 중복 입력 방지를 위한 이전 키 인덱스
	int32 LastHotbarInputIndex = -1;

	// 일시정지 처리 함수
	void Input_Pause();

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	void StopMove();
	
	virtual void Jump() override;

	
	virtual void StopJumping() override;

	void SprintStart();
	void SprintEnd();
	void UpdateSprintSpeed();

	UFUNCTION(BlueprintCallable, Category = "Action|Roll")
	void StartRoll();

	UFUNCTION(BlueprintCallable, Category = "Action|Roll")
	void StopRoll();
	void FaceCameraInput(const FInputActionValue& Value);
	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void ConsumeInputs(TArray<FInputBufferEntry>& Buffer, int32 Count);
	bool CheckCombo(
	const TArray<FInputBufferEntry>& Buffer, 
	FName Input1, 
	FName Input2, 
	float MaxTimeBetweenInputs);

	// AAGSDCharacter.h에 추가
	bool CheckSingleInput(const TArray<FInputBufferEntry>& Buffer, FName InputName);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "PlayerUI")
	UPlayerStateWidget* PlayerStateWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* DyingMontage;

	UFUNCTION()
	void WakeUp();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAudioComponent* Running;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAudioComponent* Jumping;

	UFUNCTION(BlueprintCallable)
	void playFadeWidget(float startOpacity, float endOpacity, float fadeSpeed = 2.0f);

public:
	// Enemy 로그 처리 위한 겟 함수
	FORCEINLINE float GetFarmerHealthToEnemy() const { return Health; }

	void OnRecoveryFinished(UAnimMontage* Montage, bool bInterrupted);
	void ResetAttackState();
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float MouseSensitivity = 5.0f;
	
	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "PlayerState")
	void TeleportToBed();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pet")
	bool bHasPet = false;
	
	UPROPERTY(BlueprintReadWrite, Category="Pet")
	class ABaseFlyingPet* Pet;

	UFUNCTION(BlueprintPure, Category = "Pet")
	FORCEINLINE class ABaseFlyingPet* GetPet() const { return Pet; }

	UFUNCTION(BlueprintPure, Category = "Pet")
	bool IsInDialogue() const;

	UFUNCTION(BlueprintCallable, Category = "Pet")
	void SetPet(class ABaseFlyingPet* NewPet);

	// 에디터의 Details 패널에서 어떤 펫 블루프린트를 쓸지 선택하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pet")
	TSubclassOf<class ABaseFlyingPet> DefaultPetClass;

	// 인터페이스 함수 구현
	virtual void SetMyPet_Implementation(AActor* NewPet) override;
	virtual void MasterToPetBigConversation_Implementation(FName DialogueID) override;

	// 레벨 이동 전 펫 파괴 및 이동 후 펫 스폰 함수
	UFUNCTION(BlueprintCallable)
	void DestroyPetBeforeTravel();
	UFUNCTION(BlueprintCallable)
	void SpawnMyPetAfterTravel();
	
	// 로그 관련 인터페이스 함수 구현
	UPROPERTY()
	FOnPlayerDeadSignature OnPlayerDead;
	virtual FOnPlayerDeadSignature& ReturnOnPlayerDeadDelegate() override { return OnPlayerDead; }
	
	// 락온 상태 변경 인터페이스 구현
	UPROPERTY()
	FOnLockOnStateChanged OnLockOnStateChanged;
	virtual FOnLockOnStateChanged& GetLockOnStateChangedDelegate() override { return OnLockOnStateChanged; }
	
	virtual void StopAnimMontage(UAnimMontage* AnimMontage = nullptr) override;

	/** 피격 넉백 등 루트 모션을 즉시 강제 중단해야 하는 특수 상황 전용 정지 함수 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void StopAnimMontageImmediate(float BlendOutTime = 0.0f);

	/** 피격 넉백 시 공격 모션 및 루트모션이 넉백 궤적을 방해하지 않도록 오버라이드 */
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride) override;
	
	UFUNCTION()
	void HandleLockOn(bool bLockOn);
	
	// 적이 드롭할 아이템 데이터 테이블
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemDrop")
	class UDataTable* EnemyDropDataTable;
	virtual void HandleEnemyDeadAndDropItem_Implementation( AActor* DeadActor ) override;
	//--
	// 펫 관련 추가
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	float WeaponDamage();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void WeaponAttack();

	UFUNCTION()
	void AddDamage(float addDamage);

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void HealthRecovery(float amount);

	// ── 글로벌 포션 공유 쿨다운 시스템 ──

	/** 모든 포션이 공유하는 기본 쿨다운 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Potion")
	float GlobalPotionCooldown = 5.0f;

	/** 마지막으로 포션을 사용한 시점의 월드 시간(초) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Potion")
	float LastGlobalPotionUseTime = -100.0f;

	/** 포션 쿨다운 진행 비율 반환 (1.0 = 방금 마심, 0.0 = 쿨다운 완료) */
	UFUNCTION(BlueprintPure, Category = "Combat|Potion")
	float GetPotionCooldownRatio() const;

	/** 포션 쿨다운 잔여 시간(초) 반환 */
	UFUNCTION(BlueprintPure, Category = "Combat|Potion")
	float GetPotionCooldownRemaining() const;

	/** 포션이 현재 쿨다운 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "Combat|Potion")
	bool IsPotionOnCooldown() const;

	/** 아이템 데이터가 포션 종류인지 판별하는 헬퍼 함수 */
	UFUNCTION(BlueprintPure, Category = "Combat|Potion")
	bool IsPotionItem(const FStruct_ItemData& ItemData) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetCurrentAttackDamageMultiplier() const;

	ESpearAttackDirection GetAttackDirection();

	UFUNCTION(BlueprintCallable)
	void ProcessAttackInput();

	/** 현재 장착된 아이템을 사용합니다. 무기면 공격, 포션이면 포션 사용 인터페이스를 실행합니다. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void UseEquippedItem();

	/** 보조/특수 공격 입력 (우클릭 RMB) 핸들러 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Input_SecondaryAttack();
	
	//-----------------------------------
	void ActivateAttackRotate();
	void HandleRotateCharacterStartAttack(float DeltaSeconds);
	bool bIsRotatingToCamera = false;
	FRotator TargetAttackRotation;
	float RotationTimer = 0.0f;
	float MaxRotationTime = 0.2f;   
	float RotationSpeed = 500.0f;

	/** 저스트가드 성공 후 반격 공격 중 슈퍼아머 활성화 플래그 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Parry")
	bool bIsJustGuardCounterActive = false;

	/** 저스트가드 성공 시각 (초 단위) */
	UPROPERTY()
	float LastJustGuardSuccessWorldTime = -999.0f;

	/** 저스트가드 후속 슈퍼아머 유효 제한 시간 (초, 기본값 3.0초) */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Parry")
	float JustGuardSuperArmorDuration = 3.0f;

	/** 가장 최근에 공격을 튕겨낸 공격자 (노락온 반격 타겟팅용) */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LastParryAttacker;
	
	void StartParryCombo(AActor* Attacker = nullptr);
	void StartNewCombo();
	void StartNewComboWithInput(ESpearAttackInput Input);
	
	FSpearComboData* GetComboDataByDirection(ESpearAttackDirection Direction);
	FSpearComboData* GetComboDataByDirectionAndInput(ESpearAttackDirection Direction, ESpearAttackInput Input);
	
	void ProcessAttackInputWithButton(ESpearAttackInput PressedInput);

	// 마우스 우클릭(RMB) 보조/특수 공격 액션
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* SecondaryAttackAction;

	/** 좌/우클릭 동시 입력 유효 시간 범위 (초 단위, 에디터 및 블루프린트 조정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input Buffer")
	float SimultaneousInputWindow = 0.1f;
	
	// 콤보 결정 대기 타이머 핸들 및 상태
	FTimerHandle ComboInputDecisionTimerHandle;
	ESpearAttackInput PendingComboInput = ESpearAttackInput::LMB;
	bool bIsWaitingForComboDecision = false;

	void OnComboDecisionTimeout();
	void ConfirmAndExecuteCombo(ESpearAttackInput DeterminedInput);
	
	// 선입력된 입력 버튼 종류
	ESpearAttackInput BufferedInput = ESpearAttackInput::LMB;
	
	// ComboWindow 내 입력 수집용 상태 변수
	bool bLMBPressedInWindow = false;
	bool bRMBPressedInWindow = false;

	void ResetComboWindowBuffer();
	void OnComboWindowEnd();

	void ExecuteNextStage();
	void ExecuteNextStageWithInput(ESpearAttackInput Input);
	void TryExecuteBufferedAttack();

	void PlayStage(int32 Index);
	
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void StartRecovery(UAnimMontage* RecoveryMontage);

	void OnHitReceived(AActor* DamageCauser = nullptr);

	void ResetCombo();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ShouldApplyHitReaction() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Tutorial")
	void OnSkipTutorialTriggered();
	
	void SkipTutorialPressed();
	
	UFUNCTION()
	void ExecuteTutorialSkipLevelTransition();

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void SetCharacterState(ECharacterState NewState);

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	FORCEINLINE UAGSDLockOnComponent* GetLockOnComponent() const { return LockOnComponent; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual UAGSDInteractionComponent* GetInteractionComponent() const override { return InteractionComponent; }

	UFUNCTION(BlueprintCallable, Category = "Guard")
	FORCEINLINE UAGSDGuardComponent* GetGuardComponent() const { return GuardComponent; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FORCEINLINE UAGSDComboGuideComponent* GetComboGuideComponent() const { return ComboGuideComponent; }

	void UpdateCharacterStateFromEquip();
	
	// 로그 데이터 관련
	FVector LastLocation;
	float TotalDistance = 0.0f;
	void SetTotalDistance();
	
	float GetTotalDistance() const { return TotalDistance / 100.0f; } // 미터(m) 단위 변환 시

	// ─── [로그 데이터: 세분화된 행동 소요 시간 측정] ───
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameplayLog|Action")
	FString CurrentActionCategory = TEXT("");

	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Action")
	void SetCurrentActionCategory(const FString& NewCategory);

	// NPC 상점 / 대장간 / 기능 UI 이용 상태 설정 헬퍼 (bActive: true 시작, false 종료)
	UFUNCTION(BlueprintCallable, Category = "GameplayLog|Action")
	void SetActionNPCService(bool bActive);

	// ─── [피즈 재간둥이형 모션 등 충돌 및 무적 제어 헬퍼] ───
	UFUNCTION(BlueprintCallable, Category = "Combat|Collision")
	void SetIgnorePawnCollision(bool bIgnore);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetCanBeDamaged(bool bInCanBeDamage);

	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE bool GetCanBeDamaged() const { return bCanBeDamage; }

protected:
	void UpdateActionDurationLogging(float DeltaSeconds);

	// 노티파이 등에 의해 충돌 무시 및 무적이 적용 중인지 추적하는 플래그
	bool bIsCollisionIgnoredByNotify = false;
	bool bIsInvulnerableByNotify = false;

	// 노티파이 도중 충돌을 무시한 타겟 액터 캐싱
	TWeakObjectPtr<AActor> IgnoredTargetActorOnNotify = nullptr;

public:
	// ─── [로스트아크식 3단계 피격 리액션 시스템] ───
	/** 피격 상대 각도 (-180.0 ~ +180.0도, 전방: 0, 후방: +-180, 좌: -90, 우: +90) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|HitReaction")
	float HitDirectionAngle = 0.0f;

	/** 최근 피격 대미지 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|HitReaction")
	float LastHitDamage = 0.0f;

	/** 피격 연출 강도 (0.0 = 약피격 움찔, 1.0 = 강피격 비틀거림, 2D 블렌드스페이스 Y축용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|HitReaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitIntensityValue = 0.0f;

	/** 애님그래프 Blend Poses by bool 연동용 피격 플래그 (0.833초간 On, 회피 키로 캔슬 가능) */
	/** 애님그래프 Blend Poses by bool 연동용 피격 플래그 (0.833초간 On, 회피 키로 캔슬 가능) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|HitReaction")
	bool bIsHitReacting = false;

	/** 통합 피격 리액션 지속 시간 (기본값: 0.833초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitReaction")
	float HitReactDuration = 0.833f;

	/** 넉다운(다운/무릎 꿇기) 활성화 플래그 (애님그래프 스테이트 머신 진입 조건) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|HitReaction")
	bool bIsKnockdown = false;

	/** 현재 적용된 넉다운 타입 (None = 없음, Kneel = 무릎 꿇기, AirborneFloor = 에어본 후 바닥 엎어짐) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|HitReaction")
	EKnockdownType CurrentKnockdownType = EKnockdownType::None;

	/** 넉다운 기상 트리거 (스킬별 넉다운 시간 만료 시 true로 변경되어 기상 End 상태로 전이) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|HitReaction")
	bool bKnockdownStandUp = false;

	/** 무기 착용 여부 (애님그래프에서 무기용 vs 맨손용 모션 분기 시 사용) */
	UFUNCTION(BlueprintPure, Category = "Combat|Equipment")
	bool HasEquippedWeapon() const { return (HoldingWeapon == EHoldingWeapon::Spear || HoldingWeapon == EHoldingWeapon::Sickle); }

	/** 피격 처리 메인 진입점 */
	void ProcessHitReaction(float DamageAmount, AActor* DamageCauser, struct FDamageEvent const& DamageEvent);
	void ResetHitReact();
	void TriggerKnockdownStandUp();

	/** 넉다운 기상 완료 처리 (애님 노티파이 또는 안전 타이머에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Combat|Knockdown")
	void FinishKnockdown();

	void ApplyHitStun(float Duration);
	void ClearHitStun();

	// ─── [전투 로그용 경직 계측 변수 및 헬퍼 (초 단위 및 횟수)] ───
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Log")
	float TotalHitStunDuration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Log")
	float TotalKnockdownDuration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Log")
	int32 HitStunCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Log")
	int32 KnockdownCount = 0;

	float CurrentHitStunStartTime = -1.0f;
	float CurrentKnockdownStartTime = -1.0f;

	void StartHitStunTracking();
	void EndHitStunTracking();
	void StartKnockdownTracking();
	void EndKnockdownTracking();

	// ─── [디버그/테스트용 무적 치트 (God Mode)] ───
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Debug")
	bool bIsGodMode = false;

	/** 콘솔창(~)에서 'God' 또는 'ToggleGodMode' 입력 시 On/Off 토글 */
	UFUNCTION(Exec, BlueprintCallable, Category = "Combat|Debug")
	void ToggleGodMode();

	UFUNCTION(Exec, Category = "Combat|Debug")
	void God() { ToggleGodMode(); }

protected:
	/** 넉다운 기본 유지 시간 (스킬에서 설정되지 않았을 때의 기본값, 초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitReaction")
	float KnockdownDuration = 1.2f;

	FTimerHandle HitStunTimerHandle;
	FTimerHandle HitReactTimerHandle;
	FTimerHandle KnockdownTimerHandle;
};
