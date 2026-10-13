// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Alchemy/PotionDataTable.h"
#include "AlchemyRecipeEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;
class UAlchemyUI;

/**
 * UAlchemyRecipeEntryWidget
 * 연금술 레시피 북 스크롤 목록에 들어가는 개별 레시피 카드 위젯입니다.
 */
UCLASS()
class AGSD_API UAlchemyRecipeEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 레시피 카드 데이터 설정 함수
	UFUNCTION(BlueprintCallable, Category = "Alchemy|Recipe")
	void SetupEntry(const FPotionData& InRecipeData, bool bInDiscovered, bool bInCanCraft, int32 InCountA, int32 InCountB, int32 InCrystalCount);

	FORCEINLINE const FPotionData& GetRecipeData() const { return RecipeData; }
	FORCEINLINE bool IsDiscovered() const { return bIsDiscovered; }
	FORCEINLINE bool CanCraft() const { return bCanCraft; }

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnCraftButtonClicked();

	UAlchemyUI* GetParentAlchemyUI() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alchemy|Recipe")
	FPotionData RecipeData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alchemy|Recipe")
	bool bIsDiscovered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alchemy|Recipe")
	bool bCanCraft = false;

	// ── UMG 바인딩 위젯들 (선택적 바인딩) ──
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_PotionIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PotionName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PotionDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_IngredientA;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_IngredientACount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_IngredientB;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_IngredientBCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Crystal;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_CrystalCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Craft;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_CraftButtonLabel;

	// ── 미발견 및 텍스트 비주얼 설정 (디테일 패널에서 설정 가능) ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy|Visual")
	TObjectPtr<UTexture2D> UnknownPotionTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy|Visual")
	TObjectPtr<UTexture2D> UnknownIngredientTexture;

	// 보유 재료 수량 텍스트 색상 (기본 검은색)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy|Visual")
	FLinearColor NormalTextColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

	// 재료 부족 수량 텍스트 색상 (선명한 붉은색)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy|Visual")
	FLinearColor InsufficientTextColor = FLinearColor(0.85f, 0.15f, 0.15f, 1.0f);
};
