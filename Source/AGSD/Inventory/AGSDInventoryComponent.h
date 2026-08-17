// AGSDInventoryComponent.h - 인벤토리 컴포넌트 헤더
// 기존 블루프린트 BPAC_InventoryComponent의 모든 로직을 C++로 재구현합니다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Struct_InventorySlotData.h"
#include "Inventory/EItemType.h"
#include "AGSDInventoryComponent.generated.h"

class UDataTable;

USTRUCT(BlueprintType)
struct FStruct_DefaultInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FString ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Amount = 1;
};

// ── 델리게이트 선언 ──
// 특정 슬롯이 갱신되었을 때 (UI 바인딩용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotUpdated, int32, SlotIndex);

// 핫바 선택이 변경되었을 때
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHotbarSelectionChanged, int32, PreviousIndex, int32, NewIndex);

// 아이템이 추가되었을 때
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, int32, SlotIndex, const FStruct_ItemData&, ItemData);

// 아이템이 제거되었을 때
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, int32, SlotIndex, const FStruct_ItemData&, ItemData);

// 인벤토리 전체가 갱신되었을 때 (정렬 등)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryFullyUpdated);

class USOVGameInstance;

/**
 * UAGSDInventoryComponent
 *
 * 캐릭터에 부착되는 인벤토리 관리 컴포넌트입니다.
 * 핫바(0~9) + 가방(10~MaxSlots-1)을 하나의 배열(InventorySlots)로 통합 관리합니다.
 * 
 * 기획서 기반:
 *  - 기본 총 슬롯: 30 (핫바 10 + 가방 20)
 *  - 핫바 인덱스: 0~9 (직접 참조, 인벤토리 UI에서는 건너뜀)
 *  - 가방 인덱스: 10~29 (인벤토리 UI에 표시)
 */
UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class AGSD_API UAGSDInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGSDInventoryComponent();

	// ── 상수 ──
	
	/** 핫바 시작 인덱스 (항상 0) */
	static constexpr int32 HOTBAR_START_INDEX = 0;
	
	/** 핫바 끝 인덱스 (항상 9, 포함) */
	static constexpr int32 HOTBAR_END_INDEX = 9;
	
	/** 핫바 슬롯 수 (항상 10) */
	static constexpr int32 HOTBAR_SLOT_COUNT = 10;
	
	/** 가방(메인 인벤토리 UI) 시작 인덱스 */
	static constexpr int32 BAG_START_INDEX = 10;

	// ── 핵심 함수 ──

	/**
	 * 슬롯 배열을 초기화합니다.
	 * BeginPlay 시 자동 호출되며, GameInstance에 저장된 데이터가 있으면 복원합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeSlots();

	/** 인벤토리 내의 모든 슬롯을 완전히 비웁니다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearAllSlots();

	/**
	 * 아이템을 인벤토리에 추가합니다.
	 * 배치 우선순위: 1) 기존 스택에 중첩 → 2) 빈 슬롯에 배치
	 * 
	 * @param ItemData 추가할 아이템 데이터
	 * @param OutRemainingQty 추가 후 남은 수량 (인벤토리가 가득 찬 경우)
	 * @return 아이템이 1개 이상 추가되었으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FStruct_ItemData ItemData, int32& OutRemainingQty);

	/**
	 * 아이템 ID 기반으로 아이템을 인벤토리에 추가합니다.
	 * 
	 * @param ItemID 추가할 아이템 ID
	 * @param Amount 추가할 수량
	 * @param OutRemainingQty 추가 후 남은 수량
	 * @param OutItemData 데이터 테이블에서 찾은 실제 아이템 정보 반환
	 * @return 아이템이 1개 이상 추가되었으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemByID(const FString& ItemID, int32 Amount, int32& OutRemainingQty, FStruct_ItemData& OutItemData);

	/**
	 * 특정 슬롯에서 아이템을 제거합니다.
	 * 
	 * @param SlotIndex 제거할 슬롯 인덱스
	 * @param Quantity 제거할 수량 (0이면 전체 제거)
	 * @return 제거 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 SlotIndex, int32 Quantity = 0);

	/**
	 * 두 슬롯의 아이템을 교환합니다.
	 * 같은 아이템이면 합치기를 시도하고, 다른 아이템이면 위치를 교환합니다.
	 * 
	 * @param FromIndex 소스 슬롯 인덱스
	 * @param ToIndex 대상 슬롯 인덱스
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapSlots(int32 FromIndex, int32 ToIndex);

	/**
	 * 특정 슬롯을 비웁니다.
	 * 
	 * @param SlotIndex 비울 슬롯 인덱스
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSlot(int32 SlotIndex);

	/**
	 * 빈 슬롯을 찾습니다. 핫바 영역(0~9)을 먼저 탐색하고, 없으면 가방 영역(10~)을 탐색합니다.
	 * 
	 * @return 빈 슬롯 인덱스, 없으면 -1
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 FindEmptySlot() const;

	/**
	 * 동일한 아이템이 있으면서 스택이 가득 차지 않은 슬롯을 찾습니다.
	 * 
	 * @param ItemID 검색할 아이템 ID
	 * @return 중첩 가능한 슬롯 인덱스, 없으면 -1
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 FindStackableSlot(const FString& ItemID) const;

	/**
	 * 특정 슬롯의 아이템을 월드에 드롭합니다.
	 * 
	 * @param SlotIndex 드롭할 슬롯 인덱스
	 * @return 드롭 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DropItem(int32 SlotIndex);

	/**
	 * 인벤토리를 정렬합니다 (빈 슬롯을 뒤로 이동).
	 * 가방 영역(10~)만 정렬하며, 핫바 영역(0~9)은 그대로 유지합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SortInventory();

	/**
	 * 핫바 선택을 변경합니다.
	 * 
	 * @param NewIndex 새로 선택할 핫바 인덱스 (0~9)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void SelectHotbar(int32 NewIndex);

	/**
	 * 핫바 선택을 다음/이전으로 순환합니다.
	 * 
	 * @param bForward true면 다음(+1), false면 이전(-1)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void CycleHotbar(bool bForward);

	// ── Getter 함수 ──

	/** 특정 슬롯의 데이터를 반환합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	FStruct_InventorySlotData GetSlotData(int32 SlotIndex) const;

	/** 현재 선택된 핫바 인덱스를 반환합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Hotbar")
	int32 GetCurrentHotbarIndex() const { return CurrentSelectedHotbar; }

	/** 현재 선택된 핫바 슬롯의 아이템 데이터를 반환합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Hotbar")
	FStruct_ItemData GetCurrentHotbarItemData() const;

	/** 최대 슬롯 수를 반환합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetMaxSlots() const { return MaxSlots; }

	/** 가방 슬롯 수(핫바 제외)를 반환합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetBagSlotCount() const { return MaxSlots - HOTBAR_SLOT_COUNT; }

	/** 현재 마우스가 호버링 중인 슬롯 인덱스 (-1이면 없음) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
	int32 HoveredSlotIndex = -1;

	/** 인벤토리 슬롯 배열 전체를 반환합니다 (읽기 전용) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	const TArray<FStruct_InventorySlotData>& GetAllSlots() const { return InventorySlots; }

	/** 외부에서 인벤토리 슬롯 데이터를 통째로 주입하여 갱신합니다 (상자 복원용) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetAllSlots(const TArray<FStruct_InventorySlotData>& NewSlots);

	/** 특정 인덱스가 유효한 슬롯 범위인지 확인합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	bool IsValidSlotIndex(int32 SlotIndex) const;

	/**
	 * 서로 다른 인벤토리 컴포넌트 간 슬롯 데이터를 교환합니다 (플레이어 ↔ 상자).
	 * 같은 아이템이면 합치기를 시도하고, 다른 아이템이면 위치를 교환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static void CrossInventorySwap(UAGSDInventoryComponent* SourceInv, int32 SourceIndex,
	                                UAGSDInventoryComponent* TargetInv, int32 TargetIndex);

	/** 서로 다른 인벤토리 간 아이템을 자동 전송합니다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static bool CrossInventoryTransfer(UAGSDInventoryComponent* SourceInv, int32 SourceIndex, UAGSDInventoryComponent* TargetInv);

	/** 핫바 슬롯의 아이템을 가방 영역으로 이동시킵니다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveHotbarToBag(int32 HotbarIndex);

	/** 가방 슬롯의 아이템을 핫바 영역으로 이동시킵니다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveBagToHotbar(int32 BagIndex);

	/** 특정 인덱스가 핫바 슬롯인지 확인합니다 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Hotbar")
	bool IsHotbarSlot(int32 SlotIndex) const;

	// ── GameInstance 연동 (영속성) ──

	/** 현재 인벤토리 데이터를 GameInstance에 백업합니다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Persistence")
	void SaveToGameInstance();

	/** GameInstance에서 인벤토리 데이터를 복원합니다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Persistence")
	void LoadFromGameInstance();

	// ── 델리게이트 ──

	/** 특정 슬롯 데이터가 갱신되었을 때 발동 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventorySlotUpdated OnInventorySlotUpdated;

	/** 핫바 선택이 변경되었을 때 발동 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnHotbarSelectionChanged OnHotbarSelectionChanged;

	/** 아이템이 인벤토리에 추가되었을 때 발동 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAdded OnItemAdded;

	/** 아이템이 인벤토리에서 제거되었을 때 발동 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemoved OnItemRemoved;

	/** 인벤토리 전체가 갱신되었을 때 발동 (정렬, 로드 등) */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryFullyUpdated OnInventoryFullyUpdated;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ── 멤버 변수 ──

	/** 에디터 디테일 패널에서 지정할 초기 아이템 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	TArray<FStruct_DefaultInventoryItem> DefaultItems;

	/** 최대 슬롯 수 (핫바 10 + 가방 20 = 30) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	int32 MaxSlots = 30;

	/** 아이템 데이터를 조회할 공용 데이터 테이블 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	TObjectPtr<UDataTable> ItemDataTable;

	/** 인벤토리 슬롯 배열 (인덱스 0~9: 핫바, 10~MaxSlots-1: 가방) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Data")
	TArray<FStruct_InventorySlotData> InventorySlots;

	/** 현재 선택된 핫바 인덱스 (0~9) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Hotbar")
	int32 CurrentSelectedHotbar = 0;

private:
	/** GameInstance 캐싱된 포인터 */
	UPROPERTY()
	TObjectPtr<USOVGameInstance> CachedGameInstance;

	/** GameInstance 포인터를 가져옵니다 (캐싱) */
	USOVGameInstance* GetGameInstance();

	/** 인벤토리 슬롯 유효성 검사 내부 헬퍼 */
	bool ValidateSlotIndex(int32 SlotIndex, const FString& CallerName) const;
};
