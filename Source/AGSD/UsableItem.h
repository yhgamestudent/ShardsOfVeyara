// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UsableItem.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UUsableItem : public UInterface
{
	GENERATED_BODY()
};

/**
 * 손에 들고 있는 소모품을 직접 사용할 때 호출되는 인터페이스입니다.
 */
class AGSD_API IUsableItem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void UseItem(class AAGSDCharacter* Player);
	virtual void UseItem_Implementation(class AAGSDCharacter* Player) = 0;
};
