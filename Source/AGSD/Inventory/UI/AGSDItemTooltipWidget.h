#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Struct_ItemData.h"
#include "AGSDItemTooltipWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * UAGSDItemTooltipWidget
 * 인벤토리 아이템 위에 호버 시 노출되는 설명 툴팁 위젯의 C++ 베이스 클래스입니다.
 */
UCLASS()
class AGSD_API UAGSDItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> TXT_ItemName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> TXT_ItemDescription;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> TXT_ItemType;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> IMG_ItemIcon;

	/** 아이템 데이터를 바탕으로 툴팁 위젯의 텍스트와 비주얼을 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void SetTooltipData(const FStruct_ItemData& InItemData);
};
