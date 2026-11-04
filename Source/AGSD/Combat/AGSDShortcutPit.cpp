// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AGSDShortcutPit.h"
#include "SOVShortcutInteractComponent.h"
#include "SOVShortcutRequirementWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "Character/AGSDCharacter.h"
#include "AGSDPlayerController.h"
#include "SOVGameInstance.h"
#include "Character/Components/AGSDInteractionComponent.h"
#include "Variant_Combat/Interfaces/InteractionOwnerInterface.h"

AAGSDShortcutPit::AAGSDShortcutPit()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 루트 씬 컴포넌트 생성
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 2. 구덩이 스태틱 메시 컴포넌트 생성
	PitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PitMesh"));
	PitMesh->SetupAttachment(SceneRoot);
	PitMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PitMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 기본 구덩이 에셋 할당 (/Game/LevelDesign/Meshes/LP_Landscape/Hole)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HoleMeshFinder(TEXT("/Game/LevelDesign/Meshes/LP_Landscape/Hole.Hole"));
	if (HoleMeshFinder.Succeeded())
	{
		PitMesh->SetStaticMesh(HoleMeshFinder.Object);
	}

	// 3. 상호작용 트리거 구체 생성 (텔레포트 접근 감지)
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->InitSphereRadius(250.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AAGSDShortcutPit::OnBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AAGSDShortcutPit::OnEndOverlap);

	// 4. 도착 지점 컴포넌트 생성 (구덩이 앞 200cm, 바닥 위 20cm 기본 배치)
	ArrivalPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ArrivalPoint"));
	ArrivalPoint->SetupAttachment(SceneRoot);
	ArrivalPoint->SetRelativeLocation(FVector(200.0f, 0.0f, 20.0f));
	ArrivalPoint->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	// 5. 숏컷 해금 전담 컴포넌트 생성 (차원 결정 검사/소모, 머리 위 3D 요구 위젯, 세이브 연동)
	ShortcutInteractComp = CreateDefaultSubobject<USOVShortcutInteractComponent>(TEXT("ShortcutInteractComp"));
	ShortcutInteractComp->SetupAttachment(SceneRoot);
	ShortcutInteractComp->InitSphereRadius(250.0f);
	ShortcutInteractComp->ShortcutTag = FName("Cave");
	ShortcutInteractComp->RequiredItemID = TEXT("StrangeDimensionCrystal");
	ShortcutInteractComp->RequiredItemAmount = 3;
	ShortcutInteractComp->bAutoSaveOnUnlock = true;
	ShortcutInteractComp->bConsumeItemOnUnlock = true;
	ShortcutInteractComp->bAutoTriggerDoorOpen = false;
	ShortcutInteractComp->bShowInteractionWidgetWhenReady = true;
	ShortcutInteractComp->bShowScreenPromptTextWhenLocked = false; // 재료 부족 시 하단 프롬프트는 숨기고 머리 위 위젯만 표시
	ShortcutInteractComp->CanUnlockPromptText = FText::FromString(TEXT("동굴 숏컷 개방"));
	ShortcutInteractComp->NeedCrystalPromptText = FText::FromString(TEXT("이상한 차원 결정이 필요합니다"));
	ShortcutInteractComp->WidgetRelativeOffset = FVector(0.0f, 0.0f, 120.0f);

	static ConstructorHelpers::FClassFinder<USOVShortcutRequirementWidget> RequirementWidgetFinder(TEXT("/Game/HYH/Blueprints/Widgets/UIWidget/WBP_ShortcutRequirement.WBP_ShortcutRequirement_C"));
	if (RequirementWidgetFinder.Succeeded())
	{
		ShortcutInteractComp->RequirementWidgetClass = RequirementWidgetFinder.Class;
	}
}

void AAGSDShortcutPit::BeginPlay()
{
	Super::BeginPlay();

	// GameInstance에서 숏컷 개방 상태 확인 및 동기화
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		const FName TagToUse = ShortcutInteractComp ? ShortcutInteractComp->ShortcutTag : FName("Cave");
		bIsUnlocked = GI->IsShortcutOpened(TagToUse);
		GI->OnShortcutStateChanged.AddDynamic(this, &AAGSDShortcutPit::OnShortcutStateChangedCallback);
	}

	if (ShortcutInteractComp)
	{
		ShortcutInteractComp->OnShortcutUnlocked.AddDynamic(this, &AAGSDShortcutPit::OnCompShortcutUnlocked);
		ShortcutInteractComp->OnShortcutAlreadyUnlocked.AddDynamic(this, &AAGSDShortcutPit::OnCompShortcutAlreadyUnlocked);

		// [핵심 버그 수정] 위젯 컴포넌트가 월드 원점(0, 0, 120)으로 고립되는 현상 방지:
		// 구덩이의 SceneRoot에 명시적으로 다시 어태치하고 로컬 상대 좌표를 확실히 보정합니다.
		if (ShortcutInteractComp->WorldWidgetComponent)
		{
			ShortcutInteractComp->WorldWidgetComponent->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
			ShortcutInteractComp->WorldWidgetComponent->SetRelativeLocation(ShortcutInteractComp->WidgetRelativeOffset);
			ShortcutInteractComp->WorldWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
			ShortcutInteractComp->WorldWidgetComponent->SetDrawSize(ShortcutInteractComp->WidgetDrawSize);
			if (ShortcutInteractComp->RequirementWidgetClass)
			{
				ShortcutInteractComp->WorldWidgetComponent->SetWidgetClass(ShortcutInteractComp->RequirementWidgetClass);
			}
			ShortcutInteractComp->WorldWidgetComponent->InitWidget();
			ShortcutInteractComp->WorldWidgetComponent->SetHiddenInGame(true);
			ShortcutInteractComp->WorldWidgetComponent->SetVisibility(false);
		}

		// 입구 쪽 구덩이는 결정을 바치는 곳이 아니므로 컴포넌트 상호작용 및 위젯 비활성화
		if (!bIsBossRoomPit)
		{
			ShortcutInteractComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ShortcutInteractComp->SetGenerateOverlapEvents(false);
			if (ShortcutInteractComp->WorldWidgetComponent)
			{
				ShortcutInteractComp->WorldWidgetComponent->SetHiddenInGame(true);
				ShortcutInteractComp->WorldWidgetComponent->SetVisibility(false);
			}
		}
	}

	if (bIsUnlocked)
	{
		SetShortcutUnlocked(true);
	}
}

void AAGSDShortcutPit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->OnShortcutStateChanged.RemoveDynamic(this, &AAGSDShortcutPit::OnShortcutStateChangedCallback);
	}

	if (ShortcutInteractComp)
	{
		ShortcutInteractComp->OnShortcutUnlocked.RemoveDynamic(this, &AAGSDShortcutPit::OnCompShortcutUnlocked);
		ShortcutInteractComp->OnShortcutAlreadyUnlocked.RemoveDynamic(this, &AAGSDShortcutPit::OnCompShortcutAlreadyUnlocked);
	}

	if (GetWorldTimerManager().IsTimerActive(TeleportTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(TeleportTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AAGSDShortcutPit::OnCompShortcutUnlocked(AAGSDCharacter* UnlockingPlayer)
{
	SetShortcutUnlocked(true);

	// 연결된 반대편 구덩이도 함께 해금
	if (LinkedPit)
	{
		LinkedPit->SetShortcutUnlocked(true);
	}

	if (AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(UnlockingPlayer ? UnlockingPlayer->GetController() : nullptr))
	{
		PC->ShowWarningNotice(FText::FromString(TEXT("동굴 숏컷이 활성화되었습니다!")));
		ShowWidget_Implementation(UnlockingPlayer);
	}
}

void AAGSDShortcutPit::OnCompShortcutAlreadyUnlocked(bool bWasOpened)
{
	SetShortcutUnlocked(true);
}

void AAGSDShortcutPit::OnShortcutStateChangedCallback(FName ChangedTag, bool bOpened)
{
	const FName CurrentTag = ShortcutInteractComp ? ShortcutInteractComp->ShortcutTag : FName("Cave");
	if (ChangedTag == CurrentTag)
	{
		SetShortcutUnlocked(bOpened);
	}
}

void AAGSDShortcutPit::SetShortcutUnlocked(bool bUnlocked)
{
	bIsUnlocked = bUnlocked;
	if (bIsUnlocked)
	{
		// 해금되면 결정 바치기 컴포넌트의 오버랩과 위젯을 비활성화하여 텔레포트 전용 모드로 전환
		if (ShortcutInteractComp)
		{
			ShortcutInteractComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ShortcutInteractComp->SetGenerateOverlapEvents(false);
			if (ShortcutInteractComp->WorldWidgetComponent)
			{
				ShortcutInteractComp->WorldWidgetComponent->SetHiddenInGame(true);
				ShortcutInteractComp->WorldWidgetComponent->SetVisibility(false);
			}
		}

		OnShortcutUnlocked_BP();
	}
}

FVector AAGSDShortcutPit::GetArrivalLocation() const
{
	if (ArrivalPoint)
	{
		return ArrivalPoint->GetComponentLocation();
	}
	return GetActorLocation() + GetActorForwardVector() * 200.0f;
}

FRotator AAGSDShortcutPit::GetArrivalRotation() const
{
	if (ArrivalPoint)
	{
		return ArrivalPoint->GetComponentRotation();
	}
	return GetActorRotation();
}

int32 AAGSDShortcutPit::GetPlayerItemCount(AAGSDCharacter* Player) const
{
	if (ShortcutInteractComp)
	{
		return ShortcutInteractComp->GetPlayerHoldingItemCount(Player);
	}
	return 0;
}

void AAGSDShortcutPit::ShowWidget_Implementation(ACharacter* player)
{
	AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(player ? player->GetController() : nullptr);
	if (!PC) return;

	if (bIsUnlocked)
	{
		PC->ShowInteractionWidget(TeleportPromptText);
	}
	else
	{
		if (bIsBossRoomPit && ShortcutInteractComp)
		{
			ShortcutInteractComp->HandleShowWidget(player);
		}
		else
		{
			// 해금 전 입구 구덩이는 하단 인터랙트 창을 일절 띄우지 않음
			PC->HideInteractionWidget();
		}
	}
}

bool AAGSDShortcutPit::CanInteract_Implementation(AAGSDCharacter* player)
{
	if (bIsTeleporting)
	{
		return false;
	}

	if (bIsUnlocked)
	{
		return LinkedPit != nullptr;
	}

	// 해금 전: 보스방 앞 구덩이는 컴포넌트에 위임 (결정 3개 소지 시에만 true)
	if (bIsBossRoomPit && ShortcutInteractComp)
	{
		return ShortcutInteractComp->HandleCanInteract(player);
	}

	// 해금 전 입구 구덩이는 상호작용 불가
	return false;
}

void AAGSDShortcutPit::Interact_Implementation(AAGSDCharacter* player)
{
	if (!player || bIsTeleporting)
	{
		return;
	}

	if (bIsUnlocked)
	{
		if (!LinkedPit)
		{
			if (AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(player->GetController()))
			{
				PC->ShowWarningNotice(FText::FromString(TEXT("연결된 목적지 구덩이가 없습니다.")));
			}
			return;
		}

		StartTeleport(player);
	}
	else
	{
		if (bIsBossRoomPit && ShortcutInteractComp)
		{
			ShortcutInteractComp->HandleInteract(player);
		}
	}
}

void AAGSDShortcutPit::StartTeleport(AAGSDCharacter* Player)
{
	if (!Player || !LinkedPit || bIsTeleporting)
	{
		return;
	}

	// [방어 코드] 자기 자신으로 설정되어 있는 경우 무한 루프 / 제자리 소환 방지
	if (LinkedPit == this)
	{
		if (AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(Player->GetController()))
		{
			PC->ShowWarningNotice(FText::FromString(TEXT("Linked Pit이 자기 자신으로 지정되어 있습니다. 디테일 패널에서 반대편 구덩이를 지정해 주세요.")));
		}
		return;
	}

	bIsTeleporting = true;
	TeleportingPlayer = Player;

	OnTeleportStarted_BP(Player);

	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC && PC->PlayerCameraManager)
	{
		// 1단계: 화면 암전 (페이드 아웃: 0.0 -> 1.0)
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, TeleportFadeDuration, FLinearColor::Black, false, true);
	}

	// 페이드 아웃 완료 후 실제 위치 이동 실행
	GetWorldTimerManager().SetTimer(TeleportTimerHandle, this, &AAGSDShortcutPit::PerformTeleport, TeleportFadeDuration, false);
}

void AAGSDShortcutPit::PerformTeleport()
{
	if (!TeleportingPlayer.IsValid() || !LinkedPit)
	{
		bIsTeleporting = false;
		return;
	}

	AAGSDCharacter* Player = TeleportingPlayer.Get();

	// 2단계: 목적지 구덩이의 ArrivalPoint 위치 및 회전으로 플레이어 강제 이동
	// [핵심 버그 수정] TeleportTo의 콜리전 겹침 취소 현상을 방지하기 위해 SetActorLocationAndRotation(TeleportPhysics) 사용
	const FVector TargetLocation = LinkedPit->GetArrivalLocation() + FVector(0.0f, 0.0f, ArrivalZOffset);
	const FRotator TargetRotation = LinkedPit->GetArrivalRotation();

	Player->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	Player->ResetCameraLag();

	if (AController* Controller = Player->GetController())
	{
		Controller->SetControlRotation(TargetRotation);
	}

	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC && PC->PlayerCameraManager)
	{
		// 3단계: 화면 밝아짐 (페이드 인: 1.0 -> 0.0)
		PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, TeleportFadeDuration, FLinearColor::Black, false, false);
	}

	OnTeleportFinished_BP(Player);

	bIsTeleporting = false;
	TeleportingPlayer = nullptr;
}

void AAGSDShortcutPit::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAGSDCharacter* Player = Cast<AAGSDCharacter>(OtherActor))
	{
		// 1) 해금 전 동굴 입구 구덩이인 경우: 하단 인터랙트 창은 일절 띄우지 않고, 화면 중앙 경고창으로만 알림 출력
		if (!bIsUnlocked && !bIsBossRoomPit)
		{
			if (!bHasShownNotice)
			{
				if (AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(Player->GetController()))
				{
					PC->ShowWarningNotice(NotUnlockedNoticeText);
				}
				bHasShownNotice = true;
			}
			return;
		}

		// 2) 플레이어 인터랙션 장부에 등록
		if (IInteractionOwnerInterface* OwnerInterface = Cast<IInteractionOwnerInterface>(Player))
		{
			if (UAGSDInteractionComponent* InterComp = OwnerInterface->GetInteractionComponent())
			{
				InterComp->AddInteractableActor(this);
			}
		}

		// 3) 보스방 구덩이 또는 해금 완료 구덩이에 따른 UI 출력
		if (bIsUnlocked)
		{
			ShowWidget_Implementation(Player);
		}
		else if (bIsBossRoomPit && ShortcutInteractComp)
		{
			// [핵심 버그 수정] 재료 소지 여부(0개)와 상관없이 보스방 구덩이에 접근하면 즉시 머리 위 3D 위젯(0/3) 출력!
			ShortcutInteractComp->HandleShowWidget(Player);
		}
	}
}

void AAGSDShortcutPit::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAGSDCharacter* Player = Cast<AAGSDCharacter>(OtherActor))
	{
		// 입구 구덩이 경고창 출력 플래그 리셋 (구덩이를 벗어났다가 다시 다가오면 안내 출력 가능)
		bHasShownNotice = false;

		if (IInteractionOwnerInterface* OwnerInterface = Cast<IInteractionOwnerInterface>(Player))
		{
			if (UAGSDInteractionComponent* InterComp = OwnerInterface->GetInteractionComponent())
			{
				InterComp->RemoveInteractableActor(this);
			}
		}

		// 보스방 구덩이 위젯 숨김
		if (!bIsUnlocked && bIsBossRoomPit && ShortcutInteractComp)
		{
			ShortcutInteractComp->HideInteractionUI(Player);
		}

		if (AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(Player->GetController()))
		{
			PC->HideInteractionWidget();
		}
	}
}
