// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AGSDRockBarricade.h"
#include "Combat/AGSDDestructibleRock.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

AAGSDRockBarricade::AAGSDRockBarricade()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 입구 볼륨 영역 정의용 박스 컴포넌트
	AreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaBox"));
	AreaBox->SetupAttachment(SceneRoot);
	AreaBox->SetBoxExtent(FVector(100.0f, 200.0f, 150.0f));
	AreaBox->SetCollisionProfileName(TEXT("NoCollision"));
	AreaBox->SetLineThickness(2.0f);
	AreaBox->bHiddenInGame = true;

	// 에디터 뷰포트 프리뷰용 가벼운 ISM 컴포넌트
	PreviewMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PreviewMeshComponent"));
	PreviewMeshComponent->SetupAttachment(SceneRoot);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetCastShadow(true);

	RockClass = AAGSDDestructibleRock::StaticClass();
}

void AAGSDRockBarricade::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdatePreview();
}

void AAGSDRockBarricade::BeginPlay()
{
	Super::BeginPlay();

	// 게임 실행 시 프리뷰 메시는 완전히 정리하고 비활성화
	if (PreviewMeshComponent)
	{
		PreviewMeshComponent->ClearInstances();
		PreviewMeshComponent->SetVisibility(false);
	}

	// 런타임 스폰 모드이고 아직 에디터에서 베이크되지 않은 경우 바위 액터들 스폰
	if (bSpawnAtRuntime && !bIsBaked)
	{
		SpawnRuntimeRocks();
	}
}

TArray<FTransform> AAGSDRockBarricade::CalculateRockTransforms() const
{
	TArray<FTransform> Result;
	if (!AreaBox || RockSpacing <= 0.0f)
	{
		return Result;
	}

	// 1. 박스의 실제 스케일이 반영된 Extent 사용 (스케일 툴(R)로 늘려도 완벽 대응)
	const FVector ScaledExtent = AreaBox->GetScaledBoxExtent();
	const float EffectiveSpacing = FMath::Max(30.0f, RockSpacing);
	const float LayerHeight = EffectiveSpacing * FMath::Clamp(VerticalSpacingRatio, 0.4f, 1.2f);

	const FTransform BoxTransform = AreaBox->GetComponentTransform();
	const FQuat BoxRotation = BoxTransform.GetRotation();
	const FVector BoxCenter = BoxTransform.GetLocation();

	FRandomStream Stream(RandomSeed);

	// 바닥에서부터 천장까지 층별 적층 (Z축)
	int32 LayerIndex = 0;
	for (float Z = -ScaledExtent.Z + (EffectiveSpacing * 0.45f); Z <= ScaledExtent.Z - (EffectiveSpacing * 0.15f); Z += LayerHeight, ++LayerIndex)
	{
		// 층마다 X와 Y 오프셋을 지그재그로 교차시켜 벽돌처럼 빈틈을 채움
		const float OffsetX = (LayerIndex % 2 == 1) ? (EffectiveSpacing * 0.5f) : 0.0f;
		const float OffsetY = ((LayerIndex / 2) % 2 == 1) ? (EffectiveSpacing * 0.5f) : 0.0f;

		for (float X = -ScaledExtent.X + (EffectiveSpacing * 0.4f); X <= ScaledExtent.X - (EffectiveSpacing * 0.15f); X += EffectiveSpacing)
		{
			for (float Y = -ScaledExtent.Y + (EffectiveSpacing * 0.4f); Y <= ScaledExtent.Y - (EffectiveSpacing * 0.15f); Y += EffectiveSpacing)
			{
				FVector LocalPos(X + OffsetX, Y + OffsetY, Z);

				// 박스 영역 내부인지 검사 (오프셋으로 인해 벗어난 경우 제외)
				if (FMath::Abs(LocalPos.X) > ScaledExtent.X || FMath::Abs(LocalPos.Y) > ScaledExtent.Y || FMath::Abs(LocalPos.Z) > ScaledExtent.Z)
				{
					continue;
				}

				// 불규칙한 자연스러움을 위한 위치 편차(Jitter)
				const FVector RandomOffset(
					Stream.FRandRange(-Jitter.X, Jitter.X),
					Stream.FRandRange(-Jitter.Y, Jitter.Y),
					Stream.FRandRange(-Jitter.Z, Jitter.Z)
				);
				LocalPos += RandomOffset;

				// 월드 위치 계산: 박스 중심 및 박스 회전만 적용 (박스 스케일 곱연산 제외 -> 메쉬 찌그러짐 원천 차단)
				const FVector WorldPos = BoxCenter + BoxRotation.RotateVector(LocalPos);

				// 회전: 박스 회전 + 무작위 3축 회전 (0 ~ 360도)
				const FRotator RandomRot(
					Stream.FRandRange(0.0f, 360.0f),
					Stream.FRandRange(0.0f, 360.0f),
					Stream.FRandRange(0.0f, 360.0f)
				);
				const FQuat WorldQuat = BoxRotation * RandomRot.Quaternion();

				// 스케일: 순수 균등 스케일 (부모 박스의 비균등 스케일에 의해 늘어나는 현상 제거)
				const float BaseScale = Stream.FRandRange(ScaleRange.X, ScaleRange.Y);
				const FVector WorldScale = FVector(BaseScale);

				Result.Add(FTransform(WorldQuat, WorldPos, WorldScale));
			}
		}
	}

	// 박스가 너무 작아 루프에서 생성되지 않았을 경우 최소 1개는 중심에 배치
	if (Result.IsEmpty() && ScaledExtent.GetMin() > 10.0f)
	{
		const FTransform CenterTransform(BoxRotation, BoxCenter, FVector(1.0f));
		Result.Add(CenterTransform);
	}

	return Result;
}

void AAGSDRockBarricade::UpdatePreview()
{
	if (!PreviewMeshComponent)
	{
		return;
	}

	PreviewMeshComponent->ClearInstances();

	// 이미 레벨에 베이크되었거나 실제 게임 월드인 경우 프리뷰 비활성화
	if (bIsBaked || (GetWorld() && GetWorld()->IsGameWorld()))
	{
		PreviewMeshComponent->SetVisibility(false);
		return;
	}

	PreviewMeshComponent->SetVisibility(true);

	// 프리뷰 메시 결정: 수동 지정된 PreviewMesh 우선, 없으면 RockClass 기본 메시 추출
	UStaticMesh* EffectiveMesh = PreviewMesh;
	if (!EffectiveMesh && RockClass)
	{
		if (AAGSDDestructibleRock* DefaultRock = RockClass->GetDefaultObject<AAGSDDestructibleRock>())
		{
			if (UStaticMeshComponent* RockMeshComp = DefaultRock->GetRockStaticMesh())
			{
				EffectiveMesh = RockMeshComp->GetStaticMesh();
			}
		}
	}

	if (!EffectiveMesh)
	{
		return;
	}

	if (PreviewMeshComponent->GetStaticMesh() != EffectiveMesh)
	{
		PreviewMeshComponent->SetStaticMesh(EffectiveMesh);
	}

	const TArray<FTransform> Transforms = CalculateRockTransforms();
	for (const FTransform& T : Transforms)
	{
		FTransform PreviewTransform = T;
		PreviewTransform.SetScale3D(T.GetScale3D() * PreviewMeshScale);
		PreviewMeshComponent->AddInstance(PreviewTransform, true);
	}
}

void AAGSDRockBarricade::SpawnRuntimeRocks()
{
	if (!RockClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TArray<FTransform> Transforms = CalculateRockTransforms();
	RuntimeSpawnedRocks.Reserve(Transforms.Num());

	for (const FTransform& T : Transforms)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;

		if (AAGSDDestructibleRock* NewRock = World->SpawnActor<AAGSDDestructibleRock>(RockClass, T, SpawnParams))
		{
			RuntimeSpawnedRocks.Add(NewRock);
		}
	}
}

void AAGSDRockBarricade::BakeRocksToLevel()
{
	ClearBakedRocks();

	if (!RockClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RockBarricade] RockClass가 지정되지 않았습니다."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TArray<FTransform> Transforms = CalculateRockTransforms();
	for (const FTransform& T : Transforms)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.OverrideLevel = GetLevel();

		AAGSDDestructibleRock* NewRock = World->SpawnActor<AAGSDDestructibleRock>(RockClass, T, SpawnParams);
		if (NewRock)
		{
#if WITH_EDITOR
			NewRock->SetFolderPath(FName(TEXT("RockBarricades/") + GetName()));
#endif
			BakedRocks.Add(NewRock);
		}
	}

	bIsBaked = true;
	UpdatePreview();
	UE_LOG(LogTemp, Log, TEXT("[RockBarricade] 레벨에 %d개의 바위 액터를 베이크했습니다."), BakedRocks.Num());
}

void AAGSDRockBarricade::ClearBakedRocks()
{
	for (TSoftObjectPtr<AAGSDDestructibleRock>& RockPtr : BakedRocks)
	{
		if (AAGSDDestructibleRock* Rock = RockPtr.Get())
		{
			Rock->Destroy();
		}
	}
	BakedRocks.Empty();
	bIsBaked = false;
	UpdatePreview();
	UE_LOG(LogTemp, Log, TEXT("[RockBarricade] 베이크된 바위 액터들을 정리했습니다."));
}

