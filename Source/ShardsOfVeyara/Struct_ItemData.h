#pragma once

#include "CoreMinimal.h"
#include "HoldingState.h"
#include "Inventory/EItemType.h"
#include "Struct_ItemData.generated.h"

USTRUCT(BlueprintType)
struct FStruct_ItemData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 아이템 고유 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	FString ItemID;

	// [DEPRECATED] 기존 블루프린트 호환용 - 신규 코드에서는 ItemType + EquipHoldingState를 사용하세요.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "ItemCategory 대신 ItemType과 EquipHoldingState를 사용하세요."))
	EHoldingState ItemCategory;

	// 아이템 유형 분류 (장비, 소모품, 재료, 퀘스트) - 인벤토리 탭 필터링에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	EItemType ItemType;

	// 핫바 활성화 시 캐릭터 장착 상태 (이 아이템을 손에 들었을 때 어떤 HoldingState가 되는지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	EHoldingState EquipHoldingState;

	// 아이템 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	FText ItemName;

	// 아이템 설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	FText ItemDescription;

	// 최대 중첩 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	int32 MaxQuantity;

	// 현재 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	int32 CurrentQuantity;

	// 아이템 아이콘 텍스처
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	TObjectPtr<UTexture2D> ItemIcon;

	// 아이템 블루프린트 클래스 (필드 드롭 시 스폰할 액터)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	TSubclassOf<AActor> ItemBPClass;


	// 기본값 설정을 위한 생성자
	FStruct_ItemData()
	{
		ItemID = TEXT("");
		ItemCategory = EHoldingState::EHS_None;
		ItemType = EItemType::EIT_None;
		EquipHoldingState = EHoldingState::EHS_None;
		ItemName = FText::GetEmpty();
		ItemDescription = FText::GetEmpty();
		MaxQuantity = 0;
		CurrentQuantity = 0;
		ItemIcon = nullptr;
		ItemBPClass = nullptr;
	}
};
