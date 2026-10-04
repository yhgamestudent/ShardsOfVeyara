#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialObjectiveEntryWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * 튜토리얼/퀘스트 가이드에서 개별 목표 1줄(조작 힌트/설명 + 진행도)을 표시하는 위젯 베이스 클래스입니다.
 * 조건이 여러 개일 때 세로 박스(VerticalBox) 아래에 동적으로 생성되어 배치됩니다.
 */
UCLASS()
class AGSD_API UTutorialObjectiveEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTutorialObjectiveEntryWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * 세부 목표의 설명과 진행도를 갱신합니다.
	 * @param InDescription 목표 설명 또는 조작 힌트 텍스트 (예: "작물 심기", "E: 상호작용")
	 * @param InCurrentCount 현재 달성 횟수
	 * @param InRequiredCount 목표 달성 필요 횟수
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Entry")
	void UpdateObjective(const FText& InDescription, int32 InCurrentCount, int32 InRequiredCount);

	/** 목표 달성 완료 여부 */
	UFUNCTION(BlueprintPure, Category = "Tutorial|Entry")
	bool IsCompleted() const { return bIsCompleted; }

	// --- UMG 바인딩 위젯 (선택 사항: UMG 계층 구조에 같은 이름이 있으면 자동 바인딩) ---

	/** 목표 설명 또는 조작 힌트 텍스트 블록 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Entry")
	TObjectPtr<UTextBlock> DescriptionTextBlock;

	/** 스크린샷 계층구조 이름 호환용 (ControlHintTextBlock) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Entry")
	TObjectPtr<UTextBlock> ControlHintTextBlock;

	/** 행동 진행도 텍스트 블록 (예: [ 0 / 3 ]) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Entry")
	TObjectPtr<UTextBlock> ProgressTextBlock;

	/** 개별 목표 완료 시 켜질 체크 아이콘 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Entry")
	TObjectPtr<UWidget> CompletedCheckmarkWidget;

	// --- 블루프린트 연출용 이벤트 ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Entry")
	void OnObjectiveUpdated(const FText& Description, int32 CurrentCount, int32 RequiredCount, bool bCompleted);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|Entry")
	bool bIsCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|Entry")
	FText CachedDescription;
};
