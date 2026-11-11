// Fill out your copyright notice in the Description page of Project Settings.

#include "ShardsAltar.h"

#include "NiagaraComponent.h"
#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "Tribute/Tribute.h"
#include "Components/SphereComponent.h"
#include "SOVGameInstance.h"
#include "GameplayLogSubsystem.h"
#include "AGSDPlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Interface/PetConversationInterface.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AShardsAltar::AShardsAltar()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 루트 컴포넌트 설정
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AShardsAltar::OnBeginOverlap);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &AShardsAltar::OnEndOverlap);

	DimensionMarbleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DimensionMarbleRoot"));
	DimensionMarbleRoot->SetupAttachment(RootComponent);

	// 시네마틱 컷신 연출 카메라 컴포넌트 (RootComponent에 부착하여 안정적인 고정 뷰 유지)
	CutsceneCameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CutsceneCameraComp"));
	CutsceneCameraComp->SetupAttachment(RootComponent);
	CutsceneCameraComp->SetRelativeLocation(FVector(250.f, 0.f, 120.f));
	CutsceneCameraComp->SetRelativeRotation(FRotator(-15.f, 180.f, 0.f));
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

	if (GI)
	{
		GI->ShardsAmount = ShardsAmount;
	}
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
	if (!player) return;

	player->SubItemAmount();

	if (UGameInstance* GameInst = GetGameInstance())
	{
		if (UGameplayLogSubsystem* LogSubsystem = GameInst->GetSubsystem<UGameplayLogSubsystem>())
		{
			LogSubsystem->IncrementTributeAltarUsage();
		}
	}

	if (bEnableCutscene)
	{
		StartOfferingCutscene(player);
	}
	else
	{
		UStaticMeshComponent* SpawnedMesh = SpawnShards(ShardsAmount++);
		if (GI)
		{
			GI->ShardsAmount = ShardsAmount;
		}

		if (SpawnedMesh)
		{
			const FVector EffectLoc = (SpawnedMesh->Bounds.SphereRadius > KINDA_SMALL_NUMBER)
				? SpawnedMesh->Bounds.Origin
				: SpawnedMesh->GetComponentLocation();
			PlayShardSpawnEffects(EffectLoc, SpawnedMesh->GetComponentRotation());
		}

		const FName DialogueID = (ShardsAmount >= 4) ? FinalDialogueID : RegularDialogueID;
		IPetConversationInterface::Execute_MasterToPetBigConversation(player, DialogueID);
	}
}

void AShardsAltar::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool AShardsAltar::CanInteract_Implementation(AAGSDCharacter* player)
{
	return player && player->HoldingState == EHoldingState::EHS_Shard;
}

FString AShardsAltar::GetInteractionActionType_Implementation(AAGSDCharacter* player)
{
	return TEXT("ShardsAltar");
}

void AShardsAltar::StartOfferingCutscene(AAGSDCharacter* Player)
{
	if (!Player) return;

	CachedPlayerCharacter = Player;
	CachedPlayerController = Cast<APlayerController>(Player->GetController());

	APlayerController* PC = CachedPlayerController.Get();
	if (!PC) return;

	// 1. 플레이어 조작 및 시점 입력 잠금
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	// 플레이어 이동 정지
	if (Player->GetCharacterMovement())
	{
		Player->GetCharacterMovement()->StopMovementImmediately();
	}

	// 2. 상호작용 위젯 숨김
	if (AAGSDPlayerController* AGSDPC = Cast<AAGSDPlayerController>(PC))
	{
		AGSDPC->HideInteractionWidget();
	}

	// 3. 카메라 구도 설정
	if (CutsceneCameraComp)
	{
		if (!bUseManualCutsceneCameraTransform)
		{
			// 제단 중심(구슬 부유 위치)을 향하는 자동 카메라 구도 계산
			const FVector AltarCenter = DimensionMarbleRoot ? DimensionMarbleRoot->GetComponentLocation() : GetActorLocation();
			FVector ViewDirection = GetActorForwardVector();

			const FVector FromAltarToPlayer = (Player->GetActorLocation() - AltarCenter).GetSafeNormal2D();
			if (!FromAltarToPlayer.IsNearlyZero())
			{
				ViewDirection = FromAltarToPlayer;
			}

			const FVector CameraPos = AltarCenter + (ViewDirection * 250.f) + FVector(0.f, 0.f, 80.f);
			CutsceneCameraComp->SetWorldLocation(CameraPos);

			const FRotator LookRot = UKismetMathLibrary::FindLookAtRotation(CameraPos, AltarCenter);
			CutsceneCameraComp->SetWorldRotation(LookRot);
		}
	}

	// 4. 연출 카메라로 부드럽게 전환
	PC->SetViewTargetWithBlend(this, CutsceneBlendInDuration, EViewTargetBlendFunction::VTBlend_EaseInOut, 2.0f);

	// 5. 블렌드 완료 후 구슬 스폰 타이머 등록
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			CutsceneTimerHandle,
			this,
			&AShardsAltar::OnCutsceneBlendInFinished,
			CutsceneBlendInDuration,
			false
		);
	}
}

void AShardsAltar::OnCutsceneBlendInFinished()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. 구슬 스폰 및 GI 동기화
	UStaticMeshComponent* SpawnedMesh = SpawnShards(ShardsAmount++);
	if (GI)
	{
		GI->ShardsAmount = ShardsAmount;
	}

	NewlySpawnedShardMesh = SpawnedMesh;

	// 2. 소환 버스트 이펙트 및 사운드 재생
	if (NewlySpawnedShardMesh.IsValid())
	{
		const FVector EffectLoc = (NewlySpawnedShardMesh->Bounds.SphereRadius > KINDA_SMALL_NUMBER)
			? NewlySpawnedShardMesh->Bounds.Origin
			: NewlySpawnedShardMesh->GetComponentLocation();
		PlayShardSpawnEffects(EffectLoc, NewlySpawnedShardMesh->GetComponentRotation());

		// 3. 스케일 팝업 애니메이션 시작
		NewlySpawnedShardMesh->SetWorldScale3D(FVector(0.05f));
		PopElapsedTime = 0.0f;
		World->GetTimerManager().SetTimer(
			RewardPopTimerHandle,
			this,
			&AShardsAltar::UpdateRewardPopScale,
			0.016f,
			true
		);
	}

	// 4. 구슬 회전/부유 감상 후 플레이어 시점 복귀 타이머 등록
	World->GetTimerManager().SetTimer(
		CutsceneTimerHandle,
		this,
		&AShardsAltar::StartCutsceneBlendOut,
		CutsceneHoldDuration,
		false
	);
}

void AShardsAltar::UpdateRewardPopScale()
{
	if (!NewlySpawnedShardMesh.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RewardPopTimerHandle);
		}
		return;
	}

	PopElapsedTime += 0.016f;
	const float Alpha = FMath::Clamp(PopElapsedTime / FMath::Max(RewardPopDuration, 0.05f), 0.0f, 1.0f);
	const float EasedScale = FMath::InterpEaseOut(0.05f, 1.0f, Alpha, 2.5f);

	NewlySpawnedShardMesh->SetWorldScale3D(FVector(EasedScale));

	if (Alpha >= 1.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RewardPopTimerHandle);
		}
	}
}

void AShardsAltar::StartCutsceneBlendOut()
{
	APlayerController* PC = CachedPlayerController.Get();
	AAGSDCharacter* Player = CachedPlayerCharacter.Get();

	if (PC && Player)
	{
		PC->SetViewTargetWithBlend(Player, CutsceneBlendOutDuration, EViewTargetBlendFunction::VTBlend_Cubic, 2.0f);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CutsceneTimerHandle,
			this,
			&AShardsAltar::RestorePlayerAfterCutscene,
			CutsceneBlendOutDuration,
			false
		);
	}
}

void AShardsAltar::RestorePlayerAfterCutscene()
{
	APlayerController* PC = CachedPlayerController.Get();
	AAGSDCharacter* Player = CachedPlayerCharacter.Get();

	// 1. 조작 및 시점 복구
	if (PC)
	{
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
	}

	// 2. 아르키안 정식 대화 개시
	if (Player)
	{
		const FName DialogueID = (ShardsAmount >= 4) ? FinalDialogueID : RegularDialogueID;
		IPetConversationInterface::Execute_MasterToPetBigConversation(Player, DialogueID);
	}
}

UStaticMeshComponent* AShardsAltar::SpawnShards(int32 i)
{
	// 배열 인덱스 안전성 검사
	if (!DimensionMarbleStructs.IsValidIndex(i) || !DimensionMarbleStructs[i])
	{
		return nullptr;
	}

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

	return NewMesh;
}

void AShardsAltar::PlayShardSpawnEffects(const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (ShardSpawnBurstEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			ShardSpawnBurstEffect,
			Location,
			Rotation,
			SpawnEffectScale,
			true,
			true,
			ENCPoolMethod::AutoRelease
		);
	}

	if (ShardSpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ShardSpawnSound,
			Location
		);
	}
}

