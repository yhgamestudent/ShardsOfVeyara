// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Struct_ItemData.h"
#include "Struct_InventorySlotData.h"
#include "AGSDItemLibrary.generated.h"

class UDataTable;

/**
 * UAGSDItemLibrary
 * 
 * 아이템 데이터 및 인벤토리 슬롯 데이터를 간편하게 생성하고 조회하는 헬퍼 함수 라이브러리입니다.
 */
UCLASS()
class AGSD_API UAGSDItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 아이템 데이터 테이블에서 ID(Row Name)로 아이템 정보를 찾아 반환합니다.
	 * 
	 * @param ItemDataTable 아이템 데이터 테이블 에셋 포인터
	 * @param ItemID 찾을 아이템 ID
	 * @param bOutSuccess 아이템 조회 성공 여부 반환
	 * @return 찾은 아이템 데이터 (실패 시 빈 구조체 반환)
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Library")
	static FStruct_ItemData GetItemDataByID(const UDataTable* ItemDataTable, const FString& ItemID, bool& bOutSuccess);

	/**
	 * 아이템 ID와 수량으로 완성된 슬롯 데이터를 생성해 반환합니다.
	 * 
	 * @param ItemDataTable 아이템 데이터 테이블 에셋 포인터
	 * @param ItemID 생성할 아이템 ID
	 * @param Amount 수량
	 * @param SlotIndex 슬롯 인덱스 번호 지정
	 * @param bOutSuccess 슬롯 생성 성공 여부 반환
	 * @return 완성된 인벤토리 슬롯 데이터 구조체
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Library")
	static FStruct_InventorySlotData CreateSlotDataByID(const UDataTable* ItemDataTable, const FString& ItemID, int32 Amount, int32 SlotIndex, bool& bOutSuccess);

	/**
	 * 지정된 개수만큼 가볍게 비어 있는 초기화된 슬롯 배열을 일괄 생성해 반환합니다. (상자/인벤토리 패딩용)
	 * 
	 * @param SlotCount 생성할 슬롯 개수
	 * @return 인덱스가 할당되고 IsEmpty=true인 슬롯 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Library")
	static TArray<FStruct_InventorySlotData> CreateEmptySlots(int32 SlotCount);
};
