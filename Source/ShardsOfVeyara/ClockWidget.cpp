// Fill out your copyright notice in the Description page of Project Settings.


#include "ClockWidget.h"
#include "Kismet/GameplayStatics.h"
#include "AGSDGameStateBase.h"
#include "SOVGameInstance.h"
#include "Components/TextBlock.h"

void UClockWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UWorld* World = GetWorld();
	if (!World) return;
	
	if (AAGSDGameStateBase* GameState = Cast<AAGSDGameStateBase>(UGameplayStatics::GetGameState(World)))
	{
		GameState->OnSecondChangedDelegate.AddDynamic(this, &UClockWidget::SetTimeText);
		GameState->OnDayChangedDelegate.AddUObject(this, &UClockWidget::SetDayText);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GameState Cast failed in UClockWidget::Initialize"))
	}
}

void UClockWidget::NativeConstruct()
{
	Super::NativeConstruct();
	USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance());
	SetDayText(GI->CurrentDay);
}

void UClockWidget::SetTimeText(float time)
{
	if (!TimeText) return;

	// 시간을 0~2400 사이로 정규화 (하루가 넘어가는 경우 대비)
	float NormalizedTime = FMath::Fmod(time, 2400.0f);
	if (NormalizedTime < 0.0f) NormalizedTime += 2400.0f;

	// 시간과 분을 float 상태에서 계산하여 정밀도 유지
	int32 TotalHours = FMath::FloorToInt(NormalizedTime / 100.0f);
	float MinuteFraction = FMath::Fmod(NormalizedTime, 100.0f);
	
	// 100단위 시간 체계를 60분 체계로 변환 (1단위당 0.6분)
	int32 Minutes = FMath::FloorToInt(MinuteFraction * 0.6f);

	// 12시간제 및 AM/PM 결정
	FString Period = (TotalHours < 12) ? TEXT("AM ") : TEXT("PM ");
	int32 DisplayHours = TotalHours % 12;
	if (DisplayHours == 0) DisplayHours = 12;

	FString TimeString = FString::Printf(TEXT("%s%02d:%02d"), *Period, DisplayHours, Minutes);
	TimeText->SetText(FText::FromString(TimeString));
}

void UClockWidget::SetDayText(int32 day)
{
	if (DayText)
	{
		FString DayString = TEXT("Day ");
		DayString += FString::Printf(TEXT("%d"), day);
		DayText->SetText(FText::FromString(DayString));
	}	
}
