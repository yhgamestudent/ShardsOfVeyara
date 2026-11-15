#include "Credit/EndingDirector.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "TimerManager.h"
#include "SOVGameInstance.h"
#include "Character/AGSDCharacter.h"
#include "Inventory/UI/AGSDPlayerHUD.h"

AEndingDirector::AEndingDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEndingDirector::Setup(TSubclassOf<UUserWidget> InCreditWidgetClass, TSoftObjectPtr<ULevelSequence> InEndingSequence, float InStartDelay)
{
	CreditWidgetClass = InCreditWidgetClass;
	EndingSequence = InEndingSequence;
	StartDelay = FMath::Max(0.0f, InStartDelay);
}

void AEndingDirector::BeginPlay()
{
	Super::BeginPlay();

	// 1. 엔딩이 반복 재생되지 않도록 진입 즉시 플래그 해제
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		GI->bIsEnding = false;
	}

	// 캐릭터 BeginPlay(HUD 생성, 입력 모드 설정 등)가 끝난 뒤 시작
	if (StartDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(StartTimerHandle, this, &AEndingDirector::StartEnding, StartDelay, false);
	}
	else
	{
		StartEnding();
	}
}

void AEndingDirector::StartEnding()
{
	UE_LOG(LogTemp, Warning, TEXT("[Ending] Ending started (Sequence: %s, CreditWidget: %s)"),
		EndingSequence.IsNull() ? TEXT("None") : *EndingSequence.ToString(),
		CreditWidgetClass ? *CreditWidgetClass->GetName() : TEXT("None"));

	// 2. 플레이어 입력 및 HUD 차단
	LockPlayer();

	// 3. 엔딩 시퀀스가 지정되어 있으면 재생 후 크레딧, 없으면 바로 크레딧
	if (ULevelSequence* Sequence = EndingSequence.LoadSynchronous())
	{
		FMovieSceneSequencePlaybackSettings Settings;
		Settings.bDisableMovementInput = true;
		Settings.bDisableLookAtInput = true;
		Settings.bHidePlayer = false;
		Settings.bHideHud = true;

		ALevelSequenceActor* OutActor = nullptr;
		SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), Sequence, Settings, OutActor);
		SequenceActor = OutActor;

		if (SequencePlayer)
		{
			SequencePlayer->OnFinished.AddDynamic(this, &AEndingDirector::OnEndingSequenceFinished);
			SequencePlayer->Play();
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Ending] Failed to create sequence player - showing credits directly"));
	}

	ShowCredits();
}

void AEndingDirector::LockPlayer()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->DisableInput(PC);
	}

	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn()))
	{
		if (Character->PlayerHUDRef)
		{
			Character->PlayerHUDRef->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void AEndingDirector::OnEndingSequenceFinished()
{
	ShowCredits();
}

void AEndingDirector::ShowCredits()
{
	if (CreditWidget)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!CreditWidgetClass || !PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[Ending] Credit widget class is not set on BP_GameInstance (Ending|EndingCreditWidgetClass)"));
		return;
	}

	CreditWidget = CreateWidget<UUserWidget>(PC, CreditWidgetClass);
	if (CreditWidget)
	{
		CreditWidget->AddToViewport(100);

		// 크레딧 중 게임 조작 차단 (UI 전용 입력, 커서 숨김)
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(CreditWidget->TakeWidget());
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;

		UE_LOG(LogTemp, Warning, TEXT("[Ending] Credit widget shown: %s"), *CreditWidgetClass->GetName());
	}
}
