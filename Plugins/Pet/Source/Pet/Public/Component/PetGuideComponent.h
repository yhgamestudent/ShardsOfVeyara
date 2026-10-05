#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PetGuideComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReachedWaypoint);

/**
 * 동반자 펫의 튜토리얼/퀘스트 웨이포인트 비행 및 이정표 안내를 담당하는 컴포넌트입니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PET_API UPetGuideComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPetGuideComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 웨이포인트 목표 지점 도달 시 호출되는 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Pet|Guide")
	FOnReachedWaypoint OnReachedWaypoint;

	/**
	 * 지정된 3D 월드 좌표로 비행 이동을 시작합니다.
	 * @param NewTargetLocation 이동할 목표 위치
	 * @param ArrivalRadius 도착 판정 반경 (cm)
	 */
	UFUNCTION(BlueprintCallable, Category = "Pet|Guide")
	void MoveToLocation(const FVector& NewTargetLocation, float ArrivalRadius = 150.0f);

	/**
	 * 특정 액터(타겟 포인트, NPC, 오브젝트 등)를 향해 비행 이동을 시작합니다.
	 * @param GoalActor 목표 액터
	 * @param Offset 목표 액터 기준 상대 오프셋 (기본 머리 위 높이)
	 * @param ArrivalRadius 도착 판정 반경 (cm)
	 */
	UFUNCTION(BlueprintCallable, Category = "Pet|Guide")
	void MoveToActor(AActor* GoalActor, FVector Offset = FVector(0.f, 0.f, 100.f), float ArrivalRadius = 150.0f);

	/**
	 * 목표 위치로 즉시 순간이동합니다. (점프맵 통과 등)
	 * @param NewTargetLocation 순간이동할 위치
	 */
	UFUNCTION(BlueprintCallable, Category = "Pet|Guide")
	void TeleportToLocation(const FVector& NewTargetLocation);

	/**
	 * 특정 목표 액터로 순간이동 이펙트와 함께 즉시 이동합니다.
	 * @param GoalActor 이동할 목표 액터
	 */
	UFUNCTION(BlueprintCallable, Category = "Pet|Guide")
	void TeleportToActor(AActor* GoalActor);

	/**
	 * 안내 모드를 종료하고 다시 플레이어를 따라다니는 기본 상태로 복귀합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pet|Guide")
	void ReturnToFollow();

	/**
	 * 펫 메쉬 및 충돌체를 숨기거나 표시합니다. (마을 맵 투명 이정표 연출용)
	 * @param bHidden true면 펫 메쉬를 숨김
	 */
	UFUNCTION(BlueprintCallable, Category = "Pet|Guide")
	void SetPetMeshHidden(bool bHidden);

	/**
	 * 머리 위 느낌표(!) 강조 마커의 가시성을 설정합니다.
	 * @param bVisible true면 마커 표시
	 */
	UFUNCTION(BlueprintCallable, Category = "Pet|Guide")
	void SetExclamationMarkVisible(bool bVisible);

	/** 현재 웨이포인트 안내 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "Pet|Guide")
	bool IsGuiding() const { return bIsGuiding; }

	/** 목표 지점에 도착했는지 여부 */
	UFUNCTION(BlueprintPure, Category = "Pet|Guide")
	bool HasArrived() const { return bHasArrived; }

	/** 현재 가리키고 있는 목표 월드 좌표 반환 (스크린 마커 UI 등에서 참조) */
	UFUNCTION(BlueprintPure, Category = "Pet|Guide")
	FVector GetCurrentTargetLocation() const;

	// 비행 이동 속도 (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pet|Guide")
	float FlightSpeed = 800.0f;

	// 목표 방향 회전 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pet|Guide")
	float RotationInterpSpeed = 6.0f;

	// 도착 후 제자리 부유(Hovering) 진폭
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pet|Guide")
	float HoverAmplitude = 15.0f;

	// 도착 후 제자리 부유(Hovering) 주기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pet|Guide")
	float HoverFrequency = 2.0f;

	// 느낌표 마커로 사용할 위젯 컴포넌트 오프셋 (머리 위 높이)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pet|Guide")
	FVector ExclamationMarkerOffset = FVector(0.f, 0.f, 60.f);

private:
	UPROPERTY()
	class ABaseFlyingPet* OwnerPet;

	UPROPERTY()
	class UWidgetComponent* ExclamationWidgetComp;

	bool bIsGuiding = false;
	bool bHasArrived = false;

	FVector TargetLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> TargetActor = nullptr;
	FVector TargetOffset = FVector::ZeroVector;
	float CurrentArrivalRadius = 150.0f;

	float HoverTime = 0.0f;
	FVector BaseArrivalLocation = FVector::ZeroVector;
};
