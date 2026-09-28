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

protected:
	/** 다음 단계로 이동 */
	void AdvanceToNextStep();

	/** 현재 단계에 맞게 펫 및 웨이포인트 시각 연출 세팅 */
	void SetupCurrentStepVisuals();

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
	FName CurrentSequenceName = NAME_None;
	bool bIsActive = false;

	UPROPERTY()
	bool bCompletedHubTutorial = false;

	TWeakObjectPtr<AActor> CachedWaypointActor = nullptr;
};
