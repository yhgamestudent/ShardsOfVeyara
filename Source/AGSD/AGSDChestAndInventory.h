// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AGSDCloseableUIInterface.h"
#include "AGSDChestAndInventory.generated.h"

/**
 * UAGSDChestAndInventory
 * 상자 및 인벤토리 통합 UI 위젯의 C++ 베이스 클래스입니다.
 * IUIClosable 인터페이스를 상속받아 퍼즈 키 입력 시 상자가 정상적으로 닫히도록 지원합니다.
 */
UCLASS()
class AGSD_API UAGSDChestAndInventory : public UUserWidget, public IUIClosable
{
	GENERATED_BODY()

protected:
	/** 캐릭터 수명 주기 추적 및 크래시 방지용 약참조 */
	TWeakObjectPtr<class AAGSDCharacter> OwningCharacter;

	// ── UI 바인딩 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	class UButton* CloseButton;

public:
	// IUIClosable 인터페이스 구현
	virtual void CloseUI_Implementation() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnCloseButtonClicked();
};
