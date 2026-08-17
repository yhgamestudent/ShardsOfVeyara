// Fill out your copyright notice in the Description page of Project Settings.


#include "Tribute/TributeTextUI.h"
#include "Components/UniformGridPanel.h"

void UTributeTextUI::SetNextTributeItem(const TMap<FString, int32>& ItemMap)
{
	TArray<FString> ItemKeys;
	ItemMap.GetKeys(ItemKeys);
	
	int32 RequiredCount = ItemKeys.Num();
	int32 CurrentCount = TributeItems.Num();

	for (int32 i = 0; i < RequiredCount; i++)
	{
		UTributeItem* TargetWidget = nullptr;

		if (i < CurrentCount)
		{
			TargetWidget = TributeItems[i];
		}
		else
		{
			TargetWidget = CreateWidget<UTributeItem>(this, TributeItem);
			if (TargetWidget)
			{
				TributeItems.Add(TargetWidget);
				if (TributeItemGrid)
				{
					TributeItemGrid->AddChildToUniformGrid(TargetWidget, 0, i);
				}
			}
		}
		if (TargetWidget)
		{
			FString CurrentID = ItemKeys[i];
			int32 CurrentAmount = ItemMap[CurrentID];
			
			SetTributeItemData(CurrentID, CurrentAmount, TargetWidget);
			TargetWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}

	for (int32 i = RequiredCount; i < TributeItems.Num(); i++)
	{
		TributeItems[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}
