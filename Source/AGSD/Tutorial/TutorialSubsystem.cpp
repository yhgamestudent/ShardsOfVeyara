#include "TutorialSubsystem.h"
#include "BaseFlyingPet.h"
#include "Component/PetGuideComponent.h"
#include "Component/PetTalkComponent.h"
#include "Character/AGSDCharacter.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Inventory/UI/AGSDPlayerHUD.h"
#include "GameplayLogSubsystem.h"
#include "SOVGameInstance.h"
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
	CurrentTutorialTable = nullptr;

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
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
	}
	DialogueRetryCount = 0;
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
	CurrentTutorialTable = TutorialTable;
	bIsActive = true;
	bWaitingForDialogue = false;

	// 이전 레벨 전환 및 대화 재시도 타이머가 남아있다면 즉시 취소하여 새 퀘스트가 오작동하는 것을 방지
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LevelTransitionTimerHandle);
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
		FString CleanMap = World->GetMapName();
		World->RemovePIEPrefix(CleanMap);
		CleanMap = FPaths::GetBaseFilename(CleanMap);
		CurrentSequenceOriginMapName = FName(*CleanMap);
	}
	DialogueRetryCount = 0;

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Started tutorial sequence: %s in Map [%s] (Steps: %d)"),
		*SequenceName.ToString(), *CurrentSequenceOriginMapName.ToString(), CurrentSteps.Num());

	SetupCurrentStepVisuals();

	// 대화가 진행 중이지 않은 경우에만 즉시 퀘스트 UI 브로드캐스트
	if (!bWaitingForDialogue)
	{
		OnTutorialStepStarted.Broadcast(CurrentSteps[0]);
	}
}

void UTutorialSubsystem::ResumeTutorialSequence(UDataTable* TutorialTable, FName SequenceName, int32 StepIndex, const TArray<int32>& InObjectiveCounts)
{
	if (!TutorialTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ResumeTutorialSequence: TutorialTable is null!"));
		return;
	}

	if (IsSequenceCompleted(SequenceName))
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] ResumeTutorialSequence IGNORED: Sequence [%s] is ALREADY COMPLETED."), *SequenceName.ToString());
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
		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ResumeTutorialSequence: No steps found in DataTable for sequence %s"), *SequenceName.ToString());
		return;
	}

	// 저장된 스텝 인덱스 범위 클램핑
	if (!CurrentSteps.IsValidIndex(StepIndex))
	{
		StepIndex = FMath::Clamp(StepIndex, 0, CurrentSteps.Num() - 1);
	}

	ClearTargetHighlight();
	CurrentStepIndex = StepIndex;
	CurrentSequenceName = SequenceName;
	CurrentTutorialTable = TutorialTable;
	bIsActive = true;
	bWaitingForDialogue = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LevelTransitionTimerHandle);
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
		FString CleanMap = World->GetMapName();
		World->RemovePIEPrefix(CleanMap);
		CleanMap = FPaths::GetBaseFilename(CleanMap);
		CurrentSequenceOriginMapName = FName(*CleanMap);
	}
	DialogueRetryCount = 0;

	// 세부 목표 진행 카운트 복원
	const FTutorialStepData& ResumedStep = CurrentSteps[CurrentStepIndex];
	CurrentObjectiveCounts.Empty();
	if (ResumedStep.SubObjectives.Num() > 0)
	{
		CurrentObjectiveCounts = InObjectiveCounts;
		if (CurrentObjectiveCounts.Num() < ResumedStep.SubObjectives.Num())
		{
			CurrentObjectiveCounts.SetNumZeroed(ResumedStep.SubObjectives.Num());
		}

		int32 TotalCurrent = 0;
		int32 TotalRequired = 0;
		for (int32 i = 0; i < ResumedStep.SubObjectives.Num(); ++i)
		{
			int32 Cur = CurrentObjectiveCounts[i];
			TotalCurrent += FMath::Min(Cur, ResumedStep.SubObjectives[i].RequiredActionCount);
			TotalRequired += ResumedStep.SubObjectives[i].RequiredActionCount;
		}
		CurrentActionCount = TotalCurrent;
	}
	else
	{
		CurrentActionCount = (InObjectiveCounts.Num() > 0) ? InObjectiveCounts[0] : 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] RESUMED tutorial sequence: %s at Step [%d: %s]"),
		*SequenceName.ToString(), CurrentStepIndex, *ResumedStep.StepID.ToString());

	SetupCurrentStepVisuals();

	// 퀘스트 UI 알림 브로드캐스트
	OnTutorialStepStarted.Broadcast(ResumedStep);
	if (ResumedStep.SubObjectives.Num() > 0)
	{
		int32 TotalRequired = 0;
		for (const FTutorialObjective& Obj : ResumedStep.SubObjectives)
		{
			TotalRequired += Obj.RequiredActionCount;
		}
		OnTutorialStepProgress.Broadcast(CurrentActionCount, TotalRequired);
	}
	else if (ResumedStep.RequiredActionCount > 1)
	{
		OnTutorialStepProgress.Broadcast(CurrentActionCount, ResumedStep.RequiredActionCount);
	}
}

bool UTutorialSubsystem::GetActiveTutorialState(FName& OutSequenceName, TSoftObjectPtr<UDataTable>& OutTutorialTable, int32& OutStepIndex, TArray<int32>& OutObjectiveCounts) const
{
	if (!bIsActive || CurrentSequenceName.IsNone())
	{
		OutSequenceName = NAME_None;
		OutTutorialTable = nullptr;
		OutStepIndex = 0;
		OutObjectiveCounts.Empty();
		return false;
	}

	OutSequenceName = CurrentSequenceName;
	OutTutorialTable = CurrentTutorialTable;
	OutStepIndex = CurrentStepIndex;
	OutObjectiveCounts = CurrentObjectiveCounts;
	return true;
}

void UTutorialSubsystem::AutoSaveTutorialProgress()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USOVGameInstance* SOVGI = Cast<USOVGameInstance>(GI))
		{
			SOVGI->SaveGame();
		}
	}
}

bool UTutorialSubsystem::CheckSubObjectivesCompleted(const FTutorialStepData& Step, const TArray<int32>& Counts) const
{
	if (Step.SubObjectives.Num() == 0)
	{
		return true;
	}

	bool bAllCompleted = true;
	bool bAnyCompleted = false;
	int32 TotalActionCount = 0;

	for (int32 i = 0; i < Step.SubObjectives.Num(); ++i)
	{
		const FTutorialObjective& Obj = Step.SubObjectives[i];
		int32 Cur = Counts.IsValidIndex(i) ? Counts[i] : 0;
		TotalActionCount += Cur;

		if (Cur >= Obj.RequiredActionCount)
		{
			bAnyCompleted = true;
		}
		else
		{
			bAllCompleted = false;
		}
	}

	// 1) OR 조건 (bOrSubObjectives == true):
	// 세부 목표 중 어느 하나라도 목표치를 달성했거나, 최소 목표치 이상의 행동 총합을 달성한 경우 완료
	if (Step.bOrSubObjectives)
	{
		int32 MinTargetCount = Step.SubObjectives.Num() > 0 ? Step.SubObjectives[0].RequiredActionCount : 1;
		bool bFinished = bAnyCompleted || (TotalActionCount >= MinTargetCount);
		if (bFinished)
		{
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] OR-Objective Step [%s] completed! (bAnyCompleted=%s, TotalCount=%d)"),
				*Step.StepID.ToString(), bAnyCompleted ? TEXT("True") : TEXT("False"), TotalActionCount);
		}
		return bFinished;
	}

	// 2) 기본 AND 조건: 모든 세부 목표를 달성해야 완료
	return bAllCompleted;
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
		int32 TotalCurrent = 0;
		int32 TotalRequired = 0;

		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			int32 Cur = CurrentObjectiveCounts.IsValidIndex(i) ? CurrentObjectiveCounts[i] : 0;
			TotalCurrent += FMath::Min(Cur, Obj.RequiredActionCount);
			TotalRequired += Obj.RequiredActionCount;
		}

		bool bCompleted = CheckSubObjectivesCompleted(CurrentStep, CurrentObjectiveCounts);

		CurrentActionCount = TotalCurrent;

		// 가장 가까운 다음 미완료 목표 지점으로 안내 갱신
		UpdateNearestWaypoint();

		OnTutorialStepProgress.Broadcast(TotalCurrent, TotalRequired);

		if (bCompleted)
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
				// 씨앗 심기(Planting / CultivationPlot) 상호작용 지원
				else if (Obj.CustomActionTag == FName("CultivationPlot") || Obj.ActionType == ETutorialActionType::PlantSeed)
				{
					if (InteractionType.Equals(TEXT("Planting"), ESearchCase::IgnoreCase) ||
						InteractionType.Equals(TEXT("CultivationPlot"), ESearchCase::IgnoreCase) ||
						(IsValid(InteractedActor) && InteractedActor->GetName().Contains(TEXT("CultivationPlot"))))
					{
						bMatches = true;
					}
				}
				// 작물 수확(CropHarvest) 상호작용 지원
				else if (Obj.CustomActionTag == FName("CropHarvest") || Obj.ActionType == ETutorialActionType::WeedHarvest)
				{
					if (InteractionType.Equals(TEXT("CropHarvest"), ESearchCase::IgnoreCase) ||
						(IsValid(InteractedActor) && InteractedActor->GetName().Contains(TEXT("Crop"))))
					{
						bMatches = true;
					}
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
		int32 TotalCurrent = 0;
		int32 TotalRequired = 0;
		for (int32 i = 0; i < CurrentStep.SubObjectives.Num(); ++i)
		{
			const FTutorialObjective& Obj = CurrentStep.SubObjectives[i];
			int32 Cur = CurrentObjectiveCounts.IsValidIndex(i) ? CurrentObjectiveCounts[i] : 0;
			TotalCurrent += FMath::Min(Cur, Obj.RequiredActionCount);
			TotalRequired += Obj.RequiredActionCount;
		}

		bool bCompleted = CheckSubObjectivesCompleted(CurrentStep, CurrentObjectiveCounts);

		CurrentActionCount = TotalCurrent;
		UpdateNearestWaypoint();
		OnTutorialStepProgress.Broadcast(TotalCurrent, TotalRequired);

		if (bCompleted)
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

		FName FinishedSequence = CurrentSequenceName;
		if (!CurrentSequenceName.IsNone())
		{
			CompletedSequences.AddUnique(CurrentSequenceName);
		}

		CurrentSequenceName = NAME_None;
		CurrentTutorialTable = nullptr;

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

		// 시퀀스 완료 상태 영구 저장
		AutoSaveTutorialProgress();

		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Sequence [%s] Fully Completed!"), *FinishedSequence.ToString());
		OnTutorialSequenceCompleted.Broadcast(FinishedSequence);
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
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
	}
	DialogueRetryCount = 0;

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

	FName SkippedSequence = CurrentSequenceName;
	if (!CurrentSequenceName.IsNone())
	{
		CompletedSequences.AddUnique(CurrentSequenceName);
	}

	CurrentSequenceName = NAME_None;
	CurrentTutorialTable = nullptr;
	CurrentSequenceOriginMapName = NAME_None;

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

	AutoSaveTutorialProgress();

	OnTutorialSequenceCompleted.Broadcast(SkippedSequence);
}

void UTutorialSubsystem::CancelCurrentSequence()
{
	if (!bIsActive)
	{
		return;
	}

	FName CancelledSequence = CurrentSequenceName;
	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Cancelling and clearing sequence: %s"), *CancelledSequence.ToString());

	ClearTargetHighlight();
	bIsActive = false;
	bWaitingForDialogue = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		World->GetTimerManager().ClearTimer(LevelTransitionTimerHandle);
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
	}
	DialogueRetryCount = 0;

	CurrentSteps.Empty();
	CurrentStepIndex = 0;
	CurrentActionCount = 0;
	CurrentObjectiveCounts.Empty();
	CurrentSequenceName = NAME_None;
	CurrentTutorialTable = nullptr;
	CachedWaypointActor = nullptr;
	CurrentSequenceOriginMapName = NAME_None;

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

	AutoSaveTutorialProgress();
	OnTutorialSequenceCompleted.Broadcast(CancelledSequence);
}

void UTutorialSubsystem::ResetAllQuests()
{
	UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] ResetAllQuests called! Clearing all completed sequence history and resetting active quests."));

	ClearTargetHighlight();
	bIsActive = false;
	bWaitingForDialogue = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
		World->GetTimerManager().ClearTimer(LevelTransitionTimerHandle);
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
	}
	DialogueRetryCount = 0;

	CurrentSteps.Empty();
	CurrentStepIndex = 0;
	CurrentActionCount = 0;
	CurrentObjectiveCounts.Empty();
	CurrentSequenceName = NAME_None;
	CurrentTutorialTable = nullptr;
	CachedWaypointActor = nullptr;
	CurrentSequenceOriginMapName = NAME_None;

	CompletedSequences.Empty();
	bCompletedHubTutorial = false;
	OpenedGateTags.Empty();

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

	AutoSaveTutorialProgress();
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

	// 기존 대화 재시도 타이머 취소
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
	}
	DialogueRetryCount = 0;

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
	if (!Step.DialogueID.IsNone())
	{
		bWaitingForDialogue = true;

		if (Pet && Pet->GetPetTalkComponent())
		{
			Pet->GetPetTalkComponent()->OnConversationEnded.RemoveDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
			Pet->GetPetTalkComponent()->OnConversationEnded.AddDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
			Pet->StartBigConversation(Step.DialogueID);
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Step [%s] Waiting for dialogue [%s] to finish before presenting quest."),
				*Step.StepID.ToString(), *Step.DialogueID.ToString());
		}
		else
		{
			// 맵 이동 직후 펫이 아직 스폰되지 않았거나 준비 중인 경우 재시도 타이머 가동 (0.1초마다 검사)
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Step [%s] has DialogueID [%s] but Pet/PetTalkComponent is not ready yet. Starting retry timer..."),
				*Step.StepID.ToString(), *Step.DialogueID.ToString());

			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
				World->GetTimerManager().SetTimer(
					DialogueRetryTimerHandle,
					this,
					&UTutorialSubsystem::TryStartPendingDialogue,
					0.1f,
					true
				);
			}
		}
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
	}
	DialogueRetryCount = 0;

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

	// 대화 종료 시 마우스 커서 숨김 및 GameOnly 입력 모드 복원 (단, 다른 UI가 열려있는 경우에만 UI 모드 유지)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(PC->GetPawn());
		bool bStillInUI = false;
		if (PlayerChar)
		{
			if (PlayerChar->PlayerHUDRef && PlayerChar->PlayerHUDRef->IsInventoryOpen())
			{
				bStillInUI = true;
			}
			else if (PlayerChar->ActiveCloseableUI.IsValid())
			{
				bStillInUI = true;
			}
			else if (PlayerChar->OpenedChest != nullptr)
			{
				bStillInUI = true;
			}
		}

		if (bStillInUI)
		{
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI UIMode;
			UIMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			UIMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(UIMode);
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Player is currently in another UI. Preserved mouse cursor and GameAndUI mode."));
		}
		else
		{
			PC->SetShowMouseCursor(false);
			FInputModeGameOnly GameMode;
			GameMode.SetConsumeCaptureMouseDown(false);
			PC->SetInputMode(GameMode);

			PC->ResetIgnoreMoveInput();
			PC->ResetIgnoreLookInput();
			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);

			FSlateApplication::Get().SetAllUserFocusToGameViewport();
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Dialogue finished: Restored GameOnly input mode and hid mouse cursor."));
		}
	}
}

void UTutorialSubsystem::TryStartPendingDialogue()
{
	if (!bIsActive || !CurrentSteps.IsValidIndex(CurrentStepIndex))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
		}
		DialogueRetryCount = 0;
		return;
	}

	const FTutorialStepData& Step = CurrentSteps[CurrentStepIndex];
	DialogueRetryCount++;

	ABaseFlyingPet* Pet = GetPlayerPet();
	if (Pet && Pet->GetPetTalkComponent())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
		}

		bWaitingForDialogue = true;
		Pet->GetPetTalkComponent()->OnConversationEnded.RemoveDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
		Pet->GetPetTalkComponent()->OnConversationEnded.AddDynamic(this, &UTutorialSubsystem::HandleDialogueFinished);
		Pet->StartBigConversation(Step.DialogueID);

		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Pet ready after %d retries (%.1fs). Started dialogue [%s] for Step [%s]."),
			DialogueRetryCount, DialogueRetryCount * 0.1f, *Step.DialogueID.ToString(), *Step.StepID.ToString());

		if (Pet->GetPetGuideComponent())
		{
			Pet->GetPetGuideComponent()->SetPetMeshHidden(Step.bHidePetMesh);
			Pet->GetPetGuideComponent()->ReturnToFollow();
		}
		return;
	}

	// 최대 30회 (3.0초) 동안 펫을 찾지 못한 경우 폴백 처리 (게임 진행 멈춤 방지)
	if (DialogueRetryCount >= 30)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DialogueRetryTimerHandle);
		}

		UE_LOG(LogTemp, Warning, TEXT("[TutorialSubsystem] Pet was NOT found after 30 retries (3.0s). Fallback: skipping dialogue [%s] and presenting quest."),
			*Step.DialogueID.ToString());

		bWaitingForDialogue = false;

		// 퀘스트 UI 시작 브로드캐스트
		OnTutorialStepStarted.Broadcast(Step);

		if (Step.ActionType == ETutorialActionType::ReachArea)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(ReachAreaTimerHandle);
				World->GetTimerManager().SetTimer(ReachAreaTimerHandle, this, &UTutorialSubsystem::CheckPlayerReachArea, 0.2f, true);
			}
		}

		UpdateNearestWaypoint();
		UpdateTargetHighlight();
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

	// 만약 이미 진행 중인 퀘스트의 원래 맵과 새로 로드된 맵이 완전히 같다면 타이머를 돌릴 필요조차 없음!
	if (!CurrentSequenceOriginMapName.IsNone() && CurrentSequenceOriginMapName.ToString().Equals(CleanMapName, ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] PostLoadMapWithWorld: Same map [%s] as sequence origin [%s]. Ignoring transition check."),
			*CleanMapName, *CurrentSequenceOriginMapName.ToString());
		return;
	}

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

	// 1) 현재 퀘스트가 시작된 맵과 지금 로드된 맵이 같다면, 맵 이동으로 인한 캔슬 대상이 아니므로 안전하게 무시
	if (!CurrentSequenceOriginMapName.IsNone() && CurrentSequenceOriginMapName.ToString().Equals(CurrentMapStr, ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Current map [%s] matches sequence origin map [%s]. No map transition cancellation needed."),
			*CurrentMapStr, *CurrentSequenceOriginMapName.ToString());
		return;
	}

	// 2) 현재 월드와 원본 맵 이름이 같은 경우도 추가 방어
	if (UWorld* CurrentWorld = GetWorld())
	{
		FString ActiveMap = CurrentWorld->GetMapName();
		CurrentWorld->RemovePIEPrefix(ActiveMap);
		ActiveMap = FPaths::GetBaseFilename(ActiveMap);
		if (!CurrentSequenceOriginMapName.IsNone() && CurrentSequenceOriginMapName.ToString().Equals(ActiveMap, ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Active world map [%s] matches sequence origin map [%s]. Skipping cancellation."),
				*ActiveMap, *CurrentSequenceOriginMapName.ToString());
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Checking map transition condition for Step [%s]. Current Map: %s (Origin: %s)"),
		*CurrentStep.StepID.ToString(), *CurrentMapStr, *CurrentSequenceOriginMapName.ToString());

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

		// 1) 다중 세부 목표가 일치하지 않는 경우: 맵이 변경되었으나 새 맵의 목표와 일치하지 않으므로 이전 맵 퀘스트 자동 취소 및 삭제
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Map changed to [%s], but sub-objectives for sequence [%s] do not match. Cancelling sequence to clear UI."),
			*CurrentMapStr, *CurrentSequenceName.ToString());
		CancelCurrentSequence();
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
		// 2) 단일 목표가 일치하지 않는 경우: 맵이 변경되었으나 새 맵의 목표와 일치하지 않으므로 이전 맵 퀘스트 자동 취소 및 삭제
		UE_LOG(LogTemp, Log, TEXT("[TutorialSubsystem] Map changed to [%s], but sequence [%s] (Step [%s]) does not match. Cancelling sequence to clear UI."),
			*CurrentMapStr, *CurrentSequenceName.ToString(), *CurrentStep.StepID.ToString());
		CancelCurrentSequence();
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
