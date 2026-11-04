// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGSDRockBarricade.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class AAGSDDestructibleRock;
class UStaticMesh;

/**
 * 입구 및 통로를 파괴 가능한 바위들로 자동 메우는 프로시저럴 장벽 액터 (SOLID 원칙 준수)
 * - SRP: 지정된 박스 영역 내 바위들의 적층 알고리즘 연산, 에디터 프리뷰, 스폰만을 전담.
 * - OCP: RockClass 변경을 통해 다양한 파괴 액터(얼음, 나무 등)로 코드 변경 없이 교체 가능.
 * - LSP: 스폰된 모든 바위는 AAGSDDestructibleRock의 인터페이스를 온전히 준수.
 */
UCLASS(Blueprintable, ClassGroup = (Combat))
class AGSD_API AAGSDRockBarricade : public AActor
{
	GENERATED_BODY()
	
public:	
	AAGSDRockBarricade();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	/** 루트 씬 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 입구 영역을 지정하는 박스 컴포넌트 (에디터에서 기즈모로 크기 조절 가능) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> AreaBox;

	/** 에디터 뷰포트에서 바위 배치를 가볍게 미리 보여주는 인스턴스드 스태틱 메시 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> PreviewMeshComponent;

	/** 배치할 파괴 가능 바위 클래스 (BP_DestructibleRock 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Spawning")
	TSubclassOf<AAGSDDestructibleRock> RockClass;

	/** 에디터 프리뷰용 스태틱 메시 (미지정 시 RockClass의 기본 메시 자동 추출) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Visual")
	TObjectPtr<UStaticMesh> PreviewMesh;

	/** 에디터 프리뷰 메시의 기본 스케일 배율 (BP_DestructibleRock 내부 컴포넌트 스케일 보정용, 기본값: 10, 10, 5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Visual")
	FVector PreviewMeshScale = FVector(10.0f, 10.0f, 5.0f);

	/** 바위 중심 간격 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Layout", meta = (ClampMin = "30.0", UIMin = "30.0"))
	float RockSpacing = 120.0f;

	/** 층간(Z축) 적층 높이 비율 (0.75 = 지그재그로 파고들어 벽돌처럼 맞물림) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Layout", meta = (ClampMin = "0.4", ClampMax = "1.2"))
	float VerticalSpacingRatio = 0.75f;

	/** 자연스러운 불규칙성을 위한 위치 편차 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Layout")
	FVector Jitter = FVector(20.0f, 20.0f, 15.0f);

	/** 바위 무작위 크기 비율 (최소, 최대) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Layout")
	FVector2D ScaleRange = FVector2D(0.85f, 1.25f);

	/** 불규칙 배치 난수 시드값 (동일한 시드에서는 항상 동일한 형태로 배치됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Layout")
	int32 RandomSeed = 12345;

	/** 게임 시작 시 실제 바위 액터들을 자동 스폰할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade|Spawning")
	bool bSpawnAtRuntime = true;

	/** 레벨에 영구 베이크되었는지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barricade|Bake")
	bool bIsBaked = false;

	/** 런타임에 동적으로 스폰된 바위 액터 목록 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AAGSDDestructibleRock>> RuntimeSpawnedRocks;

	/** 에디터에서 베이크된 바위 액터 목록 */
	UPROPERTY()
	TArray<TSoftObjectPtr<AAGSDDestructibleRock>> BakedRocks;

public:
	/** 박스 영역 내 바위들의 로컬/월드 트랜스폼 계산 */
	UFUNCTION(BlueprintCallable, Category = "Barricade")
	TArray<FTransform> CalculateRockTransforms() const;

	/** 에디터 뷰포트 프리뷰 갱신 */
	UFUNCTION(BlueprintCallable, Category = "Barricade")
	void UpdatePreview();

	/** 런타임 바위 액터 스폰 */
	UFUNCTION(BlueprintCallable, Category = "Barricade")
	void SpawnRuntimeRocks();

	/** [에디터 버튼] 현재 설정대로 레벨에 실제 바위 액터들을 영구 생성 */
	UFUNCTION(CallInEditor, Category = "Barricade|Bake")
	void BakeRocksToLevel();

	/** [에디터 버튼] 레벨에 베이크했던 바위 액터들을 일괄 제거 */
	UFUNCTION(CallInEditor, Category = "Barricade|Bake")
	void ClearBakedRocks();
};

