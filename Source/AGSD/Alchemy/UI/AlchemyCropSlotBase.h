// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/UI/AGSDSlotWidgetBase.h"
#include "AlchemyCropSlotBase.generated.h"

class UAlchemyInventoryUI;

/**
 * UAlchemyCropSlotBase
 * WBP_AlchemyCropSlotBase 위젯의 C++ 부모 클래스입니다.
 * AGSDSlotWidgetBase를 상속받아 기본 슬롯 기능 및 호버 툴팁 기능을 상속받습니다.
 */
UCLASS()
class AGSD_API UAlchemyCropSlotBase : public UAGSDSlotWidgetBase
{
	GENERATED_BODY()

public:
	// ── 슬롯 상태 및 데이터 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy Slot")
	bool bCantDrag = false;

	UPROPERTY(BlueprintReadWrite, Category = "Alchemy Slot")
	TObjectPtr<UAlchemyInventoryUI> AlchemyInventoryUI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy Slot")
	bool bIsInsertedSlot = false;

	// ── 외부 참조 호환용 Getter 함수들 ──
	UFUNCTION(BlueprintCallable, Category = "Alchemy Slot")
	FString GetItemID() const;

	UFUNCTION(BlueprintCallable, Category = "Alchemy Slot")
	int32 GetAmount() const;

	UFUNCTION(BlueprintCallable, Category = "Alchemy Slot")
	UTexture2D* GetTexture() const;

	// ── 위젯 인터페이스 함수 ──
	UFUNCTION(BlueprintCallable, Category = "Alchemy Slot")
	void InitSlot(bool bInCantDrag, bool bInIsInsertedSlot = false);

	UFUNCTION(BlueprintCallable, Category = "Alchemy Slot")
	void UpdateSlot(UTexture2D* InTexture, int32 InAmount);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
};
