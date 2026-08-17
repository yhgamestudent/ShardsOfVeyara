// Fill out your copyright notice in the Description page of Project Settings.


#include "ShardsAltar.h"

#include "NiagaraComponent.h"
#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "Tribute/Tribute.h"
#include "Components/SphereComponent.h"

// Sets default values
AShardsAltar::AShardsAltar()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	//루트 컴포넌트 설정
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AShardsAltar::OnBeginOverlap);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &AShardsAltar::OnEndOverlap);

	DimensionMarbleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DimensionMarbleRoot"));
	DimensionMarbleRoot->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AShardsAltar::BeginPlay()
{
	Super::BeginPlay();

	GI = Cast<USOVGameInstance>(GetGameInstance());

	if (GI)
	{
		ShardsAmount = GI->ShardsAmount;
	}
	// 1. 유효성 검사: 루트 컴포넌트가 있는지 확인
	if (!DimensionMarbleRoot) return;

	// 2. 루프를 돌며 컴포넌트 생성
	for (int32 i = 0; i < ShardsAmount; i++)
	{
		// 배열 인덱스 안전성 검사
		if (!DimensionMarbleStructs.IsValidIndex(i) || !DimensionMarbleStructs[i])
		{
			continue;
		}
		
		SpawnShards(i);
	}
}

void AShardsAltar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GI->ShardsAmount = ShardsAmount;
}

void AShardsAltar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (DimensionMarbleRoot)
	{
		// DeltaTime을 곱해 프레임 독립적인 회전 구현
		// FRotator * float 연산으로 각 축에 균일하게 적용
		DimensionMarbleRoot->AddLocalRotation(RotationSpeed * DeltaTime);
	}
}

void AShardsAltar::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

void AShardsAltar::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
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

void AShardsAltar::Interact_Implementation(AAGSDCharacter* player)
{
	player->SubItemAmount();
	SpawnShards(ShardsAmount++);
	GI->ShardsAmount = ShardsAmount;
}

void AShardsAltar::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool AShardsAltar::CanInteract_Implementation(AAGSDCharacter* player)
{
	return player->HoldingState == EHoldingState::EHS_Shard;
}

void AShardsAltar::SpawnShards(int32 i)
{
	// 3. 런타임 생성을 위해 NewObject 사용
	FName MeshName = *FString::Printf(TEXT("Shard_%d"), i);
	UStaticMeshComponent* NewMesh = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), MeshName);

	if (NewMesh)
	{
		// 4. 런타임 생성 시 필수: 컴포넌트 등록 및 부착 설정
		NewMesh->RegisterComponent(); 
		NewMesh->AttachToComponent(DimensionMarbleRoot, FAttachmentTransformRules::KeepRelativeTransform);

		// 5. 스태틱 메시 할당
		NewMesh->SetStaticMesh(DimensionMarbleStructs[i]);
			
		FName EffectName = *FString::Printf(TEXT("ShardEffect_%d"), i);
		UNiagaraComponent* NewEffect = NewObject<UNiagaraComponent>(this, UNiagaraComponent::StaticClass(), EffectName);
			
		if (NewEffect)
		{
			NewEffect->RegisterComponent();
			NewEffect->AttachToComponent(NewMesh, FAttachmentTransformRules::KeepRelativeTransform);

			NewEffect->SetAsset(ShardEffect);
			NewEffect->Activate(true);
		}
	}
}

