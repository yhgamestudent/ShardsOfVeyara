#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreditEntryWidget.generated.h"


UCLASS()
class AGSD_API UCreditEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 부모 위젯이 행 데이터를 넘겨줄 함수 (WBP 블루프린트에서 구현하도록 BlueprintImplementableEvent)
	UFUNCTION(BlueprintImplementableEvent, Category = "EndingCredit")
	void InitEntryData(const FEndingCreditDataBase& RowData);
};
