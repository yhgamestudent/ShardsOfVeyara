// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SaveGameSlot.generated.h"

class UConfirmationDialog;
/**
 * 
 */
UCLASS()
class AGSD_API USaveGameSlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGameSlot")
	int32 SlotIndex;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGameSlot")
	FString SlotName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGameSlot")
	bool bIsEmpty = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGameSlot")
	UDataTable* MapDataTable;
	
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UButton* SlotButton;
	
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* SlotNum;
	
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UHorizontalBox* SaveDataPanel;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UImage* PlayMapImage;
	
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* PlayMap;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* PlayDay;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* PlayTime;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* LastSaveTime;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* EmptyText;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UCanvasPanel* DeleteButtonPanel;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UButton* DeleteButton;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnClickEmptySlotButton();

	UFUNCTION()
	void OnClickSavedSlotButton();

	UFUNCTION()
	void OnClickDeleteButton();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UConfirmationDialog> ConfirmationDialogClass;
	
	UConfirmationDialog* ConfirmationDialog;

	UFUNCTION()
	void OnDeleteSlot();
	
	// 마우스가 위젯 영역 안으로 들어올 때 호출
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스가 위젯 영역 밖으로 나갈 때 호출
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
};
