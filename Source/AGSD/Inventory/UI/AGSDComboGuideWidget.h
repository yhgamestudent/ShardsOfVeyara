#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/Components/AGSDComboGuideComponent.h"
#include "AGSDComboGuideWidget.generated.h"

class UTextBlock;

/**
 * UAGSDComboGuideWidget
 * 몬스터 헌터 스타일 콤보/공격 조작 가이드 UI 위젯의 C++ 클래스입니다.
 * 플레이어 캐릭터의 현재 상태 및 콤보 윈도우 변경 델리게이트를 전달받아 입력 버튼과 공격 이름을 표시합니다.
 */
UCLASS()
class AGSD_API UAGSDComboGuideWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 첫 번째 조작 키/버튼 텍스트 (예: "[LMB / 좌클릭]") */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Combo UI")
	TObjectPtr<UTextBlock> ActionInputText;

	/** 첫 번째 공격 명칭 텍스트 (예: "찌르기") */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Combo UI")
	TObjectPtr<UTextBlock> AttackNameText;

	/** 두 번째 조작 키/버튼 텍스트 (예: "[W + LMB / 전진]") */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Combo UI")
	TObjectPtr<UTextBlock> SecondaryActionInputText;

	/** 두 번째 공격 명칭 텍스트 (예: "전진 찌르기") */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Combo UI")
	TObjectPtr<UTextBlock> SecondaryAttackNameText;

	/** 현재 수행 중인 공격 명칭 텍스트 (예: "찌르기 1타") */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Combo UI")
	TObjectPtr<UTextBlock> CurrentAttackText;

	/** 현재 공격명 페이드 아웃 위젯 애니메이션 */
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "Combo UI")
	TObjectPtr<UWidgetAnimation> CurrentAttackFadeOut;

	/** 콤보 가이드 텍스트 및 현재 공격명 업데이트 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combo UI")
	void UpdateComboGuide(const TArray<FComboGuideEntry>& GuideEntries, const FText& CurrentAttackName);

private:
	bool bWasAttacking = false;
};
