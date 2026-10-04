#include "TutorialObjectiveEntryWidget.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

UTutorialObjectiveEntryWidget::UTutorialObjectiveEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTutorialObjectiveEntryWidget::UpdateObjective(const FText& InDescription, int32 InCurrentCount, int32 InRequiredCount)
{
	CachedDescription = InDescription;
	bIsCompleted = (InRequiredCount > 0 && InCurrentCount >= InRequiredCount);

	// 1. 설명 텍스트 블록 탐색 (DescriptionTextBlock -> ControlHintTextBlock -> 위젯 트리 검색)
	UTextBlock* TargetDescBlock = DescriptionTextBlock ? DescriptionTextBlock.Get() : ControlHintTextBlock.Get();
	if (!TargetDescBlock && WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* W : AllWidgets)
		{
			if (UTextBlock* TB = Cast<UTextBlock>(W))
			{
				if (TB != ProgressTextBlock.Get())
				{
					TargetDescBlock = TB;
					break;
				}
			}
		}
	}

	if (TargetDescBlock)
	{
		TargetDescBlock->SetText(InDescription);
		TargetDescBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 2. 진행도 텍스트 블록 탐색 및 설정
	if (!ProgressTextBlock && WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* W : AllWidgets)
		{
			if (UTextBlock* TB = Cast<UTextBlock>(W))
			{
				if (TB != TargetDescBlock)
				{
					ProgressTextBlock = TB;
					break;
				}
			}
		}
	}

	if (ProgressTextBlock)
	{
		if (InRequiredCount > 1 || InCurrentCount > 0)
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			FText ProgText = FText::Format(NSLOCTEXT("Tutorial", "ProgressFormat", "[ {0} / {1} ]"),
				FText::AsNumber(InCurrentCount),
				FText::AsNumber(InRequiredCount));
			ProgressTextBlock->SetText(ProgText);
		}
		else
		{
			ProgressTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 3. 개별 체크 아이콘 제어 (있을 경우)
	if (CompletedCheckmarkWidget)
	{
		CompletedCheckmarkWidget->SetVisibility(bIsCompleted ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	UE_LOG(LogTemp, Log, TEXT("[ObjectiveEntry] UpdateObjective: Desc='%s', Prog=[%d/%d], Completed=%d"),
		*InDescription.ToString(), InCurrentCount, InRequiredCount, bIsCompleted ? 1 : 0);

	OnObjectiveUpdated(InDescription, InCurrentCount, InRequiredCount, bIsCompleted);
}
