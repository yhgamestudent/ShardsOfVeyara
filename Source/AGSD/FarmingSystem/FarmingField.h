// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACultivationPlot.h"
#include "FarmingField.generated.h"

class UStaticMeshComponent;
class UChildActorComponent;
class UDataTable;

UCLASS()
class AGSD_API AFarmingField : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFarmingField();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// 밭의 바닥 메시 (루트 컴포넌트)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FieldMesh;

	// 자동 그리드 레이아웃 적용 여부 (false로 끄면 에디터에서 수동 위치 조절 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Grid")
	bool bAutoLayoutGrid = true;

	// 행 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Grid", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridRows = 3;

	// 열 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Grid", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridCols = 3;

	// 경작지 간 간격 (X, Y)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Grid")
	FVector2D GridSpacing = FVector2D(150.0f, 150.0f);

	// 경작지 개별 스케일 (크기)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Grid")
	FVector PlotScale = FVector(1.0f, 1.0f, 1.0f);

	// 배치할 CultivationPlot 클래스 (기본: AACultivationPlot)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Grid")
	TSubclassOf<AACultivationPlot> PlotClass;

	// 게임 시작 시 에디터 기본 씨앗 자동 심기 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Seed")
	bool bAutoPlantOnBeginPlay = false;

	// 밭 전체에 심어질 기본 씨앗 데이터 테이블
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Seed")
	TObjectPtr<UDataTable> DefaultSeedDataTable;

	// 밭 전체에 심어질 기본 씨앗 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Seed")
	FName DefaultSeedName = NAME_None;

	// 그리드로 배치된 ChildActorComponent 목록
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming|Grid")
	TArray<TObjectPtr<UChildActorComponent>> PlotComponents;

public:	
	// 배치된 모든 CultivationPlot 액터들을 구함
	UFUNCTION(BlueprintCallable, Category = "Farming")
	TArray<AACultivationPlot*> GetCultivationPlots() const;

	// 밭 안의 모든 CultivationPlot에 지정 씨앗 정보를 세팅
	UFUNCTION(BlueprintCallable, Category = "Farming")
	void PlantAllPlots(FName SeedName, UDataTable* SeedTable);

	// 에디터 Detail 패널에서 버튼으로 그리드 자동 재정렬 실행
	UFUNCTION(CallInEditor, Category = "Farming|Grid")
	void RearrangeGrid();

private:
	void UpdateGrid(bool bForceRearrange = false);
};
