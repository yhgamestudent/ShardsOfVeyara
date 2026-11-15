#include "TutorialGuideWidget.h"
#include "TutorialSubsystem.h"
#include "TutorialObjectiveEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
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
			// (단, 대화가 진행 중인 경우에는 대화 종료 후 OnTutorialStepStarted를 수신하여 표시하도록 보류)
			if (TutSub->IsTutorialActive() && !TutSub->IsWaitingForDialogue())
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

	// 이전 애니메이션이 재생 중이면 확실하게 정지하여 위치 어긋남/반만 노출 방지
	if (SlideOutAnim && IsAnimationPlaying(SlideOutAnim))
	{
		StopAnimation(SlideOutAnim);
	}
	if (SlideInAnim && IsAnimationPlaying(SlideInAnim))
	{
		StopAnimation(SlideInAnim);
	}

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

	// 슬라이드 인 애니메이션 처음(0.0초)부터 정방향 재생
	if (SlideInAnim)
	{
		PlayAnimation(SlideInAnim, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, false);
	}

	OnStepStartedVisual(StepData);
}

void UTutorialGuideWidget::HandleTutorialStepProgress(int32 CurrentCount, int32 RequiredCount)
{
	// 1. 동적 목표 위젯 목록이 있는 경우 개별 항목 갱신
	if (ActiveObjectiveEntries.Num() > 0)
	{
		// 1-A. 다중 세부 목표인 경우 -> 각 세부 목표별 진행도 갱신
		if (CurrentStepData.SubObjectives.Num() > 0)
		{
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UTutorialSubsystem* TutorialSubsystem = GI->GetSubsystem<UTutorialSubsystem>())
				{
					TArray<int32> Counts = TutorialSubsystem->GetCurrentObjectiveCounts();
					for (int32 i = 0; i < ActiveObjectiveEntries.Num(); ++i)
					{
						if (ActiveObjectiveEntries[i] && CurrentStepData.SubObjectives.IsValidIndex(i))
						{
							const FTutorialObjective& Obj = CurrentStepData.SubObjectives[i];
							int32 Cur = Counts.IsValidIndex(i) ? Counts[i] : 0;
							FText Desc = !Obj.ObjectiveDescription.IsEmpty() ? Obj.ObjectiveDescription : CurrentStepData.ControlHintText;
							ActiveObjectiveEntries[i]->UpdateObjective(Desc, Cur, Obj.RequiredActionCount);
						}
					}
				}
			}
		}
		// 1-B. 단일 목표인 경우 (SubObjectives가 비어있고 1개의 항목 위젯만 있는 경우)
		else if (ActiveObjectiveEntries.IsValidIndex(0) && ActiveObjectiveEntries[0])
		{
			FText Desc = !CurrentStepData.ControlHintText.IsEmpty() ? CurrentStepData.ControlHintText : FText::FromString(TEXT("진행하기"));
			ActiveObjectiveEntries[0]->UpdateObjective(Desc, CurrentCount, RequiredCount);
		}
	}

	// 2. 기존 단일 ProgressTextBlock도 보조로 갱신 (폴백/하위 호환)
	if (ProgressTextBlock)
	{
		FText ProgText;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UTutorialSubsystem* TutorialSubsystem = GI->GetSubsystem<UTutorialSubsystem>())
			{
				ProgText = TutorialSubsystem->GetDetailedProgressText();
			}
		}

		if (ProgText.IsEmptyOrWhitespace() && RequiredCount > 1)
		{
			ProgText = FText::Format(NSLOCTEXT("Tutorial", "ProgressFormat", "[ {0} / {1} ]"), FText::AsNumber(CurrentCount), FText::AsNumber(RequiredCount));
		}

		if (!ProgText.IsEmptyOrWhitespace())
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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

	// 슬라이드 인 애니메이션이 아직 진행 중이었다면 즉시 정지
	if (SlideInAnim && IsAnimationPlaying(SlideInAnim))
	{
		StopAnimation(SlideInAnim);
	}

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
	// 슬라이드 인 애니메이션 정지
	if (SlideInAnim && IsAnimationPlaying(SlideInAnim))
	{
		StopAnimation(SlideInAnim);
	}

	float AnimDuration = 0.4f;

	// 슬라이드 아웃 애니메이션 재생
	if (SlideOutAnim)
	{
		PlayAnimation(SlideOutAnim, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, false);
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

	// 2. 조작 힌트 및 세부 목표 컨테이너 갱신
	const bool bHasHint = !StepData.ControlHintText.IsEmptyOrWhitespace() &&
		!StepData.ControlHintText.ToString().Equals(TEXT("None"), ESearchCase::IgnoreCase);

	// ObjectiveEntryClass 자동 폴백 탐색 (에디터 디테일 패널에서 지정을 깜빡했더라도 자동 연동)
	TSubclassOf<UTutorialObjectiveEntryWidget> EffectiveEntryClass = ObjectiveEntryClass;
	if (!EffectiveEntryClass)
	{
		static const FSoftClassPath DefaultEntryClassPath(TEXT("/Game/HYH/Blueprints/Widgets/TutorialWidget/WBP_TutorialObjectiveEntry.WBP_TutorialObjectiveEntry_C"));
		EffectiveEntryClass = DefaultEntryClassPath.TryLoadClass<UTutorialObjectiveEntryWidget>();
		if (EffectiveEntryClass)
		{
			ObjectiveEntryClass = EffectiveEntryClass;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[TutorialGuide] UpdateGuideVisuals: StepID=%s, SubObjs=%d, bHasHint=%d, HasListBox=%d, HasEntryClass=%d"),
		*StepData.StepID.ToString(),
		StepData.SubObjectives.Num(),
		bHasHint ? 1 : 0,
		ObjectiveListBox != nullptr ? 1 : 0,
		EffectiveEntryClass != nullptr ? 1 : 0);

	// 세로 박스(ObjectiveListBox) 및 동적 목표 항목(ObjectiveEntryClass)이 설정된 경우
	if (ObjectiveListBox && EffectiveEntryClass)
	{
		ObjectiveListBox->ClearChildren();
		ActiveObjectiveEntries.Empty();

		// 2-A. 다중 세부 목표가 있는 경우 -> 세로 박스 아래로 항목들을 1줄씩 생성
		if (StepData.SubObjectives.Num() > 0)
		{
			for (int32 i = 0; i < StepData.SubObjectives.Num(); ++i)
			{
				const FTutorialObjective& Obj = StepData.SubObjectives[i];
				if (UTutorialObjectiveEntryWidget* Entry = CreateWidget<UTutorialObjectiveEntryWidget>(this, EffectiveEntryClass))
				{
					ObjectiveListBox->AddChild(Entry);
					Entry->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					ActiveObjectiveEntries.Add(Entry);

					FText Desc = !Obj.ObjectiveDescription.IsEmpty() ? Obj.ObjectiveDescription : StepData.ControlHintText;
					if (Desc.IsEmptyOrWhitespace())
					{
						Desc = FText::FromString(TEXT("목표 진행"));
					}
					Entry->UpdateObjective(Desc, 0, Obj.RequiredActionCount);
				}
			}

			ObjectiveListBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (ControlHintContainer)
			{
				ControlHintContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}

			// 동적 엔트리를 사용할 때는 기존 고정형 텍스트 블록은 숨김 처리하여 중복 방지
			if (ControlHintTextBlock)
			{
				ControlHintTextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (ProgressTextBlock)
			{
				ProgressTextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
			return; // 동적 리스트 생성 완료
		}
		// 2-B. 단일 목표이지만 세부 목표처럼 1줄로 표시할 경우 (힌트 또는 행동 카운트가 있을 때)
		else if (bHasHint || StepData.RequiredActionCount > 1 || StepData.ActionType != ETutorialActionType::None)
		{
			if (UTutorialObjectiveEntryWidget* Entry = CreateWidget<UTutorialObjectiveEntryWidget>(this, EffectiveEntryClass))
			{
				ObjectiveListBox->AddChild(Entry);
				Entry->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				ActiveObjectiveEntries.Add(Entry);

				FText Desc = bHasHint ? StepData.ControlHintText : FText::FromString(TEXT("진행하기"));
				Entry->UpdateObjective(Desc, 0, StepData.RequiredActionCount);
			}

			ObjectiveListBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (ControlHintContainer)
			{
				ControlHintContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			if (ControlHintTextBlock)
			{
				ControlHintTextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (ProgressTextBlock)
			{
				ProgressTextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
			return;
		}
	}

	// 3. 기존 단일 조작 힌트 텍스트 및 컨테이너 갱신 (폴백 / 하위 호환)
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

	// 4. 기존 단일 진행도 초기 표시
	if (ProgressTextBlock)
	{
		FText ProgText;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UTutorialSubsystem* TutorialSubsystem = GI->GetSubsystem<UTutorialSubsystem>())
			{
				ProgText = TutorialSubsystem->GetDetailedProgressText();
			}
		}

		if (ProgText.IsEmptyOrWhitespace())
		{
			if (StepData.RequiredActionCount > 1)
			{
				ProgText = FText::Format(NSLOCTEXT("Tutorial", "ProgressInitFormat", "[ 0 / {0} ]"), FText::AsNumber(StepData.RequiredActionCount));
			}
		}

		if (!ProgText.IsEmptyOrWhitespace())
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
