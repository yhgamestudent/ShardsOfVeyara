#include "TutorialSubsystem.h"
#include "BaseFlyingPet.h"
#include "Component/PetGuideComponent.h"
#include "Component/PetTalkComponent.h"
#include "Character/AGSDCharacter.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Inventory/UI/AGSDPlayerHUD.h"
#include "GameplayLogSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Interface/ItemDropInterface.h"

UTutorialSubsystem::UTutorialSubsystem()
{
}

void UTutorialSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bIsActive = false;
	bWaitingForDialogue = false;
	CurrentStepIndex = 0;
	CurrentActionCount = 0;
	PendingLoadedMapName = NAME_None;

	// 엔진 레벨 전환(맵 로드 완료) 감지 델리게이트 등록
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTutorialSubsystem::OnPostLoadMapWithWorld);
}

void UTutorialSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		World->GetTimerManager().ClearTimer(LevelTransitionTimerHandle);
	}
	if (ABaseFlyingPet* Pet = GetPlayerPet())
	{
		if (Pet->GetPetTalkComponent())
		{
			Pet->GetPetTalkComponent()->OnConversationEnded.RemoveDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
		}
	}
	ClearTargetHighlight();
	bWaitingForDialogue = false;
	Super::Deinitialize();
}

void UTutorialSubsystem::StartTutorialSequence(UDataTable* TutorialTable, FName SequenceName)
{
	if (!TutorialTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] TutorialTable is null!"));
		return;
	}

	// 이미 완료된 시퀀스인 경우 1회 실행 보장을 위해 재시작 차단
	if (IsSequenceCompleted(SequenceName))
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] StartTutorialSequence IGNORED: Sequence [%s] is ALREADY COMPLETED."), *SequenceName.ToString());
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

	ClearTargetHighlight();
	CurrentStepIndex = 0;
	CurrentActionCount = 0;
	CurrentObjectiveCounts.Empty();
	if (CurrentSteps.IsValidIndex(0) && CurrentSteps[0].SubObjectives.Num() > 0)
	{
		CurrentObjectiveCounts.Init(0, CurrentSteps[0].SubObjectives.Num());
	}
	CurrentSequenceName = SequenceName;
	bIsActive = true;
	bWaitingForDialogue = false;

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Started tutorial sequence: %s (Steps: %d)"), *SequenceName.ToString(), CurrentSteps.Num());

	SetupCurrentStepVisuals();

	// 대화가 진행 중이지 않은 경우에만 즉시 퀘스트 UI 브로드캐스트
	if (!bWaitingForDialogue)
	{
		OnTutorialStepStarted.Broadcast(CurrentSteps[0]);
	}
}

void UTutorialSubsystem::ReportTutorialAction(ETutorialActionType ActionType, int32 Count, FName CustomTag)
{
	if (!bIsActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ReportTutorialAction IGNORED: Tutorial is NOT active. (Action: %d)"), (int32)ActionType);
		return;
	}
	if (bWaitingForDialogue)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ReportTutorialAction IGNORED: Waiting for dialogue. (Action: %d)"), (int32)ActionType);
		return;
	}
	if (!CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ReportTutorialAction IGNORED: Invalid step index (%d)."), CurrentStepIndex);
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];

	// 1. 다중 세부 목표(SubObjectives)가 정의되어 있는 경우 (병렬 동시 진행)
	if (CurrentStep.SubObjectives.Num() > 0)
	{
		bool bAnyProgressed = false;

		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];

			// 이미 달성한 목표는 건너뜀
			if (CurrentObjectiveCounts.IsValidIndex(i) && CurrentObjectiveCounts[i] >= Obj.RequiredActionCount)
			{
				continue;
			}

			// 1. 액션 타입 검사
			if (Obj.ActionType != ActionType)
			{
				continue;
			}

			// 2. CustomActionTag 검사 (ActionType이 Custom이든 Interact이든 검증, 단 ReachArea는 위치 도달이 완료된 경우 예외 허용)
			if (!Obj.CustomActionTag.IsNone())
			{
				if (Obj.CustomActionTag != CustomTag && ActionType != ETutorialActionType::ReachArea)
				{
					continue;
				}
			}
			else if (!CustomTag.IsNone())
			{
				// 보고된 액션에는 CustomTag가 있는데, 이 목표에는 CustomActionTag가 지정되지 않은 경우:
				// 다른 세부 목표 중에 해당 CustomTag를 전용으로 처리하는 목표가 있다면 그 목표에 양보
				bool bOtherMatchesTag = false;
				for (int32 j = 0; j < CurrentStep.SubObjectives.Num(); ++j)
				{
					if (j != i && CurrentStep.SubObjectives[j].ActionType == ActionType && CurrentStep.SubObjectives[j].CustomActionTag == CustomTag)
					{
						bOtherMatchesTag = true;
						break;
					}
				}
				if (bOtherMatchesTag)
				{
					continue;
				}
			}

			// 3. TargetLevelName 검사 (지정된 경우 맵 이름 일치 확인)
			if (!Obj.TargetLevelName.IsNone() && !CustomTag.IsNone())
			{
				if (!CustomTag.ToString().Equals(Obj.TargetLevelName.ToString(), ESearchCase::IgnoreCase))
				{
					continue;
				}
			}

			// 목표 일치! 카운트 증가
			if (CurrentObjectiveCounts.IsValidIndex(i))
			{
				CurrentObjectiveCounts[i] += Count;
				bAnyProgressed = true;
				UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] SubObjective [%d: %s] Progress: %d / %d (ActionType: %d, Tag: %s)"),
					i, *Obj.ObjectiveDescription.ToString(), CurrentObjectiveCounts[i], Obj.RequiredActionCount,
					(int32)ActionType, *CustomTag.ToString());
				break; // 한 번의 행동은 하나의 미완료 목표에 반영
			}
		}

		if (!bAnyProgressed)
		{
			return;
		}

		// 전체 목표 달성 여부 확인 및 합산 카운트 계산
		bool bAllCompleted = true;
		int32 TotalCurrent = 0;
		int32 TotalRequired = 0;

		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			int32 Cur = CurrentObjectiveCounts.IsValidIndex(i) ? CurrentObjectiveCounts[i] : 0;
			TotalCurrent += FMath::Min(Cur, Obj.RequiredActionCount);
			TotalRequired += Obj.RequiredActionCount;

			if (Cur < Obj.RequiredActionCount)
			{
				bAllCompleted = false;
			}
		}

		CurrentActionCount = TotalCurrent;

		// 가장 가까운 다음 미완료 목표 지점으로 안내 갱신
		UpdateNearestWaypoint();

		OnTutorialStepProgress.Broadcast(TotalCurrent, TotalRequired);

		if (bAllCompleted)
		{
			CompleteCurrentStep();
		}
		return;
	}

	// 2. 기존 단일 목표 처리 (SubObjectives가 비어있는 경우)
	if (CurrentStep.ActionType != ActionType)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ActionType mismatch: Step requires %d, but reported %d"),
			(int32)CurrentStep.ActionType, (int32)ActionType);
		return;
	}

	if (!CurrentStep.CustomActionTag.IsNone() && CurrentStep.CustomActionTag != CustomTag)
	{
		// ReachArea는 이미 위치/웨이포인트 도달이 검증된 상태이므로 CustomTag가 없거나 다르더라도 유연하게 통과 허용
		if (ActionType != ETutorialActionType::ReachArea)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] CustomActionTag mismatch: Step requires '%s', but reported '%s'"),
				*CurrentStep.CustomActionTag.ToString(), *CustomTag.ToString());
			return;
		}
	}

	if (!CurrentStep.TargetLevelName.IsNone() && !CustomTag.IsNone())
	{
		if (!CustomTag.ToString().Equals(CurrentStep.TargetLevelName.ToString(), ESearchCase::IgnoreCase))
		{
			return;
		}
	}

	CurrentActionCount += Count;
	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Progress updated: %d / %d (Action: %d)"),
		CurrentActionCount, CurrentStep.RequiredActionCount, (int32)ActionType);
	OnTutorialStepProgress.Broadcast(CurrentActionCount, CurrentStep.RequiredActionCount);

	// 목표 횟수 달성 여부 확인
	if (CurrentActionCount >= CurrentStep.RequiredActionCount)
	{
		CompleteCurrentStep();
	}
}

void UTutorialSubsystem::CompleteCurrentStep()
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];
	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Step [%s] Completed!"), *CurrentStep.StepID.ToString());

	OnTutorialStepCompleted.Broadcast(CurrentStep);
	ClearTargetHighlight();

	// 통과 시 열리는 장벽/문 해제
	if (!CurrentStep.GateActorTag.IsNone())
	{
		OpenGateActor(CurrentStep.GateActorTag);
	}

	// 스텝 완료 시 보상 아이템 지급
	if (!CurrentStep.RewardItemID.IsNone() && CurrentStep.RewardItemCount > 0)
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn()))
				{
					if (Character->InventoryComponent)
					{
						int32 OutRemaining = 0;
						FStruct_ItemData OutItemData;
						bool bAdded = Character->InventoryComponent->AddItemByID(
							CurrentStep.RewardItemID.ToString(),
							CurrentStep.RewardItemCount,
							OutRemaining,
							OutItemData
						);

						if (bAdded)
						{
							if (Character->PlayerHUDRef)
							{
								Character->PlayerHUDRef->AddItemNotification(OutItemData, CurrentStep.RewardItemCount - OutRemaining);
							}
							UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Granted Reward: %s x%d"),
								*CurrentStep.RewardItemID.ToString(), CurrentStep.RewardItemCount - OutRemaining);
						}
					}
				}
			}
		}
	}

	AdvanceToNextStep();
}

void UTutorialSubsystem::ReportInteractionAction(AActor* InteractedActor, const FString& InteractionType)
{
	if (!bIsActive || bWaitingForDialogue || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];

	// 1. 다중 세부 목표인 경우: 일치하는 세부 목표를 탐색하여 카운트 반영
	if (CurrentStep.SubObjectives.Num() > 0)
	{
		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			if (CurrentObjectiveCounts.IsValidIndex(i) && CurrentObjectiveCounts[i] >= Obj.RequiredActionCount)
			{
				continue;
			}

			bool bIsInteractionType = (Obj.ActionType == ETutorialActionType::Interact ||
									   Obj.ActionType == ETutorialActionType::WeedHarvest ||
									   Obj.ActionType == ETutorialActionType::PlantSeed ||
									   Obj.ActionType == ETutorialActionType::EnterPortal ||
									   Obj.ActionType == ETutorialActionType::DungeonGate ||
									   Obj.ActionType == ETutorialActionType::TributeAltar ||
									   Obj.ActionType == ETutorialActionType::AlchemyTable ||
									   Obj.ActionType == ETutorialActionType::OrbAltar ||
									   Obj.ActionType == ETutorialActionType::PortalExit ||
									   Obj.ActionType == ETutorialActionType::Custom);

			if (!bIsInteractionType)
			{
				continue;
			}

			bool bMatches = false;

			// CustomActionTag 우선 검증
			if (!Obj.CustomActionTag.IsNone())
			{
				if (IsValid(InteractedActor) && InteractedActor->ActorHasTag(Obj.CustomActionTag))
				{
					bMatches = true;
				}
				else if (!InteractionType.IsEmpty() &&
					(InteractionType.Equals(Obj.CustomActionTag.ToString(), ESearchCase::IgnoreCase) ||
					 InteractionType.Contains(Obj.CustomActionTag.ToString(), ESearchCase::IgnoreCase)))
				{
					bMatches = true;
				}
			}
			// TargetWaypointTag 검증
			else if (!Obj.TargetWaypointTag.IsNone())
			{
				if (IsValid(InteractedActor) && InteractedActor->ActorHasTag(Obj.TargetWaypointTag))
				{
					bMatches = true;
				}
			}
			else
			{
				// 태그 제한 없는 일반 상호작용
				bMatches = true;
			}

			if (bMatches)
			{
				UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Multi-Objective Interaction Verified: Index [%d: %s]"),
					i, *Obj.ObjectiveDescription.ToString());
				ReportTutorialAction(Obj.ActionType, 1, Obj.CustomActionTag);
				return;
			}
		}
		return;
	}

	// 2. 단일 목표 검증
	bool bIsInteractionStep = false;
	if (CurrentStep.ActionType == ETutorialActionType::Interact)
	{
		bIsInteractionStep = true;
	}
	else if (CurrentStep.ActionType == ETutorialActionType::WeedHarvest ||
			 CurrentStep.ActionType == ETutorialActionType::PlantSeed ||
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

	// 상호작용 대상 검증
	// 1) CustomActionTag가 지정되어 있는 경우:
	if (!CurrentStep.CustomActionTag.IsNone())
	{
		bool bTagMatches = false;
		if (IsValid(InteractedActor) && InteractedActor->ActorHasTag(CurrentStep.CustomActionTag))
		{
			bTagMatches = true;
		}
		else if (!InteractionType.IsEmpty())
		{
			if (InteractionType.Equals(CurrentStep.CustomActionTag.ToString(), ESearchCase::IgnoreCase) ||
				InteractionType.Contains(CurrentStep.CustomActionTag.ToString(), ESearchCase::IgnoreCase))
			{
				bTagMatches = true;
			}
		}

		// PlantSeed 액션의 경우 상호작용 타입이 Planting이거나 대상이 밭 액터(CultivationPlot)이면 자동 매칭
		if (CurrentStep.ActionType == ETutorialActionType::PlantSeed)
		{
			if (InteractionType.Equals(TEXT("Planting"), ESearchCase::IgnoreCase) ||
				InteractionType.Equals(TEXT("CultivationPlot"), ESearchCase::IgnoreCase) ||
				(IsValid(InteractedActor) && InteractedActor->GetName().Contains(TEXT("CultivationPlot"))))
			{
				bTagMatches = true;
			}
		}

		if (!bTagMatches)
		{
			return;
		}
	}
	// 2) CustomActionTag가 없고 TargetWaypointTag만 지정되어 있는 경우:
	else if (!CurrentStep.TargetWaypointTag.IsNone())
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

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Interaction verified: Actor=[%s], Type=[%s] for Step [%s]"),
		InteractedActor ? *InteractedActor->GetName() : TEXT("None"),
		*InteractionType,
		*CurrentStep.StepID.ToString());

	ReportTutorialAction(CurrentStep.ActionType, 1, CurrentStep.CustomActionTag);
}

void UTutorialSubsystem::ReportMonsterDefeated(AActor* DefeatedMonster)
{
	if (!bIsActive || bWaitingForDialogue || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ReportMonsterDefeated IGNORED: Inactive, WaitingForDialogue, or Invalid step."));
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];

	// 1. 다중 세부 목표(SubObjectives)가 정의된 경우
	if (CurrentStep.SubObjectives.Num() > 0)
	{
		bool bAnyProgressed = false;
		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			if (CurrentObjectiveCounts.IsValidIndex(i) && CurrentObjectiveCounts[i] >= Obj.RequiredActionCount)
			{
				continue;
			}
			if (Obj.ActionType != ETutorialActionType::DefeatMonster)
			{
				continue;
			}

			bool bMatches = false;
			if (!Obj.CustomActionTag.IsNone())
			{
				bMatches = MatchMonsterIdentifier(DefeatedMonster, Obj.CustomActionTag);
			}
			else if (!Obj.TargetWaypointTag.IsNone())
			{
				bMatches = MatchMonsterIdentifier(DefeatedMonster, Obj.TargetWaypointTag);
			}
			else
			{
				bMatches = true; // 태그 제한 없는 일반 몬스터 처치
			}

			if (bMatches)
			{
				CurrentObjectiveCounts[i] += 1;
				bAnyProgressed = true;
				UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Monster Defeated for SubObjective [%d: %s] (%d / %d)"),
					i, *Obj.ObjectiveDescription.ToString(), CurrentObjectiveCounts[i], Obj.RequiredActionCount);
				break;
			}
		}

		if (!bAnyProgressed)
		{
			return;
		}

		// 전체 목표 진행도 합산 및 완료 판정
		bool bAllCompleted = true;
		int32 TotalCurrent = 0;
		int32 TotalRequired = 0;
		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			int32 Cur = CurrentObjectiveCounts.IsValidIndex(i) ? CurrentObjectiveCounts[i] : 0;
			TotalCurrent += FMath::Min(Cur, Obj.RequiredActionCount);
			TotalRequired += Obj.RequiredActionCount;
			if (Cur < Obj.RequiredActionCount)
			{
				bAllCompleted = false;
			}
		}

		CurrentActionCount = TotalCurrent;
		UpdateNearestWaypoint();
		OnTutorialStepProgress.Broadcast(TotalCurrent, TotalRequired);

		if (bAllCompleted)
		{
			CompleteCurrentStep();
		}
		return;
	}

	// 2. 단일 목표 처리
	if (CurrentStep.ActionType != ETutorialActionType::DefeatMonster)
	{
		return;
	}

	bool bMatches = false;
	if (!CurrentStep.CustomActionTag.IsNone())
	{
		bMatches = MatchMonsterIdentifier(DefeatedMonster, CurrentStep.CustomActionTag);
	}
	else if (!CurrentStep.TargetWaypointTag.IsNone())
	{
		bMatches = MatchMonsterIdentifier(DefeatedMonster, CurrentStep.TargetWaypointTag);
	}
	else
	{
		bMatches = true; // 태그 미지정 시 모든 몬스터 처치 인정
	}

	if (!bMatches)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] Defeated Monster [%s] does not match target tag for Step [%s]"),
			DefeatedMonster ? *DefeatedMonster->GetName() : TEXT("None"), *CurrentStep.StepID.ToString());
		return;
	}

	CurrentActionCount += 1;
	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Monster Defeated Progress: %d / %d for Step [%s]"),
		CurrentActionCount, CurrentStep.RequiredActionCount, *CurrentStep.StepID.ToString());

	OnTutorialStepProgress.Broadcast(CurrentActionCount, CurrentStep.RequiredActionCount);

	if (CurrentActionCount >= CurrentStep.RequiredActionCount)
	{
		CompleteCurrentStep();
	}
	else
	{
		// 아직 사냥할 몬스터가 남았다면, 월드에 남은 동일 태그 몬스터로 웨이포인트 마커 자동 갱신
		FName TargetTagToTrack = !CurrentStep.TargetWaypointTag.IsNone() ? CurrentStep.TargetWaypointTag : CurrentStep.CustomActionTag;
		UpdateNearestAliveTarget(TargetTagToTrack);
	}
}

bool UTutorialSubsystem::MatchMonsterIdentifier(AActor* Monster, FName TargetTag) const
{
	if (!IsValid(Monster) || TargetTag.IsNone())
	{
		return false;
	}

	// 1. 액터의 Tags 배열 전체 순회 검사 (Index 0뿐만 아니라 모든 태그 확인)
	for (const FName& Tag : Monster->Tags)
	{
		if (Tag == TargetTag)
		{
			return true;
		}
	}

	// 2. ItemDropInterface의 GetItemDropTableEnemyID 검사
	if (Monster->Implements<UItemDropInterface>())
	{
		FName DropEnemyID = IItemDropInterface::Execute_GetItemDropTableEnemyID(Monster);
		if (DropEnemyID == TargetTag)
		{
			return true;
		}
	}

	return false;
}

void UTutorialSubsystem::UpdateNearestAliveTarget(FName TargetTag)
{
	if (TargetTag.IsNone())
	{
		return;
	}

	UWorld* World = GetWorld();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!World || !PlayerPawn)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(World, TargetTag, FoundActors);

	AActor* BestActor = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector PlayerLoc = PlayerPawn->GetActorLocation();

	for (AActor* Actor : FoundActors)
	{
		// 파괴 예정이 아니며 화면에 보이는 살아있는 액터 필터링
		if (IsValid(Actor) && !Actor->IsPendingKillPending() && !Actor->IsHidden())
		{
			float DistSq = FVector::DistSquared(PlayerLoc, Actor->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestActor = Actor;
			}
		}
	}

	if (BestActor)
	{
		CachedWaypointActor = BestActor;
		UpdateTargetHighlight();
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Waypoint updated to next alive target: %s"), *BestActor->GetName());
	}
}

void UTutorialSubsystem::AdvanceToNextStep()
{
	CurrentStepIndex++;
	CurrentActionCount = 0;
	CurrentObjectiveCounts.Empty();

	if (CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		if (CurrentSteps[CurrentStepIndex].SubObjectives.Num() > 0)
		{
			CurrentObjectiveCounts.Init(0, CurrentSteps[CurrentStepIndex].SubObjectives.Num());
		}
		SetupCurrentStepVisuals();
		if (!bWaitingForDialogue)
		{
			OnTutorialStepStarted.Broadcast(CurrentSteps[CurrentStepIndex]);
		}
	}
	else
	{
		// 모든 스텝 완료
		ClearTargetHighlight();
		bIsActive = false;
		bWaitingForDialogue = false;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		}

		if (CurrentSequenceName == FName("Farm_Sky_Island") || CurrentSequenceName.ToString().Contains(TEXT("Hub")))
		{
			bCompletedHubTutorial = true;
		}

		if (!CurrentSequenceName.IsNone())
		{
			CompletedSequences.AddUnique(CurrentSequenceName);
		}

		// 펫을 플레이어 추적 상태로 복귀
		ABaseFlyingPet* Pet = GetPlayerPet();
		if (Pet)
		{
			if (Pet->GetPetTalkComponent())
			{
				Pet->GetPetTalkComponent()->OnConversationEnded.RemoveDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
			}
			if (Pet->GetPetGuideComponent())
			{
				Pet->GetPetGuideComponent()->ReturnToFollow();
			}
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

	ClearTargetHighlight();
	bIsActive = false;
	bWaitingForDialogue = false;

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

	if (!CurrentSequenceName.IsNone())
	{
		CompletedSequences.AddUnique(CurrentSequenceName);
	}

	// 펫 복귀 및 대화 종료 처리
	ABaseFlyingPet* Pet = GetPlayerPet();
	if (Pet)
	{
		if (Pet->GetPetTalkComponent())
		{
			Pet->GetPetTalkComponent()->OnConversationEnded.RemoveDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
		}
		if (Pet->GetPetGuideComponent())
		{
			Pet->GetPetGuideComponent()->ReturnToFollow();
		}
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

void UTutorialSubsystem::ReportLevelChanged(FName NewLevelName)
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	PendingLoadedMapName = NewLevelName;
	HandlePostMapTransitionCheck();
}

void UTutorialSubsystem::ForceCompleteCurrentStep()
{
	if (bIsActive && CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Force completing current step [%s] by request."),
			*CurrentSteps[CurrentStepIndex].StepID.ToString());
		CompleteCurrentStep();
	}
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

	// 이전 대화 델리게이트 바인딩 해제
	ABaseFlyingPet* Pet = GetPlayerPet();
	if (Pet && Pet->GetPetTalkComponent())
	{
		Pet->GetPetTalkComponent()->OnConversationEnded.RemoveDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
	}
	bWaitingForDialogue = false;

	// 월드 내 타겟 웨이포인트 액터 검색
	CachedWaypointActor = nullptr;

	// 1) 다중 목표인 경우 가장 가까운 목표 지점을 우선 탐색
	if (Step.SubObjectives.Num() > 0)
	{
		UpdateNearestWaypoint();
	}

	// 2) 아직 웨이포인트가 없거나 단일 목표인 경우 스텝의 TargetWaypointTag로 검색
	if (!CachedWaypointActor.IsValid() && !Step.TargetWaypointTag.IsNone())
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

	// 대화 시작 여부 확인 (대화가 있으면 대화가 끝난 뒤에 퀘스트 부여 및 펫 이동)
	if (!Step.DialogueID.IsNone() && Pet && Pet->GetPetTalkComponent())
	{
		bWaitingForDialogue = true;
		Pet->GetPetTalkComponent()->OnConversationEnded.AddDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
		Pet->StartBigConversation(Step.DialogueID);
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Step [%s] Waiting for dialogue [%s] to finish before presenting quest."),
			*Step.StepID.ToString(), *Step.DialogueID.ToString());
	}
	else if (!Step.DialogueID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] Step [%s] has DialogueID [%s] but Pet or PetTalkComponent is not available!"),
			*Step.StepID.ToString(), *Step.DialogueID.ToString());
	}

	// 동반자 펫 안내 제어
	if (Pet && Pet->GetPetGuideComponent())
	{
		UPetGuideComponent* Guide = Pet->GetPetGuideComponent();

		// 메쉬 가시성 제어 (마을 맵 투명 이정표 연출 등)
		Guide->SetPetMeshHidden(Step.bHidePetMesh);

		// 대화가 있는 스텝이면 대화 중에는 펫이 먼저 날아가지 않고 플레이어 곁에서 대화 나눔
		if (bWaitingForDialogue)
		{
			Guide->ReturnToFollow();
		}
		else if (CachedWaypointActor.IsValid() && Step.bSendPetToWaypoint)
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
	}

	// ReachArea 자동 거리 감지 타이머 설정 (대화 중이 아닐 때만 시작)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		if (!bWaitingForDialogue && Step.ActionType == ETutorialActionType::ReachArea)
		{
			World->GetTimerManager().SetTimer(ReachAreaTimerHandle, this, &UTutorialSubsystem::CheckPlayerReachArea, 0.2f, true);
		}
	}

	// 대상 테두리 하이라이트 갱신 (대화 중이면 대화 종료 후 활성화)
	UpdateTargetHighlight();
}

void UTutorialSubsystem::HandleDialogueFinished()
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		bWaitingForDialogue = false;
		return;
	}

	ABaseFlyingPet* Pet = GetPlayerPet();
	if (Pet && Pet->GetPetTalkComponent())
	{
		Pet->GetPetTalkComponent()->OnConversationEnded.RemoveDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
	}

	bWaitingForDialogue = false;

	const FTutorialStepData& Step = CurrentSteps[CurrentStepIndex];
	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Dialogue finished for Step [%s]. Presenting quest guide and moving pet now!"), *Step.StepID.ToString());

	// [대화 종료 후 펫 출발]: 대화가 끝나고 퀘스트를 받았을 때 목표 위치로 펫 안내 비행 시작
	if (Pet && Pet->GetPetGuideComponent())
	{
		UPetGuideComponent* Guide = Pet->GetPetGuideComponent();
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
			Guide->ReturnToFollow();
		}
	}

	// 대화가 끝났으므로 퀘스트 가이드 UI 시작 브로드캐스트
	OnTutorialStepStarted.Broadcast(Step);

	// ReachArea 액션 스텝인 경우 위치 체크 타이머 가동
	if (Step.ActionType == ETutorialActionType::ReachArea)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
			World->GetTimerManager().SetTimer(ReachAreaTimerHandle, this, &UTutorialSubsystem::CheckPlayerReachArea, 0.2f, true);
		}
	}

	// 최적의 목표 웨이포인트 갱신
	UpdateNearestWaypoint();

	// 대화가 끝났으므로 대상 액터 테두리 하이라이트 활성화
	UpdateTargetHighlight();

	// 대화 종료 시 플레이어가 여전히 UI 조작 중(Mining 등 연금술/봉헌/상자 상호작용 중)인 경우 마우스 커서 및 UI 입력 모드 유지/복원
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(PC->GetPawn());
		if (PlayerChar && PlayerChar->Mining)
		{
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI UIMode;
			UIMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			UIMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(UIMode);
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Player is currently in UI mode (Mining=true). Preserved mouse cursor and GameAndUI mode."));
		}
	}
}

void UTutorialSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!LoadedWorld || !LoadedWorld->IsGameWorld())
	{
		return;
	}

	// 맵이 변경되거나 재진입했을 때, 튜토리얼 활성 여부와 무관하게 이미 열린 게이트(블로킹 볼륨) 상태 자동 복원
	if (OpenedGateTags.Num() > 0)
	{
		FTimerHandle GateRestoreTimerHandle;
		LoadedWorld->GetTimerManager().SetTimer(
			GateRestoreTimerHandle,
			this,
			&UTutorialSubsystem::RestoreWorldGateStates,
			0.2f,
			false
		);
	}

	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	FString CleanMapName = LoadedWorld->GetMapName();
	LoadedWorld->RemovePIEPrefix(CleanMapName);
	CleanMapName = FPaths::GetBaseFilename(CleanMapName);
	PendingLoadedMapName = FName(*CleanMapName);

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] PostLoadMapWithWorld detected: %s (Clean: %s)"),
		*LoadedWorld->GetMapName(), *CleanMapName);

	// 새 월드의 액터(플레이어 캐릭터, 동반자 펫 등)가 안정적으로 BeginPlay된 뒤 판정할 수 있도록 0.3초 대기
	LoadedWorld->GetTimerManager().ClearTimer(LevelTransitionTimerHandle);
	LoadedWorld->GetTimerManager().SetTimer(
		LevelTransitionTimerHandle,
		this,
		&UTutorialSubsystem::HandlePostMapTransitionCheck,
		0.3f,
		false
	);
}

void UTutorialSubsystem::HandlePostMapTransitionCheck()
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];
	const FString CurrentMapStr = PendingLoadedMapName.ToString();

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Checking map transition condition for Step [%s]. Current Map: %s"),
		*CurrentStep.StepID.ToString(), *CurrentMapStr);

	bool bMatches = false;

	// 1. 다중 세부 목표(SubObjectives) 검사
	if (CurrentStep.SubObjectives.Num() > 0)
	{
		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			if (CurrentObjectiveCounts.IsValidIndex(i) && CurrentObjectiveCounts[i] >= Obj.RequiredActionCount)
			{
				continue;
			}

			// EnterPortal 액션이거나 TargetLevelName이 지정된 경우
			if (Obj.ActionType == ETutorialActionType::EnterPortal || !Obj.TargetLevelName.IsNone())
			{
				if (!Obj.TargetLevelName.IsNone())
				{
					if (Obj.TargetLevelName.ToString().Equals(CurrentMapStr, ESearchCase::IgnoreCase))
					{
						ReportTutorialAction(Obj.ActionType, 1, PendingLoadedMapName);
						return;
					}
				}
				else
				{
					// 목표 맵 제한 없는 일반 포탈/맵 이동 스텝
					ReportTutorialAction(Obj.ActionType, 1, PendingLoadedMapName);
					return;
				}
			}
		}

		// 일치하지 않은 경우 새 월드에 맞춰 펫 및 웨이포인트 비주얼 재설정
		SetupCurrentStepVisuals();
		return;
	}

	// 2. 단일 목표 검사
	if (CurrentStep.ActionType == ETutorialActionType::EnterPortal)
	{
		// TargetLevelName이 지정되어 있다면 일치해야 함
		if (!CurrentStep.TargetLevelName.IsNone())
		{
			if (CurrentStep.TargetLevelName.ToString().Equals(CurrentMapStr, ESearchCase::IgnoreCase))
			{
				bMatches = true;
			}
		}
		// CustomActionTag가 지정되어 있다면 일치해야 함
		else if (!CurrentStep.CustomActionTag.IsNone())
		{
			if (CurrentStep.CustomActionTag.ToString().Equals(CurrentMapStr, ESearchCase::IgnoreCase))
			{
				bMatches = true;
			}
		}
		else
		{
			// 특정 맵 조건 없는 일반 포탈/맵 이동
			bMatches = true;
		}
	}
	// ActionType이 EnterPortal이 아니더라도 TargetLevelName이 설정되어 있고 일치하는 경우
	else if (!CurrentStep.TargetLevelName.IsNone() && CurrentStep.TargetLevelName.ToString().Equals(CurrentMapStr, ESearchCase::IgnoreCase))
	{
		bMatches = true;
	}

	if (bMatches)
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Map Transition condition matched for Step [%s]! Clearing step."),
			*CurrentStep.StepID.ToString());
		ReportTutorialAction(CurrentStep.ActionType, CurrentStep.RequiredActionCount, PendingLoadedMapName);
	}
	else
	{
		// 조건이 일치하지 않는 다른 스텝이라면 새 월드의 액터들에 맞게 비주얼 재바인딩
		SetupCurrentStepVisuals();
	}
}

void UTutorialSubsystem::CheckPlayerReachArea()
{
	if (!bIsActive || bWaitingForDialogue || !CurrentSteps.IsValidIndex(CurrentStepIndex))
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
	// 포탈 등 대형 구조물이나 지형 단차를 고려하여 Z 높이 허용치도 EffectiveRadius 이상으로 유연하게 적용
	const float EffectiveZThreshold = FMath::Max(ReachAreaZThreshold, EffectiveRadius);
	if (Dist2D <= EffectiveRadius && DiffZ <= EffectiveZThreshold)
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		ReportTutorialAction(ETutorialActionType::ReachArea, 1, Step.CustomActionTag);
	}
}

void UTutorialSubsystem::OpenGateActor(FName GateTag)
{
	if (GateTag.IsNone())
	{
		return;
	}

	OpenedGateTags.AddUnique(GateTag);

	UWorld* World = GetWorld();
	if (!World)
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

void UTutorialSubsystem::RestoreWorldGateStates()
{
	UWorld* World = GetWorld();
	if (!World || OpenedGateTags.Num() == 0)
	{
		return;
	}

	int32 DisabledCount = 0;
	for (const FName& GateTag : OpenedGateTags)
	{
		if (GateTag.IsNone())
		{
			continue;
		}

		TArray<AActor*> GateActors;
		UGameplayStatics::GetAllActorsWithTag(World, GateTag, GateActors);

		for (AActor* Actor : GateActors)
		{
			if (IsValid(Actor))
			{
				Actor->SetActorHiddenInGame(true);
				Actor->SetActorEnableCollision(false);
				DisabledCount++;
			}
		}
	}

	if (DisabledCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] RestoreWorldGateStates: Restored %d gate actors across %d tags to open state."),
			DisabledCount, OpenedGateTags.Num());
	}
}

bool UTutorialSubsystem::IsSequenceCompleted(FName SequenceName) const
{
	if (SequenceName.IsNone())
	{
		return false;
	}

	if (SequenceName == FName("Farm_Sky_Island") || SequenceName.ToString().Contains(TEXT("Hub")))
	{
		if (bCompletedHubTutorial)
		{
			return true;
		}
	}

	return CompletedSequences.Contains(SequenceName);
}

bool UTutorialSubsystem::IsGateOpened(FName GateTag) const
{
	return !GateTag.IsNone() && OpenedGateTags.Contains(GateTag);
}

void UTutorialSubsystem::SetCompletedSequences(const TArray<FName>& InSequences)
{
	CompletedSequences = InSequences;
}

void UTutorialSubsystem::SetOpenedGateTags(const TArray<FName>& InGateTags)
{
	OpenedGateTags = InGateTags;
	RestoreWorldGateStates();
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

FText UTutorialSubsystem::GetDetailedProgressText() const
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return FText::GetEmpty();
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];

	// 다중 세부 목표가 있는 경우: 각 목표별로 따로따로 진행도를 포맷팅
	if (CurrentStep.SubObjectives.Num() > 0)
	{
		TArray<FString> ObjectiveStrings;

		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			int32 Cur = CurrentObjectiveCounts.IsValidIndex(i) ? CurrentObjectiveCounts[i] : 0;
			int32 Req = Obj.RequiredActionCount;

			FString ObjStr;
			FString Desc = Obj.ObjectiveDescription.ToString();

			if (Req > 1)
			{
				if (!Desc.IsEmpty())
				{
					ObjStr = FString::Printf(TEXT("%s [ %d / %d ]"), *Desc, Cur, Req);
				}
				else
				{
					ObjStr = FString::Printf(TEXT("[ %d / %d ]"), Cur, Req);
				}
			}
			else
			{
				ObjStr = Desc;
			}

			ObjectiveStrings.Add(ObjStr);
		}

		return FText::FromString(FString::Join(ObjectiveStrings, TEXT("   |   ")));
	}

	// 단일 목표인 경우
	if (CurrentStep.RequiredActionCount > 1)
	{
		return FText::Format(NSLOCTEXT("Tutorial", "ProgressFormat", "[ {0} / {1} ]"),
			FText::AsNumber(CurrentActionCount),
			FText::AsNumber(CurrentStep.RequiredActionCount));
	}

	return FText::GetEmpty();
}

void UTutorialSubsystem::UpdateNearestWaypoint()
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FTutorialStepData& CurrentStep = CurrentSteps[CurrentStepIndex];
	if (CurrentStep.SubObjectives.Num() == 0)
	{
		return;
	}

	APawn* PlayerPawn = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PlayerPawn = PC->GetPawn();
		}
	}

	if (!PlayerPawn)
	{
		return;
	}

	FVector PlayerLoc = PlayerPawn->GetActorLocation();
	AActor* BestActor = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
	{
		const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
		int32 Cur = CurrentObjectiveCounts.IsValidIndex(i) ? CurrentObjectiveCounts[i] : 0;

		// 이미 달성한 목표는 안내 대상에서 제외
		if (Cur >= Obj.RequiredActionCount)
		{
			continue;
		}

		FName TagToSearch = !Obj.TargetWaypointTag.IsNone() ? Obj.TargetWaypointTag : CurrentStep.TargetWaypointTag;
		if (TagToSearch.IsNone())
		{
			continue;
		}

		AActor* FoundActor = FindActorWithTag(TagToSearch);
		if (FoundActor)
		{
			float DistSq = FVector::DistSquared(PlayerLoc, FoundActor->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestActor = FoundActor;
			}
		}
	}

	if (BestActor && CachedWaypointActor.Get() != BestActor)
	{
		CachedWaypointActor = BestActor;
		ABaseFlyingPet* Pet = GetPlayerPet();
		if (Pet && Pet->GetPetGuideComponent() && CurrentStep.bSendPetToWaypoint)
		{
			Pet->GetPetGuideComponent()->MoveToActor(BestActor);
		}
		UpdateTargetHighlight();
	}
}

void UTutorialSubsystem::UpdateTargetHighlight()
{
	// 대화 대기 중이거나 시퀀스가 비활성 상태이면 하이라이트 해제
	if (!bIsActive || bWaitingForDialogue || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		ClearTargetHighlight();
		return;
	}

	const FTutorialStepData& Step = CurrentSteps[CurrentStepIndex];
	AActor* TargetActor = CachedWaypointActor.Get();

	// 데이터테이블에서 bHighlightTarget 옵션이 true이고 유효한 타겟 액터가 있을 때
	if (Step.bHighlightTarget && IsValid(TargetActor))
	{
		// 이전에 하이라이트된 액터와 타겟이 다르면 이전 액터 해제
		if (CurrentHighlightedActor.IsValid() && CurrentHighlightedActor.Get() != TargetActor)
		{
			SetActorHighlight(CurrentHighlightedActor.Get(), false);
		}

		SetActorHighlight(TargetActor, true);
		CurrentHighlightedActor = TargetActor;
	}
	else
	{
		ClearTargetHighlight();
	}
}

void UTutorialSubsystem::ClearTargetHighlight()
{
	if (CurrentHighlightedActor.IsValid())
	{
		SetActorHighlight(CurrentHighlightedActor.Get(), false);
		CurrentHighlightedActor = nullptr;
	}
}

void UTutorialSubsystem::SetActorHighlight(AActor* TargetActor, bool bActive)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	// NPC 태그가 있는 경우 하이라이트 제외 (AGSDInteractionComponent와 동일 정책)
	if (TargetActor->ActorHasTag(TEXT("NPC")))
	{
		return;
	}

	TArray<UPrimitiveComponent*> Comps;
	TargetActor->GetComponents(Comps);
	for (UPrimitiveComponent* Comp : Comps)
	{
		if (Comp)
		{
			Comp->SetRenderCustomDepth(bActive);
		}
	}
}
