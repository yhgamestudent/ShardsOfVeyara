// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LevelSelectPortal.h"
#include "Blueprint/UserWidget.h"
#include "PortalSelectWidget.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UPortalSelectWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	class ALevelSelectPortal* LevelSelectPortal;
	
	FORCEINLINE class UButton* getCloseButton() { return CloseButton; }

	UFUNCTION()
	void OnCloseButtonClicked();
	
	UFUNCTION()
	void OnSelectButtonClicked();

	UFUNCTION()
	void OnNextMapButtonClicked();

	UFUNCTION()
	void OnPrevMapButtonClicked();
	
protected:
	virtual void NativeConstruct() override;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* CloseButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* SelectButton;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* NextMapButton;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* PrevMapButton;

	UPROPERTY(meta = (BindWidget))
	class UImage* MapImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MapName;
	
	int index = 0;

	FStruct_MapData mapStruct;

	class AShardsAltar* altar;
};
