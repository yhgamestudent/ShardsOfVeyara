// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InteractionWidget.h"
#include "ClockWidget.h"
#include "AGSDPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AAGSDPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AAGSDPlayerController();

	// [우측] 월드 상호작용 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Interaction")
	TSubclassOf<class UInteractionWidget> WBP_InteractionWidget;

	// [좌측] 손에 든 아이템 사용 안내 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Interaction")
	TSubclassOf<class UInteractionWidget> WBP_LeftInteractionWidget;

	// [상단] 시스템 경고 및 안내 토스트 알림 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Notice")
	TSubclassOf<class UAGSDWarningNoticeWidget> WBP_WarningNoticeWidget;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> WBP_ClockWidget;

	UFUNCTION(BlueprintCallable, Category = "UI|Interaction")
	void ShowInteractionWidget(const FText& NewText);
	
	void HideInteractionWidget();

	UFUNCTION(BlueprintCallable, Category = "UI|Interaction")
	void ShowLeftItemUseWidget(const FText& NewText);

	void HideLeftItemUseWidget();

	/** 화면 상단에 경고/안내 토스트 메시지를 표시합니다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Notice")
	void ShowWarningNotice(const FText& Message, float Duration = 1.5f);

	void ShowClockWidget();
	UFUNCTION(BlueprintCallable)
	void CutsceneHideClockWidget();
	UFUNCTION(BlueprintCallable)
	void CutsceneShowClockWidget();
private:
	UPROPERTY()
	class UInteractionWidget* InteractionWidget;

	UPROPERTY()
	class UInteractionWidget* LeftInteractionWidget;

	UPROPERTY()
	class UAGSDWarningNoticeWidget* WarningNoticeWidget;

	UPROPERTY()
	class UUserWidget* ClockWidget;
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	// ── 치트 콘솔 커맨드 (Cheat Console Commands) ──
public:
	/** 
	 * 지정한 ID의 아이템을 인벤토리에 지급합니다.
	 * 사용법: GiveItem <ItemID> <Amount> (예: GiveItem Apple 5)
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void GiveItem(const FString& ItemID, int32 Amount = 1);

	/** 
	 * 데이터 테이블의 모든 아이템을 인벤토리에 일괄 지급합니다.
	 * 사용법: GiveAllItems <Amount> (예: GiveAllItems 1)
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void GiveAllItems(int32 Amount = 1);

	/** 
	 * 인벤토리의 모든 아이템을 비웁니다.
	 * 사용법: ClearInventory
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void ClearInventory();

	/** 
	 * 사용 가능한 모든 아이템 ID 목록을 콘솔/로그에 출력합니다.
	 * 사용법: ListItems
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void ListItems();

	/** 
	 * 플레이어의 공격력을 지정한 수치만큼 증가시킵니다.
	 * 사용법: AddDamage <Amount> (예: AddDamage 50)
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void AddDamage(float Amount = 10.0f);

	/** 
	 * 플레이어의 공격력을 지정한 수치로 직접 설정합니다.
	 * 사용법: SetDamage <NewDamage> (예: SetDamage 500)
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void SetDamage(float NewDamage);

	/** 
	 * 동반자 펫을 소환합니다. (이미 소환되어 있다면 플레이어 위치로 호출)
	 * 사용법: SpawnPet 또는 SummonPet
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void SpawnPet();

	UFUNCTION(Exec, Category = "Cheats")
	void SummonPet() { SpawnPet(); }

	/** 
	 * 현재 소환된 펫을 소멸시키고 소환 해제합니다.
	 * 사용법: DestroyPet 또는 DismissPet
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void DestroyPet();

	UFUNCTION(Exec, Category = "Cheats")
	void DismissPet() { DestroyPet(); }

	/** 
	 * 현재 진행 중인 퀘스트(튜토리얼 시퀀스) 전체를 즉시 완료 처리합니다.
	 * 사용법: CompleteQuest 또는 ClearQuest
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void CompleteQuest();

	UFUNCTION(Exec, Category = "Cheats")
	void ClearQuest() { CompleteQuest(); }

	/** 
	 * 현재 진행 중인 퀘스트의 현재 단계(스텝)만 즉시 완료하고 다음 단계로 진행합니다.
	 * 사용법: CompleteQuestStep 또는 ClearStep
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void CompleteQuestStep();

	UFUNCTION(Exec, Category = "Cheats")
	void ClearStep() { CompleteQuestStep(); }

	/** 
	 * 현재 진행 중인 퀘스트를 완료 처리하지 않고 즉시 취소/삭제합니다.
	 * 사용법: CancelQuest
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void CancelQuest();

	/** 
	 * 완료된 모든 퀘스트 기록 및 진행 중인 퀘스트를 초기화하여 퀘스트를 처음부터 다시 받을 수 있도록 리셋합니다.
	 * 사용법: ResetQuest 또는 ResetAllQuests
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void ResetQuest();

	UFUNCTION(Exec, Category = "Cheats")
	void ResetAllQuests() { ResetQuest(); }

	/** 
	 * 현재 활성 퀘스트 상태와 완료된 퀘스트 목록을 화면 및 로그에 출력합니다.
	 * 사용법: PrintQuestStatus 또는 QuestStatus
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void PrintQuestStatus();

	UFUNCTION(Exec, Category = "Cheats")
	void QuestStatus() { PrintQuestStatus(); }

	// ─── [디버그/테스트용 플레이어 체력 치트 명령어] ───

	/** 
	 * 플레이어에게 지정한 수치만큼 데미지를 입힙니다.
	 * 사용법: DamagePlayer <Amount> (예: DamagePlayer 50 또는 Damage 30)
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void DamagePlayer(float Amount = 50.0f);

	UFUNCTION(Exec, Category = "Cheats")
	void Damage(float Amount = 50.0f) { DamagePlayer(Amount); }

	UFUNCTION(Exec, Category = "Cheats")
	void Hurt(float Amount = 50.0f) { DamagePlayer(Amount); }

	/** 
	 * 플레이어 체력을 원하는 수치로 즉시 설정합니다.
	 * 사용법: SetPlayerHealth <NewHealth> (예: SetPlayerHealth 10 또는 SetHP 20)
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void SetPlayerHealth(float NewHealth);

	UFUNCTION(Exec, Category = "Cheats")
	void SetHP(float NewHealth) { SetPlayerHealth(NewHealth); }

	/** 
	 * 플레이어를 즉시 사망시킵니다.
	 * 사용법: KillPlayer 또는 Suicide
	 */
	UFUNCTION(Exec, Category = "Cheats")
	void KillPlayer();

	UFUNCTION(Exec, Category = "Cheats")
	void Suicide() { KillPlayer(); }

private:
	/** 플레이어 캐릭터의 인벤토리 컴포넌트를 가져오는 헬퍼 */
	class UAGSDInventoryComponent* GetPlayerInventoryComponent() const;
};
