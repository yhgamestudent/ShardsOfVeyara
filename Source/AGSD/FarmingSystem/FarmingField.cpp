// Fill out your copyright notice in the Description page of Project Settings.

#include "FarmingSystem/FarmingField.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "FarmingSystem/ACultivationPlot.h"

// Sets default values
AFarmingField::AFarmingField()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트로 StaticMeshComponent 생성
	FieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FieldMesh"));
	RootComponent = FieldMesh;

	// 기본 PlotClass 지정
	PlotClass = AACultivationPlot::StaticClass();
}

// Called when the game starts or when spawned
void AFarmingField::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoPlantOnBeginPlay && DefaultSeedName != NAME_None)
	{
		PlantAllPlots(DefaultSeedName, DefaultSeedDataTable);
	}
}

void AFarmingField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateGrid(false);
}

void AFarmingField::RearrangeGrid()
{
	UpdateGrid(true);
}

void AFarmingField::UpdateGrid(bool bForceRearrange)
{
	if (!PlotClass) return;

	int32 TargetCount = FMath::Max(1, GridRows) * FMath::Max(1, GridCols);

	// 1. 기존에 생성되었던 모든 ChildActorComponent를 깨끗이 정리 및 배열 비우기
	TArray<UChildActorComponent*> ExistingComps;
	GetComponents<UChildActorComponent>(ExistingComps);

	for (UChildActorComponent* Comp : ExistingComps)
	{
		if (Comp)
		{
			Comp->DestroyComponent();
		}
	}
	PlotComponents.Empty();

	// 2. TargetCount 개수만큼 새로 순수하게 생성
	for (int32 i = 0; i < TargetCount; ++i)
	{
		FString CompName = FString::Printf(TEXT("CultivationPlotComp_%d"), i);
		UChildActorComponent* NewPlotComp = NewObject<UChildActorComponent>(this, *CompName);
		if (NewPlotComp)
		{
			NewPlotComp->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			NewPlotComp->SetupAttachment(FieldMesh);
			NewPlotComp->RegisterComponent();
			NewPlotComp->SetChildActorClass(PlotClass);
			PlotComponents.Add(NewPlotComp);
		}
	}

	// 3. 그리드 상대 위치 및 스케일 계산/배치
	float StartX = -((GridRows - 1) * GridSpacing.X) * 0.5f;
	float StartY = -((GridCols - 1) * GridSpacing.Y) * 0.5f;

	for (int32 Row = 0; Row < GridRows; ++Row)
	{
		for (int32 Col = 0; Col < GridCols; ++Col)
		{
			int32 Index = Row * GridCols + Col;
			if (PlotComponents.IsValidIndex(Index) && PlotComponents[Index])
			{
				FVector RelativeLoc(StartX + Row * GridSpacing.X, StartY + Col * GridSpacing.Y, 0.0f);
				PlotComponents[Index]->SetRelativeLocation(RelativeLoc);
				PlotComponents[Index]->SetRelativeScale3D(PlotScale);

				// 에디터 상에서 각 Plot의 기본 씨앗 정보 갱신
				if (AACultivationPlot* PlotActor = Cast<AACultivationPlot>(PlotComponents[Index]->GetChildActor()))
				{
					if (DefaultSeedDataTable || DefaultSeedName != NAME_None)
					{
						PlotActor->SetSeedInfo(DefaultSeedName, DefaultSeedDataTable);
					}
				}
			}
		}
	}
}

TArray<AACultivationPlot*> AFarmingField::GetCultivationPlots() const
{
	TArray<AACultivationPlot*> Result;
	for (UChildActorComponent* Comp : PlotComponents)
	{
		if (Comp)
		{
			if (AACultivationPlot* Plot = Cast<AACultivationPlot>(Comp->GetChildActor()))
			{
				Result.Add(Plot);
			}
		}
	}
	return Result;
}

void AFarmingField::PlantAllPlots(FName SeedName, UDataTable* SeedTable)
{
	TArray<AACultivationPlot*> Plots = GetCultivationPlots();
	for (AACultivationPlot* Plot : Plots)
	{
		if (Plot)
		{
			Plot->SetSeedInfo(SeedName, SeedTable);
		}
	}
}
