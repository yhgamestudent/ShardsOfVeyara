#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AGSDInventoryWidget.generated.h"

class UUniformGridPanel;
class UAGSDInventoryComponent;
class UAGSDInventorySlotWidget;

/**
 * UAGSDInventoryWidget
 * 메인 가방 인벤토리 UI 위젯 클래스입니다.
 * 핫바 영역(0~9)을 제외한 가방 영역(10~29)의 아이템 슬롯들을 표시합니다.
 */
UCLASS()
class AGSD_API UAGSDInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── UI 바인딩 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UUniformGridPanel> UGP_InventorySlots;

	// ── 구성 설정 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	TSubclassOf<UAGSDInventorySlotWidget> SlotWidgetClass;

	/** 한 행당 배치할 슬롯 개수 (기본값: 5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	int32 SlotsPerRow = 5;

	/** 슬롯 표시 시작 인덱스 (기본: 10=가방 영역, 상자는 0으로 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	int32 StartSlotIndex = 10;

	// ── 참조 및 캐싱 ──
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UAGSDInventoryComponent> InventoryComponent;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TArray<TObjectPtr<UAGSDInventorySlotWidget>> InventorySlotWidgets;

	// ── 핵심 기능 ──

	/** 인벤토리 컴포넌트와 연동하여 가방 UI를 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory(UAGSDInventoryComponent* InInventoryComponent);

	/** 전체 가방 영역의 비주얼을 강제로 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

	/** 인벤토리 개별 슬롯 업데이트 콜백 */
	UFUNCTION()
	void OnInventorySlotUpdated(int32 SlotIndex);

	/** 인벤토리 전체 업데이트(정렬 등) 콜백 */
	UFUNCTION()
	void OnInventoryFullyUpdated();

protected:
	virtual void NativeDestruct() override;
	virtual void SynchronizeProperties() override;
};
