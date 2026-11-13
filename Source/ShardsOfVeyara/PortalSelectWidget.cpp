// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalSelectWidget.h"

#include "EngineUtils.h"
#include "LevelSelectPortal.h"
#include "ShardsAltar.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UPortalSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//버튼이 있을 때 클릭 시 이벤트 바인딩
	if (CloseButton)
		CloseButton->OnClicked.AddDynamic(this, &UPortalSelectWidget::OnCloseButtonClicked);
	if (SelectButton)
		SelectButton->OnClicked.AddDynamic(this, &UPortalSelectWidget::OnSelectButtonClicked);
	if (NextMapButton)
		NextMapButton->OnClicked.AddDynamic(this, &UPortalSelectWidget::OnNextMapButtonClicked);
	if (PrevMapButton)
		PrevMapButton->OnClicked.AddDynamic(this, &UPortalSelectWidget::OnPrevMapButtonClicked);
	
	if (LevelSelectPortal)
	{
		mapStruct = LevelSelectPortal->getMapStruct(0);
	}
	if (!altar)
	{
		for (TActorIterator<AShardsAltar> It(GetWorld()); It; ++It)
		{
			altar = *It;
		}
	}
	if(MapImage)
	{
		if (LevelSelectPortal)
		{
			mapStruct = LevelSelectPortal->getMapStruct(index);
		}
		MapImage->SetBrushFromTexture(mapStruct.MapImage);
		MapName->SetText(mapStruct.MapName);
	}
}

void UPortalSelectWidget::OnCloseButtonClicked()
{
	//위젯이 닫힐 때 바인딩 해제
	CloseButton->OnClicked.RemoveDynamic(this, &UPortalSelectWidget::OnCloseButtonClicked);
	SelectButton->OnClicked.RemoveDynamic(this, &UPortalSelectWidget::OnSelectButtonClicked);
	NextMapButton->OnClicked.RemoveDynamic(this, &UPortalSelectWidget::OnNextMapButtonClicked);
	PrevMapButton->OnClicked.RemoveDynamic(this, &UPortalSelectWidget::OnPrevMapButtonClicked);
	
	LevelSelectPortal->OnWidgetClosed();
	//위젯 제거
	RemoveFromParent();
}

void UPortalSelectWidget::OnSelectButtonClicked()
{
	USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance());

	if (GI)
	{
		GI->TeleportationTag = mapStruct.TeleTag;
		GI->Level = mapStruct.Level;
		FName LevelName = FName(*mapStruct.Level.GetAssetName());
		UGameplayStatics::OpenLevel(this, LevelName);
	}
}

void UPortalSelectWidget::OnNextMapButtonClicked()
{
	if(MapImage)
	{
		if (LevelSelectPortal)
		{
			int32 mapNum = altar->getShardsAmount() + 1;
			index = (index + 1) % mapNum;
			mapStruct = LevelSelectPortal->getMapStruct(index);
		}
		MapImage->SetBrushFromTexture(mapStruct.MapImage);
		MapName->SetText(mapStruct.MapName);
	}
}

void UPortalSelectWidget::OnPrevMapButtonClicked()
{
	if(MapImage)
	{
		if (LevelSelectPortal)
		{
			int32 mapNum = altar->getShardsAmount() + 1;
			index = (mapNum + (index - 1)) % mapNum;
			mapStruct = LevelSelectPortal->getMapStruct(index);
		}
		MapImage->SetBrushFromTexture(mapStruct.MapImage);
		MapName->SetText(mapStruct.MapName);
	}
}
