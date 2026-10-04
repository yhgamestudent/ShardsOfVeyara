// Fill out your copyright notice in the Description page of Project Settings.


#include "Crop.h"
#include "ACultivationPlot.h"
#include "Components/sphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "AGSDPlayerController.h"
#include "TextLog.h"
#include "GameplayLogSubsystem.h"

// Sets default values
ACrop::ACrop()
{
 	// Set this actor to call Tick() ever	y frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	//루트 컴포넌트 설정
	CropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CropMesh"));
	RootComponent = CropMesh;

	CropMesh->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Pawn,
		ECR_Overlap
		);
	CropMesh->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Camera,
		ECR_Ignore
		);

	//콜리전 박스 설정
	CollisionBox = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetSphereRadius(50);
	CollisionBox->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ACrop::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &ACrop::OnEndOverlap);
}

//오버랩 시작 함수 구현부
void ACrop::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->Implements<UInteractionOwnerInterface>())
	{
		if (IInteractionOwnerInterface* InteractOwner = Cast<IInteractionOwnerInterface>(OtherActor))
		{
			if (UAGSDInteractionComponent* InteractionComp = InteractOwner->GetInteractionComponent())
			{
				InteractionComp->AddInteractableActor(this);
			}
		}
	}
}

//오버랩 종료 함수 구현부
void ACrop::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->Implements<UInteractionOwnerInterface>())
	{
		if (IInteractionOwnerInterface* InteractOwner = Cast<IInteractionOwnerInterface>(OtherActor))
		{
			if (UAGSDInteractionComponent* InteractionComp = InteractOwner->GetInteractionComponent())
			{
				InteractionComp->RemoveInteractableActor(this);
			}
		}
	}
}

// Called when the game starts or when spawned
void ACrop::BeginPlay()
{
	Super::BeginPlay();
	SnapCropToGround();
}

void ACrop::SnapCropToGround()
{
	if (!CropMesh) return;

	// 1. 현재 메쉬의 월드 좌표 (X, Y는 유지하고 Z만 바꿈)
	FVector MeshLoc = CropMesh->GetComponentLocation();

	// 2. 레이저 쏘기 설정 (Weeds의 SnapWeedsToGround 방식: 위 TraceDistance ~ 아래 TraceDistance)
	FVector TraceStart = FVector(MeshLoc.X, MeshLoc.Y, MeshLoc.Z + TraceDistance);
	FVector TraceEnd   = FVector(MeshLoc.X, MeshLoc.Y, MeshLoc.Z - TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CropSnapToGround), true); // bTraceComplex = true (FieldMesh 폴리곤 정밀 검사)
	
	Params.AddIgnoredActor(this); // 작물 자신은 무시

	// 소유자인 경작지(AACultivationPlot) 액터 무시
	if (AActor* MyOwner = GetOwner())
	{
		Params.AddIgnoredActor(MyOwner);
	}

	// 플레이어 캐릭터 캡슐에 레이저가 맞아 캐릭터 위에 심기는 버그 방지
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(PlayerPawn);
	}

	// 3. 레이저 발사! (지형 / FieldMesh 체크)
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		PlacementTraceChannel,
		Params
	);

	// 만약 PlacementTraceChannel로 맞지 않았을 경우 WorldStatic으로 2차 폴백 검사
	if (!bHit && PlacementTraceChannel != ECC_WorldStatic)
	{
		bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_WorldStatic,
			Params
		);
	}

	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Crop] Hit Actor: %s, Component: %s, Normal: %s"), 
			*HitResult.GetActor()->GetName(), 
			*HitResult.GetComponent()->GetName(),
			*HitResult.ImpactNormal.ToString());		
		
		// 4. 땅에 닿았다면 위치 이동 (World Location 설정, 옵션 오프셋 포함)
		FVector FinalLocation = HitResult.Location + (HitResult.ImpactNormal * GroundZOffset);
		CropMesh->SetWorldLocation(FinalLocation);

		// 5. ImpactNormal(표면 법선)에 맞춰 UpVector를 회전 정렬
		FRotator AlignRot = FRotationMatrix::MakeFromZ(HitResult.ImpactNormal).Rotator();
		CropMesh->SetWorldRotation(AlignRot);
	}
}

//작물 수확 구현부
void ACrop::HarvestCrop(int32 Quantity)
{
	if (CropData == nullptr || CropData->HarvestRewards.Num() <= 0 || CropData->HarvestRewards[0].Harvest == nullptr) return;
	
	const float SpawnRadius = 50.f;
	const FVector TargetLocation = GetActorLocation();

	float RandomAngle = FMath::RandRange(0.0f, 360.f);
	float RandomDist = FMath::RandRange(0.f, SpawnRadius);

	FVector SpawnOffset(
		RandomDist * FMath::Cos(RandomAngle),
		RandomDist * FMath::Sin(RandomAngle),
		0.0f
		);

	FVector FinalSpawnLocation = TargetLocation + SpawnOffset;

	FTransform SpawnTransform = GetTransform();
	SpawnTransform.SetLocation(FinalSpawnLocation + FVector(0.f, 0.f, 40.f));
	
	APickUpItem* Harvest = GetWorld()->SpawnActorDeferred<APickUpItem>(
	CropData->HarvestRewards[0].Harvest,
	SpawnTransform,
	this,
	nullptr,
	ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	if (Harvest)
	{
		// 스폰된 수집 아이템에 최종 획득 수량을 전달합니다.
		Harvest->SetAmount(Quantity);

		UGameplayStatics::FinishSpawningActor(Harvest, SpawnTransform);
		//초기 선형 속도 설정: Z축(위) 방향으로만 작은 속도를 줌
		if (Harvest->GetMeshComponent()) // Harvest 액터에 MeshComponent를 가져오는 함수가 있다고 가정
		{
			// 50.0f 정도의 작은 힘으로 위로 튀어 오르게 합니다.
			Harvest->GetMeshComponent()->SetPhysicsLinearVelocity(FVector(0.0f, 0.0f, 100.0f)); 
		}
	}
}

void ACrop::SetCropData(UUCropData* CData)
{
	if (CData != nullptr)
	{
		this->CropData = CData;
	}
}

void ACrop::SetBonusYield(int32 Amount)
{
	BonusYield = Amount;
}

//상호작용 시 구현부
void ACrop::Interact_Implementation(AAGSDCharacter* player)
{
	UE_LOG(LogTemp, Warning, TEXT("ACrop::OnBeginOverlap"));

	int32 FinalQuantity = FMath::Max(1, CropData->HarvestRewards[0].Quantity + BonusYield);

	HarvestCrop(FinalQuantity);
	if (OnHarvested.IsBound())
	{
		OnHarvested.Broadcast();
	}
	UTextLog::WriteTextLogByStringAndFloat(TEXT("작물 수확"), CropData->CropName.ToString(), FinalQuantity);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameplayLogSubsystem* LogSubsystem = GI->GetSubsystem<UGameplayLogSubsystem>())
		{
			LogSubsystem->RecordCropHarvest(CropData->CropName.ToString(), FinalQuantity);
		}
	}
	Destroy();
}

void ACrop::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool ACrop::CanInteract_Implementation(AAGSDCharacter* player)
{
	return true;
}

FString ACrop::GetInteractionActionType_Implementation(AAGSDCharacter* player)
{
	return TEXT("CropHarvest");
}

void ACrop::MeshUpdate(int32 currentGrowStageIndex)
{
	if (CropData != nullptr && CropData->GrowthStages[currentGrowStageIndex].Mesh)
	{
		CropMesh->SetStaticMesh(CropData->GrowthStages[currentGrowStageIndex].Mesh);
	}
}

void ACrop::SetCollisionEnable()
{
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}