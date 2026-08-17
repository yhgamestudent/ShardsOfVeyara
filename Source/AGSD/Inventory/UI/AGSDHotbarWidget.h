#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AGSDHotbarWidget.generated.h"

class UHorizontalBox;
class USizeBox;
class UTextBlock;
class UAGSDInventoryComponent;
class UAGSDHotbarSlotWidget;
class UWidgetAnimation;

/**
 * UAGSDHotbarWidget
 * 핫바 슬롯들을 담고 상태를 출력하는 핫바 컨테이너 위젯 클래스입니다.
 */
UCLASS()
class AGSD_API UAGSDHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── UI 바인딩 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UHorizontalBox> HB_Slotbar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<USizeBox> SB_HotbarStatus;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> TXT_HotbarStatus;

	// ── 구성 설정 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar|Config")
	TSubclassOf<UAGSDHotbarSlotWidget> HotbarSlotClass;

	// ── 컴포넌트 참조 ──
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UAGSDInventoryComponent> InventoryComponent;

	// ── 캐싱된 슬롯 위젯들 ──
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TArray<TObjectPtr<UAGSDHotbarSlotWidget>> HotbarSlots;

	// ── 기능 함수 ──
	
	/** 핫바를 인벤토리 컴포넌트와 연동하여 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void InitializeHotbar(UAGSDInventoryComponent* InInventoryComponent);

	/** 핫바 선택 변경 이벤트 처리 */
	UFUNCTION()
	void OnHotbarSelected(int32 PreviousIndex, int32 NewIndex);

	/** 인벤토리 슬롯 업데이트 이벤트 처리 */
	UFUNCTION()
	void OnInventorySlotUpdated(int32 SlotIndex);

	/** 선택된 슬롯의 아이템 이름에 맞춰 상태 바를 업데이트합니다. */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void SetHotbarStatus(UAGSDHotbarSlotWidget* SelectedSlot);
};
