#include "HUD/ConversationSubtitle.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "HUD/ConversationLog.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UConversationSubtitle::NativeConstruct()
{
	Super::NativeConstruct();

	if (SkipButton) SkipButton->OnClicked.AddDynamic(this, &UConversationSubtitle::OnPressedSkipButton);
	if (LogButton) LogButton->OnClicked.AddDynamic(this, &UConversationSubtitle::OnPressedLogButton);
	if (ConversationButton) ConversationButton->OnClicked.AddDynamic(this, &UConversationSubtitle::OnPressedConversationButton);
	
	if (ConversationLogWidgetClass && !LogWidgetInstance)
	{
		LogWidgetInstance = CreateWidget<UConversationLog>(GetWorld(), ConversationLogWidgetClass);

		if (LogWidgetInstance)
		{
			// 뷰포트에 추가는 해두지만, 당장은 안 보이게 숨김
			LogWidgetInstance->AddToViewport(20); // ZOrder를 높게 설정하여 대화창보다 위에 뜨게 함 (선택사항)
			LogWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if(DialogueChoiceButton_0)
	{
		DialogueChoiceButton_0->OnClicked.AddDynamic(this, &UConversationSubtitle::OnClickDialogueChoice_0);
	}
	if(DialogueChoiceButton_1)
	{
		DialogueChoiceButton_1->OnClicked.AddDynamic(this, &UConversationSubtitle::OnClickDialogueChoice_1);
	}
}

void UConversationSubtitle::SetConversationSubtitle(const FText& InName, const FText& InDialogue)
{
	if (Text_Name)
	{
		// [수정] 이미 FText이므로 FromString 없이 바로 넣습니다.
		Text_Name->SetText(InName);
	}

	if (Text_Dialogue)
	{
		Text_Dialogue->SetText(InDialogue);
	}
}

void UConversationSubtitle::PlayFadeInAnimation()
{
	//보이게 설정
	SetVisibility(ESlateVisibility::Visible);

	// 📜 대화 활성 상태를 플레이어에게 전달하여 대화 시간 누적 시작
	if (UWorld* World = GetWorld())
	{
		if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0))
		{
			if (UFunction* Func = Player->FindFunction(TEXT("SetCurrentActionCategory")))
			{
				FString Category = TEXT("Dialogue");
				Player->ProcessEvent(Func, &Category);
			}
		}
	}
	
	if (FadeInAnim)
	{
		PlayAnimation(FadeInAnim);
	}
}

void UConversationSubtitle::PlayFadeOutAnimation()
{
	if (FadeOutAnim)
	{
		PlayAnimation(FadeOutAnim);

		// FadeOut 애니메이션 길이만큼 기다렸다가 완전히 숨김 (예: 0.5초)
		float AnimEndTime = FadeOutAnim->GetEndTime();
		GetWorld()->GetTimerManager().SetTimer(FadeOutTimerHandle, this, &UConversationSubtitle::OnFadeOutFinished,
			AnimEndTime, false);
	}
	else
	{
		// 애니메이션 없으면 바로 숨김
		OnFadeOutFinished();
	}
}

void UConversationSubtitle::OnFadeOutFinished()
{
	SetVisibility(ESlateVisibility::Hidden);

	// 📜 대화 종료 상태를 플레이어에게 전달하여 대화 시간 누적 종료
	if (UWorld* World = GetWorld())
	{
		if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0))
		{
			if (UFunction* Func = Player->FindFunction(TEXT("SetCurrentActionCategory")))
			{
				FString Category = TEXT("");
				Player->ProcessEvent(Func, &Category);
			}
		}
	}
}

void UConversationSubtitle::OnPressedSkipButton()
{
	// 📜 대화 스킵 버튼 클릭 로그 기록
	if (UWorld* World = GetWorld())
	{
		if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0))
		{
			if (UFunction* Func = Player->FindFunction(TEXT("LogDialogueLineSkip")))
			{
				Player->ProcessEvent(Func, nullptr);
			}
		}
	}

	if (OnSkipClicked.IsBound())
	{
		OnSkipClicked.Broadcast(); 
	}
}

void UConversationSubtitle::OnPressedLogButton()
{
	// 📜 대화 로그(히스토리) 확인 버튼 클릭 로그 기록
	if (UWorld* World = GetWorld())
	{
		if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0))
		{
			if (UFunction* Func = Player->FindFunction(TEXT("LogDialogueLogRecheck")))
			{
				Player->ProcessEvent(Func, nullptr);
			}
		}
	}

	if (OnLogClicked.IsBound())
	{
		OnLogClicked.Broadcast();
	}
}

void UConversationSubtitle::OnPressedConversationButton()
{
	if (OnConversationButtonClicked.IsBound())
	{
		OnConversationButtonClicked.Broadcast();
	}
}

void UConversationSubtitle::SetupChoiceDialogueText(const FText& Choice0Text, const FText& Choice1Text)
{
	if ( DialogueChoiceText_0 && DialogueChoiceText_1 )
	{
		DialogueChoiceText_0->SetText( Choice0Text );
		DialogueChoiceText_1->SetText( Choice1Text );
	}
}

void UConversationSubtitle::OnClickDialogueChoice_0()
{
	if (OnDialogueChoice_0Clicked.IsBound())
	{
		OnDialogueChoice_0Clicked.Broadcast();
	}
}

void UConversationSubtitle::OnClickDialogueChoice_1()
{
	if (OnDialogueChoice_1Clicked.IsBound())
	{
		OnDialogueChoice_1Clicked.Broadcast();
	}
}
