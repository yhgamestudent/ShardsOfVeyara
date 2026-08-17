#pragma once

#include "CoreMinimal.h"
#include "AGSDSlotWidgetBase.h"
#include "AGSDHotbarSlotWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * UAGSDHotbarSlotWidget
 * 단축 핫바 내 개별 슬롯 위젯의 베이스 클래스입니다.
 * 공통 슬롯 베이스의 드래그앤드롭 및 비주얼 기능을 상속받으며, 핫바 전용 비주얼과 상태 복원을 관리합니다.
 */
UCLASS()
class AGSD_API UAGSDHotbarSlotWidget : public UAGSDSlotWidgetBase
{
	GENERATED_BODY()

public:
	// ── 드래그 앤 드롭 상태 플래그 (핫바 복원용) ──
	UPROPERTY(BlueprintReadWrite, Category = "Hotbar Slot|Drag Drop")
	bool bIsSlotDragging;

	UPROPERTY(BlueprintReadWrite, Category = "Hotbar Slot|Drag Drop")
	bool bDragLeavingFromHotbar;

	UPROPERTY(BlueprintReadWrite, Category = "Hotbar Slot|Drag Drop")
	bool bDragLeaving;

	UPROPERTY(BlueprintReadWrite, Category = "Hotbar Slot|Drag Drop")
	bool bDragToHotbar;

	UPROPERTY(BlueprintReadWrite, Category = "Hotbar Slot|Drag Drop")
	TObjectPtr<UAGSDHotbarSlotWidget> SelectedHotbar;

	// ── UI 바인딩 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> IMG_HotbarSelectedWhite;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> IMG_HotbarSelectedYellow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> TXT_SlotIndex;

	// ── 호환용 Getter/Setter 인터페이스 ──
	
	UFUNCTION(BlueprintCallable, Category = "Hotbar Slot")
	void SetHotbarItem(FStruct_ItemData ItemData, bool bClear)
	{
		SetItemData(ItemData, bClear);
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hotbar Slot")
	FStruct_InventorySlotData GetHotbarItem() const
	{
		return GetItemData();
	}

	UFUNCTION(BlueprintCallable, Category = "Hotbar Slot")
	void ToggleSelectedHotbar(bool bSelected);

protected:
	virtual void NativeConstruct() override;

	// ── 마우스 및 드래그 앤 드롭 핫바 전용 오버라이드 ──
	virtual void NativeOnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
	virtual bool NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
};
