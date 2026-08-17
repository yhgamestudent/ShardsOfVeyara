// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Alchemy/Struct_MaterialAddress.h"
#include "AGSDCloseableUIInterface.h"
#include "AlchemyUI.generated.h"

class UButton;
class UAlchemyInventoryUI;
class UAlchemyCropSlotBase;
class UDragEnterWidget;
class AAlchemyTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWidgetClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCropInserted);

/**
 * UAlchemyUI
 * 연금술 전체 UI 화면을 총괄하는 C++ 부모 클래스입니다.
 */
UCLASS()
class AGSD_API UAlchemyUI : public UUserWidget, public IUIClosable
{
	GENERATED_BODY()

public:
	// ── 델리게이트 변수 생성 ──
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWidgetClosed OnWidgetClosed;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCropInserted OnCropInserted;

	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void CallOnCropInserted();
	
	void PlayFadeIn();

	// 솥 내부 투입 슬롯들의 비주얼을 초기화/갱신하는 함수 (블루프린트에서 추가 오버라이드 및 외부 호출 가능)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Alchemy")
	void InitInsertedSlot();
	virtual void InitInsertedSlot_Implementation();

	// 물약 획득 알림 위젯을 화면에 추가하는 함수
	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void AddItemNotification(const FString& ItemID, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void OnCloseButtonClicked();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Alchemy")
	FORCEINLINE UButton* getCloseButton() const { return CloseButton; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Alchemy")
	FORCEINLINE UButton* getEmptyButton() const { return EmptyButton; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Alchemy")
	FORCEINLINE AActor* GetOwnerActor() const { return OwnerActor; }

	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	FORCEINLINE void setOwnerActor(AActor* owner) { OwnerActor = owner; }

	// ── 연금술 제작 및 인벤토리 로직 ──
	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void RefreshMaterialAddresses();

	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void InsertMaterial(UAlchemyCropSlotBase* InsertedSlot);

public:
	virtual void CloseUI_Implementation() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;

	// ── UI 바인딩 ──
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EmptyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAlchemyInventoryUI> CropSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAlchemyInventoryUI> DimensionCrystalSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAlchemyInventoryUI> InsertedCropSlot_0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAlchemyInventoryUI> InsertedCropSlot_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDragEnterWidget> WBP_DragEnterWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UVerticalBox> VB_ItemNotificationList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	TSubclassOf<class UAGSDItemNotificationWidget> ItemNotificationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	TObjectPtr<class UDataTable> ItemDataTable;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeinAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	TObjectPtr<AActor> OwnerActor;

	// ── 가방 및 핫바의 연금술 재료 맵 데이터 ──
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alchemy")
	TMap<FString, FStruct_MaterialAddress> MaterialAddress;

protected:
	TWeakObjectPtr<class AAGSDCharacter> OwningCharacter;
};
