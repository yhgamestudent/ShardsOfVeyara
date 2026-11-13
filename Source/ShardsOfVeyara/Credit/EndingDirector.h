#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndingDirector.generated.h"

class UUserWidget;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

/**
 * 엔딩 연출 진행자
 * - USOVGameInstance가 엔딩 맵(기본: Tutorial_Village) 진입 시 bIsEnding == true면 스폰합니다.
 * - 순서: 엔딩 플래그 해제 → 플레이어 입력/HUD 차단 → (엔딩 시퀀스 재생) → 크레딧 위젯 표시
 * - 크레딧 위젯(UCreditWidget)이 스크롤 종료 후 메인 메뉴로 이동합니다.
 */
UCLASS(NotBlueprintable)
class SHARDSOFVEYARA_API AEndingDirector : public AActor
{
	GENERATED_BODY()

public:
	AEndingDirector();

	/** 스폰 직후 게임 인스턴스가 설정값 전달 */
	void Setup(TSubclassOf<UUserWidget> InCreditWidgetClass, TSoftObjectPtr<ULevelSequence> InEndingSequence, float InStartDelay);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> CreditWidgetClass;

	UPROPERTY()
	TSoftObjectPtr<ULevelSequence> EndingSequence;

	/** 맵 로드 직후 캐릭터/HUD 초기화가 끝날 때까지 대기 (초) */
	float StartDelay = 0.3f;

	UPROPERTY()
	TObjectPtr<ULevelSequencePlayer> SequencePlayer;

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	UPROPERTY()
	TObjectPtr<UUserWidget> CreditWidget;

	FTimerHandle StartTimerHandle;

	void StartEnding();
	void LockPlayer();

	UFUNCTION()
	void OnEndingSequenceFinished();

	void ShowCredits();
};
