// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGSDPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "AGSD.h"
#include "Animation/WidgetAnimation.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "Struct_ItemData.h"
#include "Character/AGSDCharacter.h"
#include "SOVGameInstance.h"
#include "BaseFlyingPet.h"
#include "Tutorial/TutorialSubsystem.h"

AAGSDPlayerController::AAGSDPlayerController()
{
	DefaultMouseCursor = EMouseCursor::Default;
	CurrentMouseCursor = EMouseCursor::Default;
}

void AAGSDPlayerController::ShowInteractionWidget(const FText& NewText)
{
	if (!WBP_InteractionWidget) return;
	if (!InteractionWidget) InteractionWidget = CreateWidget<UInteractionWidget>(this, WBP_InteractionWidget);
	InteractionWidget->SetInteractionText(NewText);
	InteractionWidget->SetTargetOpacity(1.0f);
	if (!InteractionWidget->IsInViewport())
	{
		InteractionWidget->AddToViewport();
	}
}

void AAGSDPlayerController::ShowClockWidget()
{
	if (!WBP_ClockWidget)
	{
		UE_LOG(LogTemp, Display, TEXT("WBP_ClockWidget Empty"));
		return;
	}
	if (!ClockWidget)
	{
		ClockWidget = CreateWidget<UUserWidget>(this, WBP_ClockWidget);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("ClockWidget Empty"));
	}
	if (!ClockWidget->IsInViewport())
	{
		ClockWidget->AddToViewport();
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("ClockWidget is In Viewport"));
	}
}

void AAGSDPlayerController::HideInteractionWidget()
{
	if (InteractionWidget && InteractionWidget->IsInViewport())
	{
		InteractionWidget->SetTargetOpacity(0.0f);
	}
}

void AAGSDPlayerController::CutsceneHideClockWidget()
{
	if (ClockWidget)
	{
		ClockWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AAGSDPlayerController::CutsceneShowClockWidget()
{
	if (ClockWidget)
	{
		ClockWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AAGSDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShowClockWidget();
	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogAGSD, Error, TEXT("Could not spawn mobile controls widget."));

		}
	}
}

void AAGSDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

// ═══════════════════════════════════════════════════
// ── 치트 콘솔 커맨드 (Cheat Console Commands) ──
// ═══════════════════════════════════════════════════

UAGSDInventoryComponent* AAGSDPlayerController::GetPlayerInventoryComponent() const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] Player pawn not found."));
		return nullptr;
	}

	UAGSDInventoryComponent* InvComp = ControlledPawn->FindComponentByClass<UAGSDInventoryComponent>();
	if (!InvComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] UAGSDInventoryComponent not found on player pawn."));
	}
	return InvComp;
}

void AAGSDPlayerController::GiveItem(const FString& ItemID, int32 Amount)
{
	if (Amount <= 0)
	{
		Amount = 1;
	}

	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	int32 RemainingQty = 0;
	FStruct_ItemData OutItemData;
	bool bSuccess = InvComp->AddItemByID(ItemID, Amount, RemainingQty, OutItemData);

	if (bSuccess)
	{
		int32 AddedQty = Amount - RemainingQty;
		FString Msg = FString::Printf(TEXT("[Cheat] Added item '%s' (%s) x%d to Inventory!"), *ItemID, *OutItemData.ItemName.ToString(), AddedQty);
		if (RemainingQty > 0)
		{
			Msg += FString::Printf(TEXT(" (Inventory Full! %d not added)"), RemainingQty);
		}

		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg);
	}
	else
	{
		FString Msg = FString::Printf(TEXT("[Cheat] Failed to give item '%s'. Check if ItemID exists in DataTable."), *ItemID);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, Msg);
	}
}

void AAGSDPlayerController::GiveAllItems(int32 Amount)
{
	if (Amount <= 0)
	{
		Amount = 1;
	}

	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	UDataTable* DataTable = InvComp->GetItemDataTable();
	if (!DataTable)
	{
		FString Msg = TEXT("[Cheat] Failed: ItemDataTable is null on InventoryComponent.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, Msg);
		return;
	}

	TArray<FName> RowNames = DataTable->GetRowNames();
	int32 SuccessCount = 0;

	for (const FName& RowName : RowNames)
	{
		int32 RemainingQty = 0;
		FStruct_ItemData OutItemData;
		if (InvComp->AddItemByID(RowName.ToString(), Amount, RemainingQty, OutItemData))
		{
			SuccessCount++;
		}
	}

	FString Msg = FString::Printf(TEXT("[Cheat] GiveAllItems finished: %d/%d item types added."), SuccessCount, RowNames.Num());
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, Msg);
}

void AAGSDPlayerController::ClearInventory()
{
	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	InvComp->ClearAllSlots();
	FString Msg = TEXT("[Cheat] Cleared all inventory slots.");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, Msg);
}

void AAGSDPlayerController::ListItems()
{
	UAGSDInventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Inventory component not found!"));
		return;
	}

	UDataTable* DataTable = InvComp->GetItemDataTable();
	if (!DataTable)
	{
		FString Msg = TEXT("[Cheat] Failed: ItemDataTable is null on InventoryComponent.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, Msg);
		return;
	}

	TArray<FName> RowNames = DataTable->GetRowNames();
	UE_LOG(LogTemp, Log, TEXT("======== Available Items List (%d) ========"), RowNames.Num());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan, FString::Printf(TEXT("=== Available Items (%d) - Check Output Log ==="), RowNames.Num()));

	for (const FName& RowName : RowNames)
	{
		FStruct_ItemData* RowData = DataTable->FindRow<FStruct_ItemData>(RowName, TEXT("ListItems"));
		if (RowData)
		{
			UE_LOG(LogTemp, Log, TEXT("  - ID: [%s] | Name: [%s] | MaxQuantity: %d"), *RowName.ToString(), *RowData->ItemName.ToString(), RowData->MaxQuantity);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("  - ID: [%s]"), *RowName.ToString());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("==========================================="));
}

void AAGSDPlayerController::AddDamage(float Amount)
{
	AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(GetPawn());
	if (!PlayerChar)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Player character not found!"));
		return;
	}

	PlayerChar->AddDamage(Amount);

	// GameInstance 동기화
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		GI->Damage = PlayerChar->getDamage();
	}

	FString Msg = FString::Printf(TEXT("[Cheat] Added %.1f Damage! Current Damage: %.1f"), Amount, PlayerChar->getDamage());
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg);
}

void AAGSDPlayerController::SetDamage(float NewDamage)
{
	AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(GetPawn());
	if (!PlayerChar)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Player character not found!"));
		return;
	}

	float CurrentDmg = PlayerChar->getDamage();
	float Diff = NewDamage - CurrentDmg;
	PlayerChar->AddDamage(Diff);

	// GameInstance 동기화
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		GI->Damage = PlayerChar->getDamage();
	}

	FString Msg = FString::Printf(TEXT("[Cheat] Set Damage to %.1f!"), PlayerChar->getDamage());
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg);
}

void AAGSDPlayerController::SpawnPet()
{
	AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(GetPawn());
	if (!PlayerChar)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Player character not found!"));
		return;
	}

	// 1. 이미 펫이 소환되어 있는 경우: 플레이어 위치로 즉시 복귀/호출
	if (PlayerChar->Pet && IsValid(PlayerChar->Pet))
	{
		PlayerChar->Pet->ReturnToPlayer();
		FString Msg = TEXT("[Cheat] 펫이 이미 소환되어 있습니다. 플레이어 위치로 호출했습니다.");
		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, Msg);
		return;
	}

	// 2. DefaultPetClass 지정 여부 검사
	if (!PlayerChar->DefaultPetClass)
	{
		FString ErrMsg = TEXT("[Cheat] Failed: DefaultPetClass가 설정되지 않았습니다. 캐릭터 블루프린트의 DefaultPetClass를 확인해주세요.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrMsg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrMsg);
		return;
	}

	// 3. 펫 스폰 파라미터 및 위치 계산 (플레이어 뒤쪽 공중)
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PlayerChar;
	SpawnParams.Instigator = PlayerChar->GetInstigator();

	FVector PetSpawnLoc = PlayerChar->GetActorLocation() + (PlayerChar->GetActorForwardVector() * -100.f) + FVector(0.f, 0.f, 80.f);
	FRotator PetSpawnRot = PlayerChar->GetActorRotation();

	if (ABaseFlyingPet* SpawnedPet = GetWorld()->SpawnActor<ABaseFlyingPet>(PlayerChar->DefaultPetClass, PetSpawnLoc, PetSpawnRot, SpawnParams))
	{
		PlayerChar->SetPet(SpawnedPet);
		PlayerChar->bHasPet = true;

		// GameInstance 상태 동기화 (맵 이동 및 저장 시에도 펫 보유 유지)
		if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
		{
			GI->bHasPet = true;
		}

		FString Msg = TEXT("[Cheat] 펫이 성공적으로 소환되었습니다! (SpawnPet / SummonPet)");
		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg);
	}
	else
	{
		FString ErrMsg = TEXT("[Cheat] Failed: 펫 액터 스폰에 실패했습니다.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrMsg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, ErrMsg);
	}
}

void AAGSDPlayerController::DestroyPet()
{
	AAGSDCharacter* PlayerChar = Cast<AAGSDCharacter>(GetPawn());
	if (!PlayerChar)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: Player character not found!"));
		return;
	}

	if (PlayerChar->Pet && IsValid(PlayerChar->Pet))
	{
		PlayerChar->Pet->Destroy();
		PlayerChar->Pet = nullptr;
	}

	PlayerChar->bHasPet = false;

	// GameInstance 상태 동기화
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(GetGameInstance()))
	{
		GI->bHasPet = false;
	}

	FString Msg = TEXT("[Cheat] 펫을 역소환(삭제)했습니다. (DestroyPet / DismissPet)");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, Msg);
}

void AAGSDPlayerController::CompleteQuest()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>();
	if (!TutSub)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: TutorialSubsystem not found!"));
		return;
	}

	if (!TutSub->IsTutorialActive())
	{
		FString Msg = TEXT("[Cheat] 현재 진행 중인 퀘스트가 없습니다.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, Msg);
		return;
	}

	TutSub->SkipCurrentSequence();

	FString Msg = TEXT("[Cheat] 현재 퀘스트를 성공적으로 전체 완료 처리했습니다! (CompleteQuest)");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg);
}

void AAGSDPlayerController::CompleteQuestStep()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>();
	if (!TutSub)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] Failed: TutorialSubsystem not found!"));
		return;
	}

	if (!TutSub->IsTutorialActive())
	{
		FString Msg = TEXT("[Cheat] 현재 진행 중인 퀘스트가 없습니다.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, Msg);
		return;
	}

	FTutorialStepData StepData;
	TutSub->GetCurrentStepData(StepData);

	TutSub->ForceCompleteCurrentStep();

	FString Msg = FString::Printf(TEXT("[Cheat] 퀘스트 스텝 [%s]을(를) 강제 완료했습니다! (CompleteQuestStep)"), *StepData.StepID.ToString());
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg);
}

void AAGSDPlayerController::CancelQuest()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>();
	if (!TutSub || !TutSub->IsTutorialActive())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, TEXT("[Cheat] 취소할 활성 퀘스트가 없습니다."));
		return;
	}

	TutSub->CancelCurrentSequence();

	FString Msg = TEXT("[Cheat] 현재 퀘스트를 즉시 취소/삭제했습니다. (CancelQuest)");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, Msg);
}

void AAGSDPlayerController::ResetQuest()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>();
	if (TutSub)
	{
		TutSub->ResetAllQuests();
	}

	FString Msg = TEXT("[Cheat] 모든 퀘스트 완료 기록과 진행 상태를 초기화했습니다! (ResetQuest / ResetAllQuests)");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
}

void AAGSDPlayerController::PrintQuestStatus()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>();
	if (!TutSub)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[Cheat] TutorialSubsystem not found!"));
		return;
	}

	bool bActive = TutSub->IsTutorialActive();
	int32 StepIdx = TutSub->GetCurrentStepIndex();
	int32 TotalSteps = TutSub->GetTotalStepCount();
	TArray<FName> Completed = TutSub->GetCompletedSequences();
	FString CompList = TEXT("None");
	if (Completed.Num() > 0)
	{
		CompList = FString::JoinBy(Completed, TEXT(", "), [](const FName& Name) { return Name.ToString(); });
	}

	FString StepName = TEXT("None");
	FTutorialStepData StepData;
	if (bActive && TutSub->GetCurrentStepData(StepData))
	{
		StepName = StepData.StepID.ToString();
	}

	FString Msg = FString::Printf(TEXT("[QuestStatus] Active: %s | Step: %d/%d (ID: %s) | Completed: [%s]"),
		bActive ? TEXT("TRUE") : TEXT("FALSE"), bActive ? (StepIdx + 1) : 0, TotalSteps, *StepName, *CompList);

	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Cyan, Msg);
}



