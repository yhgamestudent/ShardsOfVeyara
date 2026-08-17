#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Struct_ItemData.h"
#include "AGSDDragVisualWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * UAGSDDragVisualWidget
 * 슬롯에서 드래그 시작 시 마우스 커서 아래에 표시되는 드래그 비주얼 위젯의 C++ 베이스 클래스입니다.
 * (기존 WBP_SlotDragnDrop 블루프린트 이관)
 * 아이템 아이콘과 수량을 시각적으로 표시합니다.
 */
UCLASS()
class AGSD_API UAGSDDragVisualWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── UI 바인딩 ──

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UImage> IMG_ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> TXT_ItemAmount;

	// ── 표시할 아이템 데이터 ──

	/** 드래그 비주얼에 표시할 아이템 데이터. 생성 후 외부에서 주입합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Visual")
	FStruct_ItemData ItemData;

	/** 아이템 데이터를 대입하고 비주얼을 즉시 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "Drag Visual")
	void SetDragItemData(const FStruct_ItemData& InItemData);

protected:
	virtual void NativeConstruct() override;

private:
	/** 현재 ItemData를 기반으로 아이콘과 수량 텍스트를 업데이트합니다. */
	void RefreshVisual();
};
