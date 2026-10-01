#include "TutorialSubsystem.h"
#include "BaseFlyingPet.h"
#include "Component/PetGuideComponent.h"
#include "Character/AGSDCharacter.h"
#include "GameplayLogSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

UTutorialSubsystem::UTutorialSubsystem()
{
}

void UTutorialSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bIsActive = false;
	CurrentStepIndex = 0;
	CurrentActionCount = 0;
}

void UTutorialSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
	}
	Super::Deinitialize();
}

void UTutorialSubsystem::StartTutorialSequence(UDataTable* TutorialTable, FName SequenceName)
{
	if (!TutorialTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] TutorialTable is null!"));
		return;
	}

	CurrentSteps.Empty();
	TArray<FTutorialStepData*> RowPtrs;
	TutorialTable->GetAllRows<FTutorialStepData>(TEXT("TutorialSubsystem"), RowPtrs);

	for (FTutorialStepData* Row : RowPtrs)
	{
		if (Row)
		{
			CurrentSteps.Add(*Row);
		}
	}

	if (CurrentSteps.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] No steps found in DataTable for sequence %s"), *SequenceName.ToString());
		return;
	}

	CurrentStepIndex = 0;
	CurrentActionCount = 0;
	CurrentSequenceName = SequenceName;
	bIsActive = true;

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Started tutorial sequence: %s (Steps: %d)"), *SequenceName.ToString(), CurrentSteps.Num());

	SetupCurrentStepVisuals();
	OnTutorialStepStarted.Broadcast(CurrentSteps[0]);
}

void UTutorialSubsystem::ReportTutorialAction(ETutorialActionType ActionType, int32 Count, FName CustomTag)
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];

	// 검증할 액션 일치 확인
	if (CurrentStep.ActionType != ActionType)
	{
		return;
	}

	// 커스텀 액션인 경우 태그 일치 확인
	if (ActionType == ETutorialActionType::Custom && !CurrentStep.CustomActionTag.IsNone() && CurrentStep.CustomActionTag != CustomTag)
	{
		return;
	}

	CurrentActionCount += Count;
	OnTutorialStepProgress.Broadcast(CurrentActionCount, CurrentStep.RequiredActionCount);

	// 목표 횟수 달성 여부 확인
	if (CurrentActionCount >= CurrentStep.RequiredActionCount)
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Step [%s] Completed!"), *CurrentStep.StepID.ToString());

		OnTutorialStepCompleted.Broadcast(CurrentStep);

		// 통과 시 열리는 장벽/문 해제
		if (!CurrentStep.GateActorTag.IsNone())
		{
			OpenGateActor(CurrentStep.GateActorTag);
		}

		AdvanceToNextStep();
	}
}

void UTutorialSubsystem::ReportInteractionAction(AActor* InteractedActor, const FString& InteractionType)
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];

	// 상호작용 관련 액션 타입인지 판별
	bool bIsInteractionStep = false;
	if (CurrentStep.ActionType == ETutorialActionType::Interact)
	{
		bIsInteractionStep = true;
	}
	else if (CurrentStep.ActionType == ETutorialActionType::WeedHarvest ||
			 CurrentStep.ActionType == ETutorialActionType::EnterPortal ||
			 CurrentStep.ActionType == ETutorialActionType::DungeonGate ||
			 CurrentStep.ActionType == ETutorialActionType::TributeAltar ||
			 CurrentStep.ActionType == ETutorialActionType::AlchemyTable ||
			 CurrentStep.ActionType == ETutorialActionType::OrbAltar ||
			 CurrentStep.ActionType == ETutorialActionType::PortalExit)
	{
		bIsInteractionStep = true;
	}

	if (!bIsInteractionStep)
	{
		return;
	}

	// 1. 목표 웨이포인트 태그가 지정되어 있는 경우: 목표 액터이거나 태그를 가진 액터인지 검증
	if (!CurrentStep.TargetWaypointTag.IsNone())
	{
		bool bActorMatches = false;
		if (CachedWaypointActor.IsValid() && CachedWaypointActor.Get() == InteractedActor)
		{
			bActorMatches = true;
		}
		else if (IsValid(InteractedActor) && InteractedActor->ActorHasTag(CurrentStep.TargetWaypointTag))
		{
			bActorMatches = true;
		}

		if (!bActorMatches)
		{
			return;
		}
	}

	// 2. 커스텀 액션 태그가 지정되어 있는 경우: 액터의 태그 또는 상호작용 타입 문자열 일치 검증
	if (!CurrentStep.CustomActionTag.IsNone())
	{
		bool bTagMatches = false;
		if (IsValid(InteractedActor) && InteractedActor->ActorHasTag(CurrentStep.CustomActionTag))
		{
			bTagMatches = true;
		}
		else if (!InteractionType.IsEmpty() && InteractionType.Equals(CurrentStep.CustomActionTag.ToString(), ESearchCase::IgnoreCase))
		{
			bTagMatches = true;
		}

		if (!bTagMatches)
		{
			return;
		}
	}

	// 검증 통과 시 해당 스텝의 행동 달성 처리
	ReportTutorialAction(CurrentStep.ActionType, 1);
}

void UTutorialSubsystem::AdvanceToNextStep()
{
	CurrentStepIndex++;
	CurrentActionCount = 0;

	if (CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		SetupCurrentStepVisuals();
		OnTutorialStepStarted.Broadcast(CurrentSteps[CurrentStepIndex]);
	}
	else
	{
		// 모든 스텝 완료
		bIsActive = false;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		}

		if (CurrentSequenceName == FName("Farm_Sky_Island") || CurrentSequenceName.ToString().Contains(TEXT("Hub")))
		{
			bCompletedHubTutorial = true;
		}

		// 펫을 플레이어 추적 상태로 복귀
		ABaseFlyingPet* Pet = GetPlayerPet();
		if (Pet && Pet->GetPetGuideComponent())
		{
			Pet->GetPetGuideComponent()->ReturnToFollow();
		}

		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Sequence [%s] Fully Completed!"), *CurrentSequenceName.ToString());
		OnTutorialSequenceCompleted.Broadcast(CurrentSequenceName);
	}
}

void UTutorialSubsystem::SkipCurrentSequence()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Skipping sequence: %s"), *CurrentSequenceName.ToString());

	bIsActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
	}

	// 남아있는 모든 단계의 장벽/문 해제
	for (int32 i = CurrentStepIndex; i < CurrentSteps.Num(); ++i)
	{
		if (!CurrentSteps[i].GateActorTag.IsNone())
		{
			OpenGateActor(CurrentSteps[i].GateActorTag);
		}
	}

	if (CurrentSequenceName == FName("Farm_Sky_Island") || CurrentSequenceName.ToString().Contains(TEXT("Hub")))
	{
		bCompletedHubTutorial = true;
	}

	// 펫 복귀
	ABaseFlyingPet* Pet = GetPlayerPet();
	if (Pet && Pet->GetPetGuideComponent())
	{
		Pet->GetPetGuideComponent()->ReturnToFollow();
	}

	// 게임플레이 로그에 스킵 기록 반영
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameplayLogSubsystem* LogSub = GI->GetSubsystem<UGameplayLogSubsystem>())
		{
			LogSub->IncrementTutorialFullSkip();
		}
	}

	OnTutorialSequenceCompleted.Broadcast(CurrentSequenceName);
}

bool UTutorialSubsystem::GetCurrentStepData(FTutorialStepData& OutStepData) const
{
	if (bIsActive && CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		OutStepData = CurrentSteps[CurrentStepIndex];
		return true;
	}
	return false;
}

void UTutorialSubsystem::SetHubTutorialCompleted(bool bCompleted)
{
	bCompletedHubTutorial = bCompleted;
}

AActor* UTutorialSubsystem::GetCurrentWaypointActor() const
{
	return CachedWaypointActor.Get();
}

FVector UTutorialSubsystem::GetCurrentTargetLocation() const
{
	if (CachedWaypointActor.IsValid())
	{
		return CachedWaypointActor->GetActorLocation();
	}

	ABaseFlyingPet* Pet = GetPlayerPet();
	if (Pet && Pet->GetPetGuideComponent())
	{
		return Pet->GetPetGuideComponent()->GetCurrentTargetLocation();
	}

	return FVector::ZeroVector;
}

void UTutorialSubsystem::SetupCurrentStepVisuals()
{
	if (!CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FTutorialStepData& Step = CurrentSteps[CurrentStepIndex];

	// 월드 내 타겟 웨이포인트 액터 검색
	CachedWaypointActor = nullptr;
	if (!Step.TargetWaypointTag.IsNone())
	{
		CachedWaypointActor = FindActorWithTag(Step.TargetWaypointTag);
		if (!CachedWaypointActor.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] Target Waypoint Actor with Tag '%s' was NOT FOUND in World!"), *Step.TargetWaypointTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Found Target Waypoint Actor: %s (Tag: %s)"), *CachedWaypointActor->GetName(), *Step.TargetWaypointTag.ToString());
		}
	}

	// 동반자 펫 안내 제어
	ABaseFlyingPet* Pet = GetPlayerPet();
	if (Pet && Pet->GetPetGuideComponent())
	{
		UPetGuideComponent* Guide = Pet->GetPetGuideComponent();

		// 메쉬 가시성 제어 (마을 맵 투명 이정표 연출 등)
		Guide->SetPetMeshHidden(Step.bHidePetMesh);

		if (CachedWaypointActor.IsValid() && Step.bSendPetToWaypoint)
		{
			if (Step.bTeleportPet)
			{
				Guide->TeleportToActor(CachedWaypointActor.Get());
			}
			else
			{
				Guide->MoveToActor(CachedWaypointActor.Get());
			}
		}
		else
		{
			// 펫이 웨이포인트 액터로 이동하지 않는 스텝이면 플레이어 추적으로 복귀/유지
			Guide->ReturnToFollow();
		}

		// 대화 시작
		if (!Step.DialogueID.IsNone())
		{
			Pet->StartBigConversation(Step.DialogueID);
		}
	}

	// ReachArea 자동 거리 감지 타이머 설정
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		if (Step.ActionType == ETutorialActionType::ReachArea)
		{
			World->GetTimerManager().SetTimer(ReachAreaTimerHandle, this, &UTutorialSubsystem::CheckPlayerReachArea, 0.2f, true);
		}
	}
}

void UTutorialSubsystem::CheckPlayerReachArea()
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		}
		return;
	}

	const FTutorialStepData& Step = CurrentSteps[CurrentStepIndex];
	if (Step.ActionType != ETutorialActionType::ReachArea)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	FVector TargetLocation = FVector::ZeroVector;
	if (CachedWaypointActor.IsValid())
	{
		TargetLocation = CachedWaypointActor->GetActorLocation();
	}
	else
	{
		TargetLocation = GetCurrentTargetLocation();
	}

	if (TargetLocation.IsNearlyZero())
	{
		return;
	}

	FVector PlayerLoc = PlayerPawn->GetActorLocation();
	float Dist2D = FVector::Dist2D(PlayerLoc, TargetLocation);
	float DiffZ = FMath::Abs(PlayerLoc.Z - TargetLocation.Z);

	// 스텝 데이터에 개별 지정된 반경이 있으면 우선 적용하고, 0 이하면 서브시스템 기본값 사용
	const float EffectiveRadius = (Step.ReachAreaRadius > 0.0f) ? Step.ReachAreaRadius : ReachAreaDistanceThreshold;

	// 수평 반경 및 수직 높이 오차 범위 이내 진입 시 지점 도달 완료 판정
	if (Dist2D <= EffectiveRadius && DiffZ <= ReachAreaZThreshold)
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		ReportTutorialAction(ETutorialActionType::ReachArea, 1);
	}
}

void UTutorialSubsystem::OpenGateActor(FName GateTag)
{
	UWorld* World = GetWorld();
	if (!World || GateTag.IsNone())
	{
		return;
	}

	TArray<AActor*> GateActors;
	UGameplayStatics::GetAllActorsWithTag(World, GateTag, GateActors);

	for (AActor* Actor : GateActors)
	{
		if (Actor)
		{
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Gate actor [%s] opened/disabled."), *Actor->GetName());
		}
	}
}

AActor* UTutorialSubsystem::FindActorWithTag(FName Tag) const
{
	UWorld* World = GetWorld();
	if (!World || Tag.IsNone())
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(World, Tag, FoundActors);

	if (FoundActors.Num() > 0)
	{
		return FoundActors[0];
	}

	return nullptr;
}

ABaseFlyingPet* UTutorialSubsystem::GetPlayerPet() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(PC->GetPawn());
	if (PlayerChar)
	{
		return PlayerChar->GetPet();
	}

	// 캐릭터에서 못 찾았을 경우 태그로 월드 검색
	TArray<AActor*> PetActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("Pet"), PetActors);
	for (AActor* Actor : PetActors)
	{
		if (ABaseFlyingPet* Pet = Cast<ABaseFlyingPet>(Actor))
		{
			return Pet;
		}
	}

	return nullptr;
}
