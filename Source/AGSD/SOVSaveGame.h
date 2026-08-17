// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "GameFramework/SaveGame.h"
#include "SOVSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class AGSD_API USOVSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// GameInstance에서 모은 모든 데이터를 이 구조체 하나에 담아 저장합니다.
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	FSaveData SaveData;

	/*
	// 필요하다면 슬롯 이름이나 저장 시간 등 추가 정보를 넣을 수 있습니다.
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	FString SaveSlotName;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 UserIndex;
	*/
};
