#pragma once

#include "CoreMinimal.h"
#include "AGSDSlotWidgetBase.h"
#include "AGSDInventorySlotWidget.generated.h"

/**
 * UAGSDInventorySlotWidget
 * 가방 인벤토리 그리드 내 개별 슬롯 위젯의 클래스입니다.
 * 공통 슬롯 베이스 기능을 모두 상속받습니다.
 */
UCLASS()
class AGSD_API UAGSDInventorySlotWidget : public UAGSDSlotWidgetBase
{
	GENERATED_BODY()

public:
	/** 기존 블루프린트 호환용 Getter/Setter API */
	UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
	void SetInventoryItem(FStruct_ItemData ItemData, bool bClear)
	{
		SetItemData(ItemData, bClear);
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory Slot")
	FStruct_InventorySlotData GetInventoryItem() const
	{
		return GetItemData();
	}
};
