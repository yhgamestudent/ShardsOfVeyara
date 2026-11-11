// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AGSDTrainSplineLibrary.generated.h"

class AActor;
class USplineComponent;

/**
 * 스플라인 거리(Distance) 기반 순환 기차 주행 설정 구조체
 */
USTRUCT(BlueprintType)
struct FAGSDLoopingTrainSettings
{
	GENERATED_BODY()

	/** 목표 순항 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Movement", meta = (ClampMin = "0.0"))
	double MaxSpeed = 500.0;

	/** 출발 가속도 (cm/s^2, 0 이하 입력 시 즉시 최고 속도로 출발) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Movement", meta = (ClampMin = "0.0"))
	double Acceleration = 250.0;

	/** 정지 감속도 (cm/s^2, 0 이하 입력 시 즉시 정지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Movement", meta = (ClampMin = "0.0"))
	double Deceleration = 300.0;

	/** 기관차와 각 화물칸 사이 스플라인 거리 간격 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Carriage", meta = (ClampMin = "0.0"))
	double CarriageSpacing = 900.0;

	/** 스플라인 대비 차체 위아래 높이 오프셋 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Alignment")
	double HeightOffset = 0.0;

	/** 기차 메시의 정면 축이 +X가 아닐 경우 맞출 로컬 회전 오프셋 (예: Yaw = 90, -90, 180) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Alignment")
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** true면 오르막/내리막 기울기(Pitch/Roll)를 0으로 고정해 평지 탑승 안정성을 높입니다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Alignment")
	bool bFlattenPitchAndRoll = false;

	/** 스플라인 포인트 역방향(SplineLength -> 0) 주행 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Movement")
	bool bReverseDirection = false;

	/** 기존 FloatingPawnMovement 등 폰 이동 컴포넌트가 남아있을 때 간섭 방지를 위해 자동 비활성화할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Safety")
	bool bDisablePawnMovement = true;

	/** true면 스플라인의 Closed Loop가 꺼져 있을 때 자동으로 닫힘 설정 적용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Track")
	bool bEnsureClosedLoop = true;
};

/**
 * UAGSDTrainSplineLibrary
 *
 * 도시 맵의 기차(BP_Train) 및 화물칸(BP_TrainCarriage)이 스플라인 트랙 위를
 * 탈선 없이 무한 순환 주행하도록 지원하는 블루프린트 함수 라이브러리입니다.
 */
UCLASS()
class AGSD_API UAGSDTrainSplineLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * [권장] 스플라인 거리(Distance) 기반으로 선두 기관차(LeaderTrain)와 연결된 화물칸(Carriages)을
	 * 단 1번의 호출로 동시에 무한 순환 이동/회전시킵니다.
	 * - 레일 위를 100% 정확히 따라가며 탈선이나 공중 이탈이 발생하지 않습니다.
	 * - 화물칸 간격이 영구적으로 유지되며, ETeleportType::None 적용으로 탑승 캐릭터 물리도 안전하게 유지됩니다.
	 *
	 * @param LeaderTrain 선두 기차 액터 (보통 BP_Train Self)
	 * @param Carriages 뒤따를 화물칸 액터 배열 (비워두면 기관차만 이동)
	 * @param SplineComp 따라갈 트랙(BP_TrainTrack)의 USplineComponent
	 * @param DeltaSeconds 프레임 델타 타임 (0 입력 시 World DeltaSeconds 자동 사용)
	 * @param bShouldMove true면 가속/주행, false면 감속/정지
	 * @param Settings 속도, 가감속, 화물칸 간격, 높이/회전 오프셋 등 설정값
	 * @param CurrentSpeed [Ref] 현재 주행 속도 (cm/s, 내부에서 자동 갱신)
	 * @param CurrentDistance [Ref] 현재 선두 기차 스플라인 위치 거리 (cm, BeginPlay에서 FindClosestDistanceOnSpline으로 초기화 권장)
	 * @param OutNormalizedSpeed 현재 속도의 0.0~1.0 정규화 비율 (기차 사운드 볼륨/피치 연동용)
	 * @return 유효한 스플라인 업데이트가 수행되었으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "AGSD|Train", meta = (DefaultToSelf = "LeaderTrain", AutoCreateRefTerm = "Carriages,Settings"))
	static bool UpdateLoopingTrainWithCarriages(
		AActor* LeaderTrain,
		const TArray<AActor*>& Carriages,
		USplineComponent* SplineComp,
		double DeltaSeconds,
		bool bShouldMove,
		const FAGSDLoopingTrainSettings& Settings,
		UPARAM(ref) double& CurrentSpeed,
		UPARAM(ref) double& CurrentDistance,
		float& OutNormalizedSpeed
	);

	/**
	 * BeginPlay 등에서 현재 액터 위치로부터 가장 가까운 스플라인 시작 거리(cm)를 구합니다.
	 * 게임 시작 시 기차가 0번 지점으로 순간이동하지 않고 배치된 위치 그대로 출발하게 할 때 사용합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "AGSD|Train", meta = (DefaultToSelf = "TargetActor"))
	static double FindClosestDistanceOnSpline(const AActor* TargetActor, const USplineComponent* SplineComp);

	/**
	 * 기존 BP_Train / BP_TrainCarriage의 'Get Path' 함수를 대체하여,
	 * 액터가 닫힌 스플라인 트랙을 무한으로 따라가도록 회전 및 횡방향 레일 중심 이탈 보정을 수행합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "AGSD|Train", meta = (DefaultToSelf = "TargetActor"))
	static bool UpdateLoopingSplineFollow(
		AActor* TargetActor,
		USplineComponent* SplineComp,
		double DeltaSeconds,
		double& OutCurrentDistance,
		FRotator& OutTargetRotation,
		double LookAheadDistance = 120.0,
		float RotationInterpSpeed = 12.0f,
		float RailCenterCorrectionSpeed = 4.0f,
		bool bFollowSplinePitch = true,
		bool bPreserveActorZ = true,
		bool bReverseDirection = false,
		bool bEnsureClosedLoop = true
	);

	/**
	 * 임의의 스플라인 거리를 [0, SplineLength) 범위로 안전하게 순환(Wrap)시킵니다.
	 * 음수 거리(예: 0번 포인트 직전 위치의 화물칸)도 끊김 없이 스플라인 끝 부근으로 변환합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "AGSD|Train")
	static double WrapSplineDistance(double Distance, double SplineLength);

	/**
	 * 선두 액터(예: BP_Train)의 스플라인 위치로부터 지정된 거리만큼 뒤따르는
	 * 후행 액터(예: BP_TrainCarriage)의 목표 스플라인 위치, 회전 및 거리를 순수 계산합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "AGSD|Train")
	static bool GetTrainCarriageFollowTransform(
		const AActor* LeaderActor,
		const USplineComponent* SplineComp,
		double FollowOffsetDistance,
		bool bReverseDirection,
		FVector& OutTargetLocation,
		FRotator& OutTargetRotation,
		double& OutTargetDistance
	);

private:
	/** 가감속 운동학(속도 및 누적 거리)을 계산하여 상태를 갱신합니다 */
	static void AdvanceLoopingTrainKinematics(
		double DeltaSeconds,
		bool bShouldMove,
		const FAGSDLoopingTrainSettings& Settings,
		double SplineLength,
		double& InOutCurrentSpeed,
		double& InOutCurrentDistance
	);

	/** 특정 스플라인 거리에서의 월드 위치/회전(오프셋 및 평탄화 반영)을 계산합니다 */
	static void ComputeSplineActorTransform(
		const USplineComponent* SplineComp,
		double Distance,
		const FAGSDLoopingTrainSettings& Settings,
		FVector& OutWorldLocation,
		FRotator& OutWorldRotation
	);

	/** 간섭하는 PawnMovement 비활성화 및 ETeleportType::None으로 안전하게 트랜스폼을 적용합니다 */
	static void ApplyTrainTransformSafely(
		AActor* TargetActor,
		const FVector& WorldLocation,
		const FRotator& WorldRotation,
		bool bDisablePawnMovement
	);

	/** 순수 스플라인 추종 목표 위치 및 회전값을 계산합니다 (UpdateLoopingSplineFollow 전용) */
	static void EvaluateLoopingSplineTarget(
		const USplineComponent* SplineComp,
		const FVector& ActorLocation,
		double LookAheadDistance,
		bool bFollowSplinePitch,
		bool bPreserveActorZ,
		bool bReverseDirection,
		double& OutCurrentDistance,
		FVector& OutRailCenterLocation,
		FRotator& OutTargetRotation
	);
};
