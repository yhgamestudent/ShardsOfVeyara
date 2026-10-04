#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialTypes.h"
#include "TutorialGuideWidget.generated.h"

class UTextBlock;
class UWidget;
class UWidgetAnimation;

/**
 * 튜토리얼 진행 중 상단 퀘스트 가이드 텍스트(QuestGuideText)와
 * 화면 중앙/하단 조작 키 힌트(ControlHintText), 진행도 카운트를 표시하는 UI 위젯 베이스 클래스입니다.
 */
UCLASS()
class AGSD_API UTutorialGuideWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTutorialGuideWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	virtual void HandleTutorialStepStarted(const FTutorialStepData& StepData);

	UFUNCTION()
	virtual void HandleTutorialStepProgress(int32 CurrentCount, int32 RequiredCount);

	UFUNCTION()
	virtual void HandleTutorialStepCompleted(const FTutorialStepData& CompletedStep);

	UFUNCTION()
	virtual void HandleTutorialSequenceCompleted(FName SequenceName);

public:
	/** 수동으로 텍스트 및 UI 상태를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Guide")
	void UpdateGuideVisuals(const FTutorialStepData& StepData);

	/** 가이드 UI 요소들의 가시성을 일괄 제어합니다. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Guide")
	void SetGuideVisibility(bool bVisible);

	// --- UMG 바인딩 위젯 (선택 사항: UMG 계층 구조에 같은 이름의 위젯이 있으면 자동 연동) ---

	/** 가이드 전체를 감싸는 컨테이너 (위젯 전체 표시/숨김 제어용) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<UWidget> GuideContainer;

	/** 퀘스트 가이드 컨테이너/보더 (QuestGuideText가 비어있을 때 상단 박스 전체 숨김 제어용) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<UWidget> QuestGuideContainer;

	/** 상단 퀘스트 목표 텍스트 블록 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<UTextBlock> QuestGuideTextBlock;

	/** 조작 힌트 컨테이너/보더 (힌트 텍스트가 비어있을 때 숨김 제어) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<UWidget> ControlHintContainer;

	/** 조작 힌트 텍스트 블록 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<UTextBlock> ControlHintTextBlock;

	/** 행동 진행도 텍스트 블록 (예: [ 0 / 2 ]) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<UTextBlock> ProgressTextBlock;

	/** 퀘스트 완료 시 표시될 녹색 체크 표시 위젯/아이콘 (평소엔 숨김) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<UWidget> CompletedCheckmarkWidget;

	/** 목표 항목들을 세로로 나열할 패널 위젯 (스크린샷의 ControlHintContainer 아래 세로 박스 / VerticalBox) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|Guide")
	TObjectPtr<class UPanelWidget> ObjectiveListBox;

	/** 동적으로 생성할 세부 목표 항목 위젯 클래스 (UTutorialObjectiveEntryWidget을 상속받은 위젯 블루프린트) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Guide")
	TSubclassOf<class UTutorialObjectiveEntryWidget> ObjectiveEntryClass;

	/** 현재 생성되어 있는 목표 항목 위젯 목록 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Tutorial|Guide")
	TArray<TObjectPtr<class UTutorialObjectiveEntryWidget>> ActiveObjectiveEntries;

	// --- UMG 바인딩 애니메이션 (선택 사항: UMG 애니메이션 탭에 같은 이름의 애니메이션이 있으면 자동 연동) ---

	/** 오른쪽에서 화면 안으로 슬라이드 인 되는 애니메이션 */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnimOptional), Category = "Tutorial|Guide")
	TObjectPtr<UWidgetAnimation> SlideInAnim;

	/** 화면 안에서 오른쪽으로 슬라이드 아웃 되는 애니메이션 */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnimOptional), Category = "Tutorial|Guide")
	TObjectPtr<UWidgetAnimation> SlideOutAnim;

	// --- 설정 파라미터 ---

	/** 퀘스트 완료 후 체크 표시를 보여주며 대기할 시간 (초, 기본 1.0초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Guide")
	float CompletedHoldDuration = 1.0f;

	// --- 블루프린트 연출용 이벤트 ---

	/** 새 스텝이 시작될 때 블루프린트에서 추가 사운드나 비주얼 효과를 실행할 수 있는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Guide")
	void OnStepStartedVisual(const FTutorialStepData& StepData);

	/** 스텝 진행도(카운트)가 갱신될 때 실행되는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Guide")
	void OnStepProgressVisual(int32 CurrentCount, int32 RequiredCount);

	/** 스텝이 완료되었을 때 실행되는 이벤트 (완료 딩! 효과음 등) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Guide")
	void OnStepCompletedVisual(const FTutorialStepData& CompletedStep);

	/** 전체 시퀀스가 완료되었을 때 실행되는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Guide")
	void OnSequenceCompletedVisual(FName SequenceName);

protected:
	/** 완료 연출 후 슬라이드 아웃 시작 */
	UFUNCTION()
	void StartSlideOut();

	/** 슬라이드 아웃 완료 후 다음 스텝 내용 적용 및 슬라이드 인 재생 */
	UFUNCTION()
	void FinishTransitionAndSlideIn();

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|Guide")
	FTutorialStepData CurrentStepData;

private:
	FTimerHandle CompletedHoldTimerHandle;
	FTimerHandle SlideOutFinishTimerHandle;

	bool bIsTransitioning = false;
	FTutorialStepData PendingStepData;
	bool bHasPendingStep = false;
};
