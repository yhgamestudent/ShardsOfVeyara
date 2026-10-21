// Fill out your copyright notice in the Description page of Project Settings.

#include "AGSDMineTrainLibrary.h"

void UAGSDMineTrainLibrary::UpdateMineTrainMovement(
	USplineComponent* SplineComp,
	double DeltaSeconds,
	double LaunchAcceleration,
	double TargetLaunchSpeed,
	double GravityScale,
	double MinSpeed,
	double BrakeDeceleration,
	bool& bIsLaunching,
	double& CurrentSpeed,
	double& CurrentDistance,
	FVector& OutNewLocation,
	FRotator& OutNewRotation,
	bool& bOutIsFinished,
	bool& bOutIsBraking)
{
	bOutIsFinished = false;
	bOutIsBraking = false;
	OutNewLocation = FVector::ZeroVector;
	OutNewRotation = FRotator::ZeroRotator;

	if (!IsValid(SplineComp))
	{
		return;
	}

	const double SplineLength = static_cast<double>(SplineComp->GetSplineLength());
	const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();

	// 1. 마지막 스플라인 포인트 전 라인(감속 구간) 시작 거리 계산
	// 예: 포인트가 5개(0, 1, 2, 3, 4)라면 마지막 포인트(4) 전 라인의 시작은 3번 포인트 (NumPoints - 2)
	const int32 SlowdownPointIndex = FMath::Max(0, NumPoints - 2);
	const double SlowdownStartDistance = static_cast<double>(SplineComp->GetDistanceAlongSplineAtSplinePoint(SlowdownPointIndex));

	// 2. 현재 상태에 따른 속도 업데이트
	if (bIsLaunching)
	{
		// 런칭 가속 모드
		CurrentSpeed += LaunchAcceleration * DeltaSeconds;
		if (CurrentSpeed >= TargetLaunchSpeed)
		{
			CurrentSpeed = TargetLaunchSpeed;
			bIsLaunching = false;
		}
	}
	else if (CurrentDistance >= SlowdownStartDistance)
	{
		// 마지막 포인트 전 라인 진입: 감속(Brake) 처리
		bOutIsBraking = true;

		if (BrakeDeceleration > 0.0)
		{
			// 선형 감속도 적용
			CurrentSpeed = FMath::Max(CurrentSpeed - (BrakeDeceleration * DeltaSeconds), MinSpeed);
		}
		else
		{
			// 0 이하 입력 시 부드러운 지수 보간 감속
			CurrentSpeed = FMath::FInterpTo(CurrentSpeed, MinSpeed, DeltaSeconds, 3.0);
		}
	}
	else
	{
		// 일반 주행 모드 (경사로 중력 가속)
		const FVector Direction = SplineComp->GetDirectionAtDistanceAlongSpline(static_cast<float>(CurrentDistance), ESplineCoordinateSpace::World);
		const double GravityDelta = (Direction.Z * GravityScale * -1.0) * DeltaSeconds;

		CurrentSpeed = FMath::Max(CurrentSpeed + GravityDelta, MinSpeed);
	}

	// 3. 이동 거리 누적
	CurrentDistance += CurrentSpeed * DeltaSeconds;

	// 4. 종점 도착 판정
	if (CurrentDistance >= SplineLength)
	{
		CurrentDistance = SplineLength;
		bOutIsFinished = true;
	}

	// 5. 스플라인 상의 새 위치와 회전값 계산
	OutNewLocation = SplineComp->GetLocationAtDistanceAlongSpline(static_cast<float>(CurrentDistance), ESplineCoordinateSpace::World);
	OutNewRotation = SplineComp->GetRotationAtDistanceAlongSpline(static_cast<float>(CurrentDistance), ESplineCoordinateSpace::World);
}
