// Fill out your copyright notice in the Description page of Project Settings.


#include "DragEnterWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "AlchemyCropSlotBase.h"
#include "Tribute/TributeUI.h"
#include "Tribute/Tribute.h"
#include "Alchemy/UI/AlchemyUI.h"
#include "Alchemy/AlchemyTable.h"
#include "Components/StaticMeshComponent.h"

void UDragEnterWidget::NativeOnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDragEnter(MyGeometry, DragDropEvent, Operation);

	if (Operation && Cast<UAlchemyCropSlotBase>(Operation->Payload))
	{
		bHovered = true;
		
		UE_LOG(LogTemp, Warning, TEXT("DragEnterWidget: Hovered"));
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Hovered"));
		}
#endif

		if (UAlchemyUI* AlchemyUI = GetTypedOuter<UAlchemyUI>())
		{
			if (AAlchemyTable* Table = Cast<AAlchemyTable>(AlchemyUI->GetOwnerActor()))
			{
				if (Table->PotMeshComponent)
				{
					Table->PotMeshComponent->SetRenderCustomDepth(true);
				}
			}
		}
		else if (UTributeUI* TributeUI = GetTypedOuter<UTributeUI>())
		{
			if (ATribute* Tribute = Cast<ATribute>(TributeUI->GetTributeActor()))
			{
				Tribute->SetFireBurnActive(true);
			}
		}
	}
}

void UDragEnterWidget::NativeOnDragLeave(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDragLeave(DragDropEvent, Operation);

	if (Operation && Cast<UAlchemyCropSlotBase>(Operation->Payload))
	{
		bHovered = false;

		UE_LOG(LogTemp, Warning, TEXT("DragEnterWidget: UnHovered"));
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("UnHovered"));
		}
#endif

		if (UAlchemyUI* AlchemyUI = GetTypedOuter<UAlchemyUI>())
		{
			if (AAlchemyTable* Table = Cast<AAlchemyTable>(AlchemyUI->GetOwnerActor()))
			{
				if (Table->PotMeshComponent)
				{
					Table->PotMeshComponent->SetRenderCustomDepth(false);
				}
			}
		}
		else if (UTributeUI* TributeUI = GetTypedOuter<UTributeUI>())
		{
			if (ATribute* Tribute = Cast<ATribute>(TributeUI->GetTributeActor()))
			{
				Tribute->SetFireBurnActive(false);
			}
		}
	}
}
