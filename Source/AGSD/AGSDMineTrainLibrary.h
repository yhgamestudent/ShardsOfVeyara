// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/SplineComponent.h"
#include "AGSDMineTrainLibrary.generated.h"

/**
 * UAGSDMineTrainLibrary
 * 
 * 마인 트레인(롤러코스터/광산 카트)의 스플라인 주행, 가속, 중력 물리, 
 * 마지막 구간 감속 판별을 일괄 처리하는 블루프린트 함수 라이브러리입니다.
 */
UCLASS()
class AGSD_API UAGSDMineTrainLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 마인 트레인의 이동, 런칭 가속, 중력 가속, 마지막 라인 진입 감속, 거리 누적을 일괄 계산합니다.
	 * 
	 * @param SplineComp 이동 경로가 되는 트랙의 USplineComponent
	 * @param DeltaSeconds 프레임 델타 타임 (GetWorldDeltaSeconds 또는 Tick DeltaSeconds)
	 * @param LaunchAcceleration 출발 런칭 가속도
	 * @param TargetLaunchSpeed 런칭 목표 속도 (도달 시 bIsLaunching 해제)
	 * @param GravityScale 중력 가속 계수 (내리막길 가속, 오르막길 감속)
	 * @param MinSpeed 보장되는 최소 속도
	 * @param BrakeDeceleration 마지막 라인 진입 시 초당 감속도 (0 이하 입력 시 부드러운 보간 FInterpTo 적용)
	 * @param bIsLaunching [Ref] 현재 런칭 가속 중인지 여부 (목표 속도 도달 시 false로 자동 변경)
	 * @param CurrentSpeed [Ref] 현재 속도 (내부에서 계산 후 자동 갱신)
	 * @param CurrentDistance [Ref] 현재 스플라인 누적 거리 (내부에서 속도에 따라 자동 누적)
	 * @param OutNewLocation 이번 프레임의 스플라인 월드 위치
	 * @param OutNewRotation 이번 프레임의 스플라인 월드 회전값
	 * @param bOutIsFinished 트랙 끝에 도달했는지 여부 (도착 시 true)
	 * @param bOutIsBraking 마지막 포인트 전 라인(감속 구간)에 진입했는지 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "AGSD|MineTrain")
	static void UpdateMineTrainMovement(
		USplineComponent* SplineComp,
		double DeltaSeconds,
		double LaunchAcceleration,
		double TargetLaunchSpeed,
		double GravityScale,
		double MinSpeed,
		double BrakeDeceleration,
		UPARAM(ref) bool& bIsLaunching,
		UPARAM(ref) double& CurrentSpeed,
		UPARAM(ref) double& CurrentDistance,
		FVector& OutNewLocation,
		FRotator& OutNewRotation,
		bool& bOutIsFinished,
		bool& bOutIsBraking
	);
};
