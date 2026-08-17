#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Struct_InventorySlotData.h"
#include "AGSDSlotDragDropOperation.generated.h"

class UAGSDSlotWidgetBase;

/**
 * UAGSDSlotDragDropOperation
 * 인벤토리 및 핫바 슬롯 간의 드래그 앤 드롭 시 데이터를 안전하게 전달하는 오퍼레이션 객체입니다.
 */
UCLASS()
class AGSD_API UAGSDSlotDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** 드래그를 시작한 슬롯의 인덱스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	int32 SourceSlotIndex;

	/** 드래그를 시작한 슬롯의 아이템 데이터 복사본 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	FStruct_InventorySlotData SourceSlotData;

	/** 드래그를 유도한 원본 위젯 포인터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<UAGSDSlotWidgetBase> SourceWidget;

	/** 핫바 슬롯에서 드래그가 시작되었는지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	bool bFromHotbar;

	/** 드래그를 시작한 슬롯의 소속 인벤토리 컴포넌트 (교차 인벤토리 이동 판별용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<class UAGSDInventoryComponent> SourceInventoryComponent;

public:
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
};
