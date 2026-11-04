// Fill out your copyright notice in the Description page of Project Settings.

#include "AGSDMineTrainLibrary.h"
#include "Camera/CameraComponent.h"

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

void UAGSDMineTrainLibrary::UpdateMineTrainCameraEffects(
	UCameraComponent* CameraComp,
	USceneComponent* ParentComp,
	double CurrentSpeed,
	double DeltaSeconds,
	double MinSpeed,
	double BaseSpeed,
	double MaxSpeed,
	float MinFOV,
	float BaseFOV,
	float MaxSpeedFOV,
	float FOVInterpSpeed,
	float RotationLagSpeed,
	float CameraPitchAngle,
	bool bEnableRotationLag)
{
	if (!IsValid(CameraComp) || DeltaSeconds <= 0.0)
	{
		return;
	}

	// 1. 3단계 동적 FOV 계산 (감속 줌인 ↔ 순항 기본 ↔ 가속 광각 줌아웃)
	float TargetFOV = BaseFOV;
	if (CurrentSpeed < BaseSpeed)
	{
		const double SpeedRange = BaseSpeed - MinSpeed;
		const float SpeedAlpha = (SpeedRange > 0.0)
			? static_cast<float>(FMath::Clamp((CurrentSpeed - MinSpeed) / SpeedRange, 0.0, 1.0))
			: 1.0f;
		TargetFOV = FMath::Lerp(MinFOV, BaseFOV, SpeedAlpha);
	}
	else
	{
		const double SpeedRange = MaxSpeed - BaseSpeed;
		const float SpeedAlpha = (SpeedRange > 0.0)
			? static_cast<float>(FMath::Clamp((CurrentSpeed - BaseSpeed) / SpeedRange, 0.0, 1.0))
			: 0.0f;
		TargetFOV = FMath::Lerp(BaseFOV, MaxSpeedFOV, SpeedAlpha);
	}

	const float CurrentFOV = CameraComp->FieldOfView;
	const float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, static_cast<float>(DeltaSeconds), FOVInterpSpeed);
	CameraComp->SetFieldOfView(NewFOV);

	// 2. 카트 회전 관성 지연 (Camera Rotation Lag)
	if (bEnableRotationLag && IsValid(ParentComp))
	{
		if (!CameraComp->IsUsingAbsoluteRotation())
		{
			CameraComp->SetUsingAbsoluteRotation(true);
		}

		// 부모 컴포넌트(카트)의 월드 쿼터니언에 카메라 기본 Pitch 오프셋을 결합하여 목표 월드 회전 산출
		const FQuat ParentQuat = ParentComp->GetComponentQuat();
		const FQuat LocalPitchQuat = FRotator(CameraPitchAngle, 0.0f, 0.0f).Quaternion();
		const FRotator TargetRotation = (ParentQuat * LocalPitchQuat).Rotator();

		const FRotator CurrentRotation = CameraComp->GetComponentRotation();
		const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, static_cast<float>(DeltaSeconds), RotationLagSpeed);
		CameraComp->SetWorldRotation(NewRotation);
	}
}
