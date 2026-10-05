#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TutorialTypes.h"
#include "TutorialSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialStepStarted, const FTutorialStepData&, StepData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTutorialStepProgress, int32, CurrentCount, int32, RequiredCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialStepCompleted, const FTutorialStepData&, CompletedStep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialSequenceCompleted, FName, SequenceName);

/**
 * 게임의 모든 튜토리얼 시퀀스를 총괄 관리하는 중앙 서브시스템입니다.
 * 행동 검증(Action-Gating), 동반자 펫 안내, 스크린 마커 연동, 장벽 해제를 제어합니다.
 */
UCLASS()
class AGSD_API UTutorialSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UTutorialSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * 특정 데이터 테이블 기반으로 튜토리얼 시퀀스를 시작합니다.
	 * @param TutorialTable FTutorialStepData 행 구조를 가진 데이터 테이블
	 * @param SequenceName 시퀀스 식별자 (예: Tutorial_Village, Tutorial_Sky_Island, Farm_Sky_Island)
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorialSequence(UDataTable* TutorialTable, FName SequenceName);

	/**
	 * 플레이어나 상호작용 오브젝트가 수행한 행동을 보고합니다.
	 * 현재 스텝의 목표 행동과 일치하면 카운트가 올라가고, 목표 달성 시 다음 스텝으로 진행합니다.
	 * @param ActionType 수행된 행동 타입
	 * @param Count 수행 횟수 (기본 1)
	 * @param CustomTag ActionType이 Custom일 때 사용하는 식별 태그
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ReportTutorialAction(ETutorialActionType ActionType, int32 Count = 1, FName CustomTag = NAME_None);

	/**
	 * 플레이어가 E키로 상호작용을 수행했을 때 호출하여 튜토리얼 스텝 조건을 검증합니다.
	 * 범용 상호작용(Interact)뿐만 아니라 목표 웨이포인트 액터나 특정 태그 일치 여부를 판정합니다.
	 * @param InteractedActor 상호작용한 대상 액터
	 * @param InteractionType 상호작용 액션 타입 문자열 (선택)
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ReportInteractionAction(AActor* InteractedActor, const FString& InteractionType = TEXT(""));

	/**
	 * 특정 레벨/맵으로 전환되었음을 알립니다. (맵 전환 시점 외부/수동 호출용)
	 * 현재 스텝의 목표 레벨과 일치하거나 EnterPortal 조건이면 스텝을 완료합니다.
	 * @param NewLevelName 전환된 맵/레벨 이름
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ReportLevelChanged(FName NewLevelName);

	/**
	 * 현재 진행 중인 스텝을 강제로 즉시 완료 처리합니다. (블루프린트 수동 제어용)
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ForceCompleteCurrentStep();

	/**
	 * 현재 진행 중인 튜토리얼 시퀀스를 즉시 건너뜁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SkipCurrentSequence();

	/** 현재 스텝 데이터 반환 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool GetCurrentStepData(FTutorialStepData& OutStepData) const;

	/** 현재 스텝 인덱스 (0-based) */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetCurrentStepIndex() const { return CurrentStepIndex; }

	/** 전체 스텝 개수 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetTotalStepCount() const { return CurrentSteps.Num(); }

	/** 현재 튜토리얼이 진행 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsTutorialActive() const { return bIsActive; }

	/** 거점(Farm_Sky_Island) 튜토리얼 완료 여부 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsHubTutorialCompleted() const { return bCompletedHubTutorial; }

	/** 거점 튜토리얼 완료 여부 설정 (세이브 데이터 연동) */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetHubTutorialCompleted(bool bCompleted);

	/** 현재 가리키고 있는 목표 웨이포인트 액터 반환 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	AActor* GetCurrentWaypointActor() const;

	/** 현재 가리키고 있는 목표 월드 위치 반환 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	FVector GetCurrentTargetLocation() const;

	// --- 델리게이트 이벤트 ---
	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialStepStarted OnTutorialStepStarted;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialStepProgress OnTutorialStepProgress;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialStepCompleted OnTutorialStepCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialSequenceCompleted OnTutorialSequenceCompleted;

	/** 지점 도달(ReachArea) 판정 수평 반경 (cm 단위, 기본 150cm = 1.5m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ReachArea")
	float ReachAreaDistanceThreshold = 150.0f;

	/** 지점 도달(ReachArea) 판정 수직 높이 허용치 (cm 단위, 기본 200cm = 2.0m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ReachArea")
	float ReachAreaZThreshold = 200.0f;

	/** 현재 스텝의 세부 목표별 진행도 텍스트를 생성하여 반환 (예: "작물 심기 [ 1 / 3 ]  |  작물 수확 [ 2 / 3 ]") */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	FText GetDetailedProgressText() const;

	/** 세부 목표별 현재 카운트 배열 반환 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	TArray<int32> GetCurrentObjectiveCounts() const { return CurrentObjectiveCounts; }

	/** 현재 스텝의 대화(다이얼로그)가 끝나기를 대기 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsWaitingForDialogue() const { return bWaitingForDialogue; }

protected:
	/** 다음 단계로 이동 */
	void AdvanceToNextStep();

	/** 현재 스텝 클리어 및 보상 지급, 장벽 해제 처리 */
	void CompleteCurrentStep();

	/** 미완료 목표 중 플레이어와 가장 가까운 웨이포인트를 찾아 펫/마커 안내 갱신 */
	void UpdateNearestWaypoint();

	/** 현재 단계에 맞게 펫 및 웨이포인트 시각 연출 세팅 */
	void SetupCurrentStepVisuals();

	/** ReachArea 액션 스텝 진행 시 주기적으로 플레이어 위치를 확인하는 함수 */
	UFUNCTION()
	void CheckPlayerReachArea();

	/** 동반자 펫의 대화(다이얼로그)가 끝났을 때 호출되는 콜백 */
	UFUNCTION()
	void HandleDialogueFinished();

	/** 엔진 레벨 전환(PostLoadMapWithWorld) 완료 시 호출되는 콜백 */
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	/** 맵 전환 후 새 월드의 액터들이 안정적으로 스폰된 뒤 퀘스트 조건을 판정하는 함수 */
	UFUNCTION()
	void HandlePostMapTransitionCheck();

	/** 장벽/문 액터 열기 */
	void OpenGateActor(FName GateTag);

	/** 맵에서 특정 태그를 가진 액터 검색 */
	AActor* FindActorWithTag(FName Tag) const;

	/** 플레이어의 동반자 펫 참조 가져오기 */
	class ABaseFlyingPet* GetPlayerPet() const;

private:
	UPROPERTY()
	TArray<FTutorialStepData> CurrentSteps;

	int32 CurrentStepIndex = 0;
	int32 CurrentActionCount = 0;

	/** 현재 스텝의 세부 목표별 달성 카운트 목록 */
	UPROPERTY()
	TArray<int32> CurrentObjectiveCounts;

	FName CurrentSequenceName = NAME_None;
	bool bIsActive = false;

	/** 현재 스텝의 대화가 진행 중이어서 퀘스트 부여를 대기하고 있는지 여부 */
	bool bWaitingForDialogue = false;

	UPROPERTY()
	bool bCompletedHubTutorial = false;

	TWeakObjectPtr<AActor> CachedWaypointActor = nullptr;

	FTimerHandle ReachAreaTimerHandle;

	/** 맵 전환 감지 시 로드된 새 맵 이름 */
	FName PendingLoadedMapName = NAME_None;

	/** 맵 전환 후 안정화 대기 타이머 */
	FTimerHandle LevelTransitionTimerHandle;
};
