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

	FORCEINLINE const TMap<FString, FStruct_MaterialAddress>& GetMaterialAddress() const { return MaterialAddress; }
	FORCEINLINE class UDataTable* GetItemDataTable() const { return ItemDataTable; }

	// ── 레시피 북(도감) 관련 기능 ──
	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void ToggleRecipeBook();

	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void OpenRecipeBook(bool bOpen);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Alchemy")
	FORCEINLINE bool IsRecipeBookOpen() const { return bIsRecipeBookOpen; }

	// 레시피 북에서 원클릭 완전 자동 제작
	UFUNCTION(BlueprintCallable, Category = "Alchemy")
	void ExecuteQuickBrew(const FPotionData& RecipeToBrew);

	// 연금술 안내/경고 토스트 알림 (블루프린트 오버라이드 지원)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Alchemy")
	void ShowNotice(const FText& NoticeText);
	virtual void ShowNotice_Implementation(const FText& NoticeText);

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

	// 도감 토글 버튼 [📖]
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RecipeBookButton;

	// 우측 슬라이드 레시피 북 위젯
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UAlchemyRecipeBookWidget> WBP_AlchemyRecipeBookWidget;

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

	// 알림 텍스트 및 애니메이션 바인딩 (선택적)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> TXT_NoticeMessage;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> NoticeAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	TSubclassOf<class UAGSDItemNotificationWidget> ItemNotificationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	TObjectPtr<class UDataTable> ItemDataTable;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeinAnim;

	// 레시피 북 슬라이드 애니메이션 (열기/닫기 양방향 역재생)
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> RecipeBookAnim;

	UFUNCTION()
	void OnRecipeBookCloseFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy")
	TObjectPtr<AActor> OwnerActor;

	// ── 가방 및 핫바의 연금술 재료 맵 데이터 ──
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alchemy")
	TMap<FString, FStruct_MaterialAddress> MaterialAddress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alchemy")
	bool bIsRecipeBookOpen = false;

protected:
	TWeakObjectPtr<class AAGSDCharacter> OwningCharacter;
};
