// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOVShortcutRequirementWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * USOVShortcutRequirementWidget
 *
 * 숏컷 상호작용 구체에 접근했을 때 아이템 아이콘과 필요 수량(예: 0/3)을 표시하는 위젯 베이스 클래스입니다.
 * UMG 블루프린트에서 본 클래스를 상속받아 디자인할 수 있습니다.
 */
UCLASS()
class AGSD_API USOVShortcutRequirementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USOVShortcutRequirementWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * 아이콘과 수량(0/3 등)을 갱신합니다.
	 * @param InIconTexture 표시할 아이템 아이콘 텍스처
	 * @param InCurrentAmount 플레이어가 현재 보유한 수량
	 * @param InRequiredAmount 숏컷 개방에 필요한 수량
	 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut|UI")
	void UpdateRequirement(UTexture2D* InIconTexture, int32 InCurrentAmount, int32 InRequiredAmount);

	/** 개방 가능한 상태인지 여부 (CurrentAmount >= RequiredAmount) */
	UFUNCTION(BlueprintPure, Category = "Shortcut|UI")
	bool CanUnlock() const { return bCanUnlock; }

public:
	// ── UMG 위젯 바인딩 (선택 사항: UMG 계층 구조에 같은 이름이 있으면 자동 바인딩) ──

	/** 아이템 아이콘 이미지 위젯 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Shortcut|UI")
	TObjectPtr<UImage> ItemIcon;

	/** 필요 수량 표시 텍스트 블록 (AmountText 또는 AmountTextBlock 이름 모두 자동 바인딩 지원) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Shortcut|UI")
	TObjectPtr<UTextBlock> AmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Shortcut|UI")
	TObjectPtr<UTextBlock> AmountTextBlock;

	/** 상호작용 키 안내 텍스트 (예: [E], 선택적) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Shortcut|UI")
	TObjectPtr<UTextBlock> InteractionKeyTextBlock;

	/** 개방 가능할 때 켜질 인디케이터/체크 위젯 (선택적) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Shortcut|UI")
	TObjectPtr<UWidget> ReadyIndicatorWidget;

	// ── 블루프린트 연출용 이벤트 ──

	/** 데이터가 갱신되었을 때 블루프린트에서 색상 변경, 애니메이션 등을 처리할 수 있는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shortcut|UI")
	void OnRequirementUpdated(int32 CurrentAmount, int32 RequiredAmount, bool bInCanUnlock);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Shortcut|UI")
	bool bCanUnlock = false;

	UPROPERTY(BlueprintReadOnly, Category = "Shortcut|UI")
	int32 CachedCurrentAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Shortcut|UI")
	int32 CachedRequiredAmount = 1;
};
