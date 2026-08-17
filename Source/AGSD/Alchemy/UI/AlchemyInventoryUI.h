// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HoldingState.h"
#include "Alchemy/Struct_MaterialAddress.h"
#include "AlchemyInventoryUI.generated.h"

class UUniformGridPanel;
class UAlchemyCropSlotBase;
class UDataTable;
class AAGSDCharacter;

/**
 * UAlchemyInventoryUI
 * WBP_AlchemyInventoryUI 위젯의 C++ 부모 클래스입니다.
 */
UCLASS()
class AGSD_API UAlchemyInventoryUI : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── UI 바인딩 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UUniformGridPanel> UGP_InventorySlots;

	// ── 설정값 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy Inventory")
	TSubclassOf<UAlchemyCropSlotBase> CropSlotClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Alchemy Inventory")
	TArray<TObjectPtr<UAlchemyCropSlotBase>> Slots;

	// ── 초기화 함수 ──
	UFUNCTION(BlueprintCallable, Category = "Alchemy Inventory")
	void InitializeSlots(const TMap<FString, FStruct_MaterialAddress>& MaterialAddress, EHoldingState Category, bool bInsertedSlot);

	// ── 인벤토리 스캔 유틸리티 함수 ──
	UFUNCTION(BlueprintCallable, Category = "Alchemy Inventory")
	static TMap<FString, FStruct_MaterialAddress> ScanInventoryForMaterials(AAGSDCharacter* Character);
};
