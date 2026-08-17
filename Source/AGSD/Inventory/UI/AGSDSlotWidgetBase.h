#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Struct_InventorySlotData.h"
#include "AGSDSlotWidgetBase.generated.h"

class UImage;
class UTextBlock;
class UAGSDInventoryComponent;
class UAGSDSlotWidgetBase;
class UAGSDDragVisualWidget;
class UAGSDItemTooltipWidget;

// ── 더블클릭 델리게이트 선언 ──
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotDoubleClicked, UAGSDSlotWidgetBase*, SlotWidget);

/**
 * UAGSDSlotWidgetBase
 * 가방 인벤토리 슬롯과 핫바 슬롯의 공통 분모가 되는 C++ 베이스 클래스입니다.
 * 클릭, 드래그앤드롭 감지 및 공통 아이템 비주얼 갱신 기능을 담당합니다.
 */
UCLASS()
class AGSD_API UAGSDSlotWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UAGSDSlotWidgetBase(const FObjectInitializer& ObjectInitializer);

	/** 백엔드 인벤토리 컴포넌트 내 실제 슬롯 인덱스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot Data", meta = (ExposeOnSpawn = "true"))
	int32 SlotIndex;

	/** 이 슬롯이 소속된 인벤토리 컴포넌트 (상자/플레이어 구분용) */
	UPROPERTY(BlueprintReadWrite, Category = "Slot Data")
	TObjectPtr<UAGSDInventoryComponent> OwningInventoryComponent;

	/** 슬롯이 현재 보유 중인 백엔드 데이터 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot Data")
	FStruct_InventorySlotData SlotItemData;

	/** 드래그 시 마우스 밑에 띄울 비주얼 위젯 클래스 (UAGSDDragVisualWidget 또는 그 파생 BP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot Config")
	TSubclassOf<UAGSDDragVisualWidget> DragVisualClass;

	/** 마우스 호버 시 띄울 설명 툴팁 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot Config")
	TSubclassOf<UAGSDItemTooltipWidget> TooltipWidgetClass;

	/** 현재 화면에 생성된 툴팁 인스턴스 */
	UPROPERTY()
	TObjectPtr<UAGSDItemTooltipWidget> ActiveTooltipInstance;

	// ── 공통 UI 컴포넌트 바인딩 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> IMG_ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> TXT_ItemAmount;

	// ── 더블클릭 이벤트 델리게이트 ──
	UPROPERTY(BlueprintAssignable, Category = "Slot UI|Events")
	FOnSlotDoubleClicked OnSlotDoubleClicked;

	// ── 기능 함수 ──

	/** 슬롯 데이터를 수동으로 대입하고 비주얼을 동기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "Slot UI")
	virtual void SetItemData(FStruct_ItemData ItemData, bool bClear);

	/** 현재 슬롯 데이터를 반환합니다. (Getter) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Slot UI")
	FStruct_InventorySlotData GetItemData() const { return SlotItemData; }

	/** 슬롯 데이터를 기반으로 아이콘/수량 등의 비주얼을 실시간 업데이트합니다. */
	UFUNCTION(BlueprintCallable, Category = "Slot UI")
	virtual void UpdateVisual();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 더블클릭 발생 시 아이템 자동 이동을 처리합니다. */
	virtual void HandleSlotDoubleClicked();

	// ── 마우스 및 드래그 앤 드롭 기본 처리 ──
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
	virtual bool NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;

	/** 소유한 캐릭터로부터 인벤토리 컴포넌트를 탐색하여 반환하는 헬퍼 함수 */
	UAGSDInventoryComponent* GetInventoryComponent() const;

private:
	/** 수동 더블 클릭 판정을 위한 클릭 시점 트래킹 시간 */
	float LastClickTime = 0.f;

	/** 수동 더블 클릭 판정을 위한 클릭 좌표 */
	FVector2D LastClickPosition = FVector2D::ZeroVector;
};
