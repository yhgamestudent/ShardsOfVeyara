// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Alchemy/PotionDataTable.h"
#include "AlchemyRecipeBookWidget.generated.h"

class UScrollBox;
class UButton;
class UAlchemyRecipeEntryWidget;
class UAlchemyUI;

/**
 * UAlchemyRecipeBookWidget
 * 연금술 레시피 북 서브 패널 위젯입니다.
 */
UCLASS()
class AGSD_API UAlchemyRecipeBookWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 레시피 목록 갱신 (스마트 정렬 적용)
	UFUNCTION(BlueprintCallable, Category = "Alchemy|Recipe")
	void RefreshRecipeList();

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnCloseButtonClicked();

	UAlchemyUI* GetParentAlchemyUI() const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> SB_RecipeList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseRecipeBookButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy|Recipe")
	TSubclassOf<UAlchemyRecipeEntryWidget> RecipeEntryWidgetClass;

	// 스크롤 박스에 추가되는 레시피 카드 슬롯의 패딩 (기본 상하 4px)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy|Recipe")
	FMargin EntryPadding = FMargin(0.0f, 4.0f, 0.0f, 4.0f);
};
