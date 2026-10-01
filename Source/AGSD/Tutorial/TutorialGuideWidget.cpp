#include "TutorialGuideWidget.h"
#include "TutorialSubsystem.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

UTutorialGuideWidget::UTutorialGuideWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTutorialGuideWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기 상태: 위젯 전체 숨김 및 체크 표시 숨김
	SetGuideVisibility(false);
	if (CompletedCheckmarkWidget)
	{
		CompletedCheckmarkWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	bIsTransitioning = false;
	bHasPendingStep = false;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>())
		{
			TutSub->OnTutorialStepStarted.AddUniqueDynamic(this, &UTutorialGuideWidget::HandleTutorialStepStarted);
			TutSub->OnTutorialStepProgress.AddUniqueDynamic(this, &UTutorialGuideWidget::HandleTutorialStepProgress);
			TutSub->OnTutorialStepCompleted.AddUniqueDynamic(this, &UTutorialGuideWidget::HandleTutorialStepCompleted);
			TutSub->OnTutorialSequenceCompleted.AddUniqueDynamic(this, &UTutorialGuideWidget::HandleTutorialSequenceCompleted);

			// 이미 튜토리얼이 진행 중일 때 위젯이 생성된 경우 현재 스텝 데이터로 즉시 동기화
			if (TutSub->IsTutorialActive())
			{
				FTutorialStepData StepData;
				if (TutSub->GetCurrentStepData(StepData))
				{
					HandleTutorialStepStarted(StepData);
				}
			}
		}
	}
}

void UTutorialGuideWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CompletedHoldTimerHandle);
		World->GetTimerManager().ClearTimer(SlideOutFinishTimerHandle);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>())
		{
			TutSub->OnTutorialStepStarted.RemoveAll(this);
			TutSub->OnTutorialStepProgress.RemoveAll(this);
			TutSub->OnTutorialStepCompleted.RemoveAll(this);
			TutSub->OnTutorialSequenceCompleted.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void UTutorialGuideWidget::HandleTutorialStepStarted(const FTutorialStepData& StepData)
{
	// 현재 이전 스텝 완료 연출(체크마크 표시 및 슬라이드 아웃)이 진행 중인 경우, 새 스텝 데이터를 대기열에 보관
	if (bIsTransitioning)
	{
		PendingStepData = StepData;
		bHasPendingStep = true;
		return;
	}

	CurrentStepData = StepData;

	// QuestGuideText에 실제 값이 있는지 검사
	const bool bHasQuestGuide = !StepData.QuestGuideText.IsEmptyOrWhitespace() &&
		!StepData.QuestGuideText.ToString().Equals(TEXT("None"), ESearchCase::IgnoreCase);

	// QuestGuideText가 없으면 가이드 위젯 전체를 아예 완전히 Collapsed 처리
	if (!bHasQuestGuide)
	{
		SetGuideVisibility(false);
		return;
	}

	// QuestGuideText가 있을 때만 비주얼 갱신 및 화면에 노출
	UpdateGuideVisuals(StepData);
	SetGuideVisibility(true);

	// 체크 표시 숨김
	if (CompletedCheckmarkWidget)
	{
		CompletedCheckmarkWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 슬라이드 인 애니메이션 재생
	if (SlideInAnim)
	{
		PlayAnimation(SlideInAnim);
	}

	OnStepStartedVisual(StepData);
}

void UTutorialGuideWidget::HandleTutorialStepProgress(int32 CurrentCount, int32 RequiredCount)
{
	if (ProgressTextBlock)
	{
		if (RequiredCount > 1)
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			FText ProgText = FText::Format(NSLOCTEXT("Tutorial", "ProgressFormat", "[ {0} / {1} ]"), FText::AsNumber(CurrentCount), FText::AsNumber(RequiredCount));
			ProgressTextBlock->SetText(ProgText);
		}
		else
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	OnStepProgressVisual(CurrentCount, RequiredCount);
}

void UTutorialGuideWidget::HandleTutorialStepCompleted(const FTutorialStepData& CompletedStep)
{
	const bool bHasQuestGuide = !CompletedStep.QuestGuideText.IsEmptyOrWhitespace() &&
		!CompletedStep.QuestGuideText.ToString().Equals(TEXT("None"), ESearchCase::IgnoreCase);

	// 퀘스트 텍스트가 애초에 없었던 스텝이면 연출 없이 건너뜀
	if (!bHasQuestGuide)
	{
		return;
	}

	bIsTransitioning = true;

	// 1. 녹색 체크 표시 노출
	if (CompletedCheckmarkWidget)
	{
		CompletedCheckmarkWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	OnStepCompletedVisual(CompletedStep);

	// 2. CompletedHoldDuration 동안 완료 체크 상태를 유지한 뒤 슬라이드 아웃 시작
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CompletedHoldTimerHandle);
		World->GetTimerManager().SetTimer(CompletedHoldTimerHandle, this, &UTutorialGuideWidget::StartSlideOut, CompletedHoldDuration, false);
	}
}

void UTutorialGuideWidget::StartSlideOut()
{
	float AnimDuration = 0.4f;

	// 슬라이드 아웃 애니메이션 재생
	if (SlideOutAnim)
	{
		PlayAnimation(SlideOutAnim);
		AnimDuration = FMath::Max(0.1f, SlideOutAnim->GetEndTime());
	}

	// 슬라이드 아웃 애니메이션이 끝난 후 새 스텝 내용으로 교체 및 재등장
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlideOutFinishTimerHandle);
		World->GetTimerManager().SetTimer(SlideOutFinishTimerHandle, this, &UTutorialGuideWidget::FinishTransitionAndSlideIn, AnimDuration, false);
	}
}

void UTutorialGuideWidget::FinishTransitionAndSlideIn()
{
	bIsTransitioning = false;

	// 대기 중인 다음 스텝 데이터가 있다면 적용하고 시작
	if (bHasPendingStep)
	{
		bHasPendingStep = false;
		HandleTutorialStepStarted(PendingStepData);
	}
	else
	{
		// 다음 스텝이 없으면 위젯을 완전히 숨김
		SetGuideVisibility(false);
	}
}

void UTutorialGuideWidget::HandleTutorialSequenceCompleted(FName SequenceName)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CompletedHoldTimerHandle);
		World->GetTimerManager().ClearTimer(SlideOutFinishTimerHandle);
	}

	bIsTransitioning = false;
	bHasPendingStep = false;

	SetGuideVisibility(false);
	OnSequenceCompletedVisual(SequenceName);
}

void UTutorialGuideWidget::UpdateGuideVisuals(const FTutorialStepData& StepData)
{
	const bool bHasQuestGuide = !StepData.QuestGuideText.IsEmptyOrWhitespace() &&
		!StepData.QuestGuideText.ToString().Equals(TEXT("None"), ESearchCase::IgnoreCase);

	// QuestGuideText가 없으면 전체 위젯 숨김
	if (!bHasQuestGuide)
	{
		SetGuideVisibility(false);
		return;
	}

	// 1. 퀘스트 가이드 목표 텍스트 및 컨테이너 갱신
	if (QuestGuideTextBlock)
	{
		QuestGuideTextBlock->SetText(StepData.QuestGuideText);
		QuestGuideTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (QuestGuideContainer)
	{
		QuestGuideContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 2. 조작 힌트 텍스트 및 컨테이너 갱신
	const bool bHasHint = !StepData.ControlHintText.IsEmptyOrWhitespace() &&
		!StepData.ControlHintText.ToString().Equals(TEXT("None"), ESearchCase::IgnoreCase);

	if (ControlHintTextBlock)
	{
		if (bHasHint)
		{
			ControlHintTextBlock->SetText(StepData.ControlHintText);
			ControlHintTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ControlHintTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (ControlHintContainer)
	{
		ControlHintContainer->SetVisibility(bHasHint ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	else if (ControlHintTextBlock)
	{
		ControlHintTextBlock->SetVisibility(bHasHint ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 3. 진행도 초기 표시 (다회 요구 행동일 때)
	if (ProgressTextBlock)
	{
		if (StepData.RequiredActionCount > 1)
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			FText ProgText = FText::Format(NSLOCTEXT("Tutorial", "ProgressInitFormat", "[ 0 / {0} ]"), FText::AsNumber(StepData.RequiredActionCount));
			ProgressTextBlock->SetText(ProgText);
		}
		else
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTutorialGuideWidget::SetGuideVisibility(bool bVisible)
{
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	
	// 위젯 자체(this)를 항상 Collapsed 처리하여 UMG 계층 구조와 무관하게 완전히 숨김
	SetVisibility(TargetVisibility);

	if (GuideContainer)
	{
		GuideContainer->SetVisibility(TargetVisibility);
	}
}
