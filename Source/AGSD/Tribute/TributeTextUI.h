// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tribute/TributeItem.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "TributeTextUI.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API UTributeTextUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UUniformGridPanel* TributeItemGrid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class UTributeItem*> TributeItems;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	TSubclassOf<UTributeItem> TributeItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	UDataTable* ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tribute")
	float FadeSpeed = 10.0f;

public:
	UFUNCTION(BlueprintCallable, Category = "Tribute")
	void SetNextTributeItem(const TMap<FString, int32>& ItemMap);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Tribute")
	void SetTributeItemData(const FString& ItemID, int32 Amount, UTributeItem* TargetWidget);

private:
	float TargetOpacity = 0.0f;
};
