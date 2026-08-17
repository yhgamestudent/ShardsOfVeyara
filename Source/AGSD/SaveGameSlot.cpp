// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGameSlot.h"

#include "SOVGameInstance.h"
#include "SOVSaveGame.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "ConfirmationDialog.h"
#include "Struct_MapData.h"
#include "Components/Image.h"
#include "Navigation/PathFollowingComponent.h"

void USaveGameSlot::NativeConstruct()
{
	if (SlotNum != nullptr)
	{
		SlotNum->SetText(FText::AsNumber(SlotIndex));
	}
	if (DeleteButton != nullptr)
	{
		DeleteButton->OnClicked.AddDynamic(this, &USaveGameSlot::OnClickDeleteButton);
	}
	if (SlotName.IsEmpty()) return;

	// 해당 슬롯에 세이브 파일이 있는지 확인
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		// 파일 읽기
		USOVSaveGame* LoadedInstance = Cast<USOVSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
        
		if (LoadedInstance)
		{
			if (SlotButton)
			{
				SlotButton->OnClicked.AddDynamic(this, &USaveGameSlot::OnClickSavedSlotButton);
			}
			bIsEmpty = false;
			EmptyText->SetVisibility(ESlateVisibility::Collapsed);

			FName LevelName = FName(*LoadedInstance->SaveData.Level.GetAssetName());
			static const FString ContextString(TEXT("Item Look Up Context"));
			FStruct_MapData* Row = MapDataTable->FindRow<FStruct_MapData>(LevelName, ContextString);
			if (PlayMapImage && Row)
			{
				PlayMapImage->SetBrushFromTexture(Row->MapImage);
			}
			if (PlayMap && Row)
			{
				PlayMap->SetText(Row->MapName);
			}
			if (PlayDay)
			{
				FString DayString = TEXT("Day ");
				DayString += FString::Printf(TEXT("%d"), LoadedInstance->SaveData.CurrentDay);
				PlayDay->SetText(FText::FromString(DayString));
			}	
			if (PlayTime)
			{
				float time = LoadedInstance->SaveData.CurrentTime;
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
				PlayTime->SetText(FText::FromString(TimeString));
			}
			if (LastSaveTime)
			{
				FString LastSaveTimeString = LoadedInstance->SaveData.LastSaveTime.ToString(TEXT("%Y-%m-%d %H:%M:%S"));
				LastSaveTime->SetText(FText::FromString(LastSaveTimeString));
			}
		}
		
		return;
	}

	SaveDataPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &USaveGameSlot::OnClickEmptySlotButton);
	}
}

void USaveGameSlot::OnClickEmptySlotButton()
{
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		GI->InitializeVariables();
		GI->SaveGameSlot = SlotName;
	}
	UGameplayStatics::OpenLevel(GetWorld(), FName("Tutorial_Village"));
}

void USaveGameSlot::OnClickSavedSlotButton()
{
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		GI->SaveGameSlot = SlotName;
		GI->LoadGame();
		FName LevelName = FName(*GI->Level.GetAssetName());
    
		UGameplayStatics::OpenLevel(this, LevelName);
	}
}

void USaveGameSlot::OnClickDeleteButton()
{
	if (!ConfirmationDialog && ConfirmationDialogClass)
	{
		ConfirmationDialog = CreateWidget<UConfirmationDialog>(GetWorld(), ConfirmationDialogClass);
	}
	if (ConfirmationDialog)
	{
		ConfirmationDialog->YesButton->OnClicked.AddDynamic(this, &USaveGameSlot::OnDeleteSlot);
		ConfirmationDialog->AddToViewport();
	}
}

void USaveGameSlot::OnDeleteSlot()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		// 2. 세이브 파일 삭제
		UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	}
	
	SaveDataPanel->SetVisibility(ESlateVisibility::Collapsed);
	EmptyText->SetVisibility(ESlateVisibility::Visible);
	bIsEmpty = true;
	
	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &USaveGameSlot::OnClickEmptySlotButton);
	}
	if (ConfirmationDialog)
	{
		ConfirmationDialog->YesButton->OnClicked.RemoveAll(this);
		ConfirmationDialog->RemoveFromParent();
	}
}

void USaveGameSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!bIsEmpty && DeleteButtonPanel)
	{
		DeleteButtonPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void USaveGameSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (DeleteButtonPanel)
	{
		DeleteButtonPanel->SetVisibility(ESlateVisibility::Hidden);
	}
}


