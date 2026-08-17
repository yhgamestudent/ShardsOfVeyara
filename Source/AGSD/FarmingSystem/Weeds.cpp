// Fill out your copyright notice in the Description page of Project Settings.


#include "Weeds.h"

#include "AGSDCharacter.h"
#include "AGSDInteractionComponent.h"
#include "InteractionOwnerInterface.h"
#include "TextLog.h"
#include "Components/BoxComponent.h"

// Sets default values
AWeeds::AWeeds()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	//콜리전 박스 설정
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWeeds::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AWeeds::OnEndOverlap);
}

void AWeeds::SnapWeedsToGround()
{
	// 1. 이 액터가 가지고 있는 모든 '스태틱 메쉬 컴포넌트'를 배열로 가져옵니다.
	TArray<UStaticMeshComponent*> WeedMeshes;
	GetComponents<UStaticMeshComponent>(WeedMeshes);

	// 2. 반복문으로 하나씩 처리합니다.
	for (UStaticMeshComponent* Mesh : WeedMeshes)
	{
		// 현재 메쉬의 월드 좌표 (X, Y는 유지하고 Z만 바꿀 것임)
		FVector MeshLoc = Mesh->GetComponentLocation();

		// 3. 레이저 쏘기 설정 (내 머리 위 500 ~ 내 발 밑 500)
		FVector TraceStart = FVector(MeshLoc.X, MeshLoc.Y, MeshLoc.Z + 50.0f);
		FVector TraceEnd   = FVector(MeshLoc.X, MeshLoc.Y, MeshLoc.Z - 50.0f);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		
		Params.AddIgnoredActor(this); // 내 자신(잡초 뭉치)은 무시

		// 4. 레이저 발사!
		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			PlacementTraceChannel, // 지형(WorldStatic)만 체크
			Params
		);

		if (bHit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s, Component: %s"), 
			*HitResult.GetActor()->GetName(), 
			*HitResult.GetComponent()->GetName());		
			
			// 5. 땅에 닿았다면 위치 이동 (World Location 설정)
			Mesh->SetWorldLocation(HitResult.Location);
            
			// 간단하게는 위 코드 대신 아래처럼 Normal에 UpVector를 맞추는 방식을 많이 씁니다.
			FRotator AlignRot = FRotationMatrix::MakeFromZ(HitResult.ImpactNormal).Rotator();
			Mesh->SetWorldRotation(AlignRot);
		}
	}
}

// Called when the game starts or when spawned
void AWeeds::BeginPlay()
{
	Super::BeginPlay();
	SnapWeedsToGround();
}

void AWeeds::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

void AWeeds::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
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

void AWeeds::OnMontageEnded(UAnimMontage* AnimMontage, bool bArg)
{
	bIsActionActive = false;

	Player->Mining = false;
	Player->Weeding = false;
	
	// 2. 이동(WASD) 입력 다시 받기
	if (PC)
	{
		PC->SetIgnoreMoveInput(false);
	}
    
	// 로그로 확인
	UE_LOG(LogTemp, Warning, TEXT("몽타주 종료! 다시 움직일 수 있습니다."));

	UTextLog::WriteTextLogByKeyword(TEXT("잡초 제거"));
	Destroy();
}

void AWeeds::PlayPullPlant(AAGSDCharacter* player)
{
	if (bIsActionActive) return;
	bIsActionActive = true;
	
	PC = player->getPlayerController();
	PC->SetIgnoreMoveInput(true);
	
	UAnimInstance* AnimInstance = player->GetMesh()->GetAnimInstance();
	if (AnimInstance && PullPlant)
	{
		AnimInstance->Montage_Play(PullPlant);

		// 4. 몽타주가 끝나면 알려달라고 예약 (델리게이트 연결)
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AWeeds::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, PullPlant);
	}
}

void AWeeds::Interact_Implementation(AAGSDCharacter* player)
{
	if (player->Mining) return;

	Player = player;
	Player->Mining = true;
	Player->Weeding = true;
	
	if (OnWeeding.IsBound())
	{
		OnWeeding.Broadcast();
	}
	if (UAGSDInteractionComponent* InteractionComp = player->GetInteractionComponent())
	{
		InteractionComp->SetHighLight(InteractionComp->GetCurrentInteractableActor(), false);
	}
	
	PlayPullPlant(player);
}

void AWeeds::ShowWidget_Implementation(ACharacter* player)
{
	if (AAGSDPlayerController* PlayerController = Cast<AAGSDPlayerController>(player->GetController()))
		PlayerController->ShowInteractionWidget(InteractActionText);
}

bool AWeeds::CanInteract_Implementation(AAGSDCharacter* player)
{
	return true;
}

