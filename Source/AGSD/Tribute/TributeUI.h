// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tribute/TributeItem.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Alchemy/Struct_MaterialAddress.h"
#include "AGSDCloseableUIInterface.h"
#include "TributeUI.generated.h"

class UAlchemyInventoryUI;
class UTributeTextUI;
class UDragEnterWidget;

USTRUCT(BlueprintType)
struct FTributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	TMap<FString, int32> TributeItems;

	FTributeData() : TributeItems({}) {}
};

UCLASS()
class AGSD_API UTributeUI : public UUserWidget, public IUIClosable
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class UTributeItem*> TributeItems;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	TSubclassOf<UTributeItem> TributeItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	UDataTable* ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	float FadeSpeed = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tribute")
	TMap<FString, FStruct_MaterialAddress> MaterialAddress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tribute")
	class AAGSDCharacter* FarmerChar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tribute")
	class ATribute* TributeActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	bool bHovered = false;

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Tribute")
	void SetTributeItemData(const FString& ItemID, int32 Amount, UTributeItem* TargetWidget);

private:
	float TargetOpacity = 0.0f;

public:
	void PlayFadeIn();

	UFUNCTION()
	void OnCloseButtonClicked();

	UFUNCTION()
	FORCEINLINE UButton* getCloseButton() const {return CloseButton;};
	
	UFUNCTION()
	void setOwnerActor(AActor* owner);

	UFUNCTION(BlueprintCallable, Category = "Tribute")
	FORCEINLINE class ATribute* GetTributeActor() const { return TributeActor; }

	UFUNCTION(BlueprintCallable, Category = "Tribute")
	void RebuildMaterialAddresses();

	UFUNCTION(BlueprintCallable, Category = "Tribute")
	void InsertMaterial(class UAlchemyCropSlotBase* InsertedSlot);

	UFUNCTION(BlueprintCallable, Category = "Tribute")
	void SubHotbarItemAmount(int32 Index, int32 AmountToRemove);

public:
	virtual void CloseUI_Implementation() override;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual bool NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	UPROPERTY(meta = (BindWidget))
	class UButton* CloseButton;

	UPROPERTY(meta = (BindWidget))
	UAlchemyInventoryUI* CropSlot;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	UDragEnterWidget* WBP_DragEnterWidget;

	UPROPERTY(meta = (BindWidget))
	UTributeTextUI* WBP_TributeTextUI;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* FadeinAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* OwnerActor;
};