// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Interaction.h"
#include "SOVShortcutInteractComponent.generated.h"

class AAGSDCharacter;
class AAGSDPlayerController;
class USOVGameInstance;
class USOVShortcutInteractComponent;

/**
 * 부모 액터가 IInteraction 인터페이스를 직접 구현하지 않은 경우,
 * 플레이어의 AGSDInteractionComponent와 통신하기 위해 내부적으로 사용하는 가벼운 프록시 액터입니다.
 */
UCLASS()
class AGSD_API ASOVInteractionProxyActor : public AActor, public IInteraction
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USOVShortcutInteractComponent> OwnerComponent;

	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;
	virtual FString GetInteractionActionType_Implementation(AAGSDCharacter* player) override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShortcutUnlockedDelegate, AAGSDCharacter*, UnlockingPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShortcutAlreadyUnlockedDelegate, bool, bWasOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShortcutPlayerProximityChanged, AAGSDCharacter*, Player, bool, bInRange);

/**
 * USOVShortcutInteractComponent
 *
 * 이동 발판, 도어 등 임의의 블루프린트 액터에 추가하여 사용하는 범용 숏컷 개방 컴포넌트입니다.
 * - 부모 액터가 Interaction 인터페이스를 구현하지 않아도 자체 프록시를 통해 E키 상호작용이 100% 자동 동작합니다.
 * - 플레이어가 가까이 오면 차원 결정 필요 여부에 따른 상호작용 UI 프롬프트를 표시합니다.
 * - 차원 결정을 보유하고 있을 때 상호작용(TryInteract)하면 결정을 소모하고 GameInstance 및 세이브파일에 숏컷을 영구 개방합니다.
 * - 맵 진입 시 이미 숏컷이 열려있는 상태라면 자동으로 개방 상태 이벤트를 발생시키고 트리거를 비활성화합니다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AGSD_API USOVShortcutInteractComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	USOVShortcutInteractComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ── 숏컷 설정 ──

	/** 연동할 숏컷 지역 태그 ("Forest", "Dungeon", "Cave", "City") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Settings")
	FName ShortcutTag = FName("Forest");

	/** 숏컷 개방 시 세이브파일에 즉시 저장할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Settings")
	bool bAutoSaveOnUnlock = true;

	/** 숏컷 개방 시(또는 이미 열려있을 때) 부모 액터의 활성화 함수(DoorOpen, ActivatePlatform 등)를 자동 호출할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Settings")
	bool bAutoTriggerDoorOpen = true;

	/** 
	 * 숏컷 개방 시 스피어 콜리전 컴포넌트 자체를 Destroy할지 여부.
	 * 기본값(false)일 때는 콜리전/오버랩만 완벽히 꺼서 블루프린트 참조 에러(Accessed None)를 원천 방지합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Settings")
	bool bDestroyComponentOnUnlock = false;

	/** 
	 * 부모 액터에서 자동으로 찾아 실행할 함수명 목록 (우선순위 순서대로 탐색).
	 * 예: DoorOpen, ActivatePlatform, StartMoving, Play, Activate 등
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Settings")
	TArray<FName> AutoInvokeFunctionNames;

	// ── 요구 아이템 설정 ──

	/** 숏컷 개방에 필요한 아이템 ID (기본: DimensionCrystal / 차원 결정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Requirement")
	FString RequiredItemID = TEXT("DimensionCrystal");

	/** 숏컷 개방에 필요한 아이템 수량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Requirement")
	int32 RequiredItemAmount = 1;

	/** 숏컷 개방 시 플레이어 인벤토리에서 아이템을 실제로 차감할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Requirement")
	bool bConsumeItemOnUnlock = true;

	/** 플레이어가 차원 조각(EHS_Shard)을 손에 들고 있을 때도 개방 가능한 것으로 인정할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Requirement")
	bool bAllowHoldingShard = true;

	// ── UI 및 위젯 설정 ──

	/** 몬스터 체력바처럼 액터 머리 위에 3D/스크린 공간으로 띄울 위젯 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shortcut|UI")
	TObjectPtr<class UWidgetComponent> WorldWidgetComponent;

	/** 아이콘 및 수량(0/3)을 표시할 커스텀 요구량 위젯 클래스 (WBP_ShortcutRequirement 등 지정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	TSubclassOf<class USOVShortcutRequirementWidget> RequirementWidgetClass;

	/** 액터 머리 위 위젯의 상대 오프셋 위치 (기본값: Z축 +120) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	FVector WidgetRelativeOffset = FVector(0.0f, 0.0f, 120.0f);

	/** 액터 머리 위 위젯의 가로/세로 드로우 사이즈 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	FVector2D WidgetDrawSize = FVector2D(160.0f, 50.0f);

	/** 아이템 데이터 테이블 대신 사용할 커스텀 아이콘 텍스처 (비워두면 데이터 테이블에서 자동 탐색) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	TObjectPtr<UTexture2D> OverrideItemIcon;

	/** 수량이 모두 충족되어 개방 가능할 때 화면 하단에 ShowInteractionWidget(예: 숏컷 개방)을 띄울지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	bool bShowInteractionWidgetWhenReady = true;

	/** 수량이 모두 충족되어 개방 가능할 때 부모 액터 메시에 외곽선 테두리 하이라이트(CustomDepth)를 켤지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	bool bHighlightOwnerWhenReady = true;

	/** 수량이 부족할 때도 화면 하단에 차원 결정 필요 텍스트를 띄울지 여부 (기본: 머리 위 0/3 위젯만 표시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	bool bShowScreenPromptTextWhenLocked = false;

	/** 차원 결정이 없을 때 표시할 안내 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	FText NeedCrystalPromptText = FText::FromString(TEXT("차원 결정이 필요합니다"));

	/** 차원 결정을 보유하여 개방 가능할 때 표시할 안내 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	FText CanUnlockPromptText = FText::FromString(TEXT("숏컷 개방"));

	/** 이미 개방된 상태일 때 표시할 안내 텍스트 (빈 텍스트면 표시 안 함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	FText AlreadyOpenedPromptText = FText::FromString(TEXT(""));

	// ── 이벤트 디스패처 ──

	/** 숏컷이 성공적으로 개방되었을 때 브로드캐스트되는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Shortcut|Events")
	FOnShortcutUnlockedDelegate OnShortcutUnlocked;

	/** 맵 로드 시 숏컷이 이미 개방되어 있는 상태일 때 브로드캐스트되는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Shortcut|Events")
	FOnShortcutAlreadyUnlockedDelegate OnShortcutAlreadyUnlocked;

	/** 플레이어가 감지 반경에 들어오거나 나갈 때 브로드캐스트되는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Shortcut|Events")
	FOnShortcutPlayerProximityChanged OnPlayerProximityChanged;

public:
	// ── 외부 호출 및 블루프린트 연동 함수 ──

	/** 현재 숏컷이 이미 열려있는지 여부를 반환합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shortcut")
	bool IsShortcutAlreadyUnlocked() const;

	/** 플레이어가 숏컷을 개방할 수 있는 조건(차원 결정 보유 여부 등)을 갖추었는지 확인합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shortcut")
	bool CanUnlockShortcut(AAGSDCharacter* Player) const;

	/** 
	 * 상호작용 키 입력 시 호출하여 숏컷 개방을 시도합니다.
	 * 조건 만족 시 아이템을 차감하고 GameInstance/세이브파일에 반영하며 OnShortcutUnlocked를 호출합니다.
	 * @return 개방 성공 시 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut")
	bool TryUnlockShortcut(AAGSDCharacter* Player);

	/** 플레이어 UI 프롬프트를 현재 상황에 맞게 갱신/표시합니다 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut|UI")
	void RefreshInteractionUI(AAGSDCharacter* Player);

	/** 상호작용 UI를 숨깁니다 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut|UI")
	void HideInteractionUI(AAGSDCharacter* Player);

	/** 부모 액터(이동 발판 등)의 메시 컴포넌트들에 외곽선 테두리(CustomDepth)를 켜거나 끕니다 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut|UI")
	void SetOwnerMeshHighlight(bool bActive);

	/** 플레이어가 보유한 총 요구 아이템 수량을 반환합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shortcut")
	int32 GetPlayerHoldingItemCount(AAGSDCharacter* Player) const;

	/** 요구 아이템의 아이콘 텍스처를 가져옵니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shortcut")
	UTexture2D* GetRequirementItemIcon(AAGSDCharacter* Player) const;

	/** 스피어 콜리전을 비활성화하고 컴포넌트 자체를 안전하게 제거합니다 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut")
	void DisableAndDestroySphereCollision();

	// ── IInteraction 인터페이스 연동용 헬퍼 함수 ──

	/** BP_DoorBase 등의 Event ShowWidget에서 호출할 수 있는 헬퍼 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut|Interface")
	void HandleShowWidget(ACharacter* InPlayer);

	/** BP_DoorBase 등의 Event CanInteract에서 호출할 수 있는 헬퍼 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut|Interface")
	bool HandleCanInteract(AAGSDCharacter* InPlayer);

	/** BP_DoorBase 등의 Event Interact에서 호출할 수 있는 헬퍼 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut|Interface")
	void HandleInteract(AAGSDCharacter* InPlayer);

protected:
	// 오버랩 콜백
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 부모 액터에 활성화 UFunction(DoorOpen, ActivatePlatform 등)이 있다면 리플렉션으로 호출 */
	void InvokeParentDoorOpen();

	/** 부모 액터가 IInteraction을 구현하지 않은 경우 등록할 프록시 액터를 가져오거나 생성 */
	AActor* GetOrCreateInteractableActor();

	/** 플레이어 인벤토리에서 요구 아이템 슬롯 인덱스 및 수량 확인 (-1이면 미보유) */
	int32 FindRequiredItemSlot(AAGSDCharacter* Player, int32& OutQuantity) const;

private:
	/** 현재 범위 내에 있는 플레이어 캐릭터 캐싱 */
	UPROPERTY()
	TObjectPtr<AAGSDCharacter> CurrentOverlappingPlayer;

	/** 부모 액터가 IInteraction 인터페이스를 구현하지 않았을 때 대신 상호작용을 처리하는 프록시 액터 */
	UPROPERTY()
	TObjectPtr<ASOVInteractionProxyActor> ProxyActor;
};
