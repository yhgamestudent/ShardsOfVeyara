#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AGSDInteractionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AGSD_API UAGSDInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAGSDInteractionComponent();

protected:
	virtual void BeginPlay() override;

public:
	// 매 프레임 캐릭터 Tick에서 호출되어 최단 거리 상호작용 대상 감지 및 하이라이트를 갱신합니다.
	void UpdateInteractionState(float DeltaSeconds);

	// 상호작용 키 입력 시 최우선 타겟과 상호작용 수행
	void TryInteract();

	// 상호작용 가능 영역 진입/이탈 시 액터 수집
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AddInteractableActor(AActor* NewActor);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RemoveInteractableActor(AActor* ActorToRemove);

	// 외부 조회 함수
	FORCEINLINE AActor* GetCurrentInteractableActor() const { return CurrentInteractableActor; }
	FORCEINLINE int32 GetInteractableActorNum() const { return InteractableActorsInRange.Num(); }
	FORCEINLINE bool GetCanInteract() const { return CanInteract; }

	// 아웃라인 하이라이트 제어
	void SetHighLight(AActor* TargetActor, bool bActive);

private:
	// 현재 상호작용 대상
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> CurrentInteractableActor = nullptr;

	// 상호작용 범위 내 수집된 모든 액터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TSet<TObjectPtr<AActor>> InteractableActorsInRange;

	// 상호작용 가능 여부 플래그
	bool CanInteract = false;

	// 헬퍼 함수
	AActor* MinDistActor();

	// 소유주 캐릭터 캐싱
	UPROPERTY()
	TObjectPtr<class AAGSDCharacter> OwnerCharacter;
};
