// Fill out your copyright notice in the Description page of Project Settings.



#pragma once



#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "AGSDVibrationComponent.generated.h"



class USceneComponent;



/**

 * 진동 컴포넌트의 현재 가동 상태

 */

UENUM(BlueprintType)

enum class EAGSDVibrationState : uint8

{

	Stopped     UMETA(DisplayName = "정지"),

	RampingUp   UMETA(DisplayName = "시동 가속 중"),

	Running     UMETA(DisplayName = "정상 가동 중"),

	RampingDown UMETA(DisplayName = "정지 감속 중")

};



/**

 * 기계/오브젝트 진동 파형 및 가감속 설정 구조체

 * 코어 로직 수정 없이 수치 조합만으로 다양한 기계(발전기, 드릴, 엔진 등)의 떨림을 표현합니다.

 */

USTRUCT(BlueprintType)

struct FAGSDVibrationSettings

{

	GENERATED_BODY()



	/** X/Y/Z 축 상대 위치 떨림의 최대 진폭 (cm) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Amplitude")

	FVector PositionAmplitude = FVector(1.2f, 1.2f, 0.7f);



	/** Pitch/Yaw/Roll 미세 회전 흔들림의 최대 각도 (deg) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Amplitude")

	FRotator RotationAmplitude = FRotator(0.9f, 0.35f, 1.1f);



	/** 기본 엔진/모터 진동 주파수 (Hz) - 값이 클수록 빠르게 덜덜거립니다 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Frequency", meta = (ClampMin = "0.1", UIMin = "1.0", UIMax = "60.0"))

	float FrequencyHz = 26.0f;



	/** 주기적인 패턴 반복을 방지하기 위한 2차 고조파 주파수 비율 (비정수 비율 권장) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Frequency", meta = (ClampMin = "0.1", UIMin = "0.5", UIMax = "5.0"))

	float SecondaryFrequencyRatio = 1.73f;



	/** Perlin Noise 불규칙 떨림 혼합 비율 (0 = 순수 사인파, 1 = 노이즈 위주) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Frequency", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))

	float NoiseBlend = 0.35f;



	/** 간헐적으로 힘차게 덜컹거리는 엔진 맥동(진폭 변조) 활성화 여부 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Pulse")

	bool bEnableEnginePulse = true;



	/** 엔진 맥동 주파수 (Hz) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Pulse", meta = (EditCondition = "bEnableEnginePulse", ClampMin = "0.0", UIMin = "0.5", UIMax = "15.0"))

	float PulseFrequencyHz = 3.5f;



	/** 엔진 맥동에 따른 진폭 변화 폭 (0 ~ 0.8) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Pulse", meta = (EditCondition = "bEnableEnginePulse", ClampMin = "0.0", ClampMax = "0.8", UIMin = "0.0", UIMax = "0.8"))

	float PulseDepth = 0.25f;



	/** 전원 Activate 시 최대 진동까지 서서히 가속되는 시간 (초) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Ramp", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))

	float RampUpDuration = 1.5f;



	/** 전원 Deactivate 시 완전히 정지할 때까지 서서히 감속되는 시간 (초) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Ramp", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))

	float RampDownDuration = 1.0f;



	/** 가감속 커브 지수 (1.0 = 선형, 1.0보다 클수록 초반에 묵직하게 올라오는 시동 느낌) */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Ramp", meta = (ClampMin = "0.1", UIMin = "0.5", UIMax = "4.0"))

	float RampCurveExponent = 1.8f;

};



/**

 * 타겟 씬 컴포넌트와 초기 상대 트랜스폼을 보관하는 내부 관리 항목

 */

USTRUCT()

struct FAGSDVibrationTargetEntry

{

	GENERATED_BODY()



	UPROPERTY()

	TWeakObjectPtr<USceneComponent> TargetComponent = nullptr;



	UPROPERTY()

	FVector InitialRelativeLocation = FVector::ZeroVector;



	UPROPERTY()

	FRotator InitialRelativeRotation = FRotator::ZeroRotator;



	bool bHasSavedInitialTransform = false;

};



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAGSDVibrationStateChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAGSDVibrationIntensityChanged, float, CurrentIntensityAlpha);



/**

 * UAGSDVibrationComponent

 *

 * 발전기(BP_Generator_1), 기계 장치, 엔진 오브젝트 등에 부착하여 재사용할 수 있는 범용 진동 컴포넌트입니다.

 * - 루트/콜리전은 고정한 채 지정된 메시 컴포넌트만 초기 상대 좌표 기준으로 덜덜덜 흔들어 물리/상호작용 오동작을 방지합니다.

 * - 시동 가속(Ramp-Up) 및 정지 감속(Ramp-Down)을 지원하며, 정지 시에는 Tick을 비활성화해 연산 비용이 0입니다.

 * - 정지 시 캐싱된 초기 위치/회전으로 완벽히 복원하여 부동소수점 오차 누적을 방지합니다.

 */

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))

class SHARDSOFVEYARA_API UAGSDVibrationComponent : public UActorComponent

{

	GENERATED_BODY()



public:

	UAGSDVibrationComponent();



protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



public:

	// ── 진동 설정 ──



	/** 진동 진폭, 주파수, 시동 가감속 등을 정의하는 설정값 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Settings")

	FAGSDVibrationSettings Settings;



	/**

	 * 진동시킬 대상 컴포넌트 이름 목록입니다.

	 * 비워두면 BeginPlay 또는 Activate 시 루트(Root)가 아닌 첫 번째 스태틱 메시(없으면 일반 메시)를 자동으로 찾아 바인딩합니다.

	 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Target")

	TArray<FName> TargetComponentNames;



	// ── 라이프사이클 및 최적화 ──



	/** BeginPlay 직후 자동으로 진동을 시작할지 여부 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Lifecycle")

	bool bAutoPlayOnBeginPlay = false;



	/** 시작 시 랜덤 위상 오프셋을 부여하여 같은 기계를 여러 대 놓아도 똑같이 움직이지 않도록 분산합니다 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Lifecycle")

	bool bRandomizePhaseOnStart = true;



	/**

	 * 플레이어 카메라/폰과의 최대 활성화 거리(cm).

	 * 이 거리를 초과하면 트랜스폼 갱신을 잠시 멈추고 원점을 유지합니다. (0 이하 입력 시 거리 제한 없이 항상 연산)

	 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vibration|Optimization", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20000.0"))

	float MaxActiveDistance = 5000.0f;



	// ── 이벤트 델리게이트 (사운드 / VFX / 머티리얼 연동용) ──



	/** 진동이 시작될 때(Activate 호출 시) 발생합니다 */

	UPROPERTY(BlueprintAssignable, Category = "Vibration|Events")

	FOnAGSDVibrationStateChanged OnVibrationStarted;



	/** 시동 가속이 끝나고 최대 출력(Running)에 도달했을 때 발생합니다 */

	UPROPERTY(BlueprintAssignable, Category = "Vibration|Events")

	FOnAGSDVibrationStateChanged OnVibrationFullSpeed;



	/** 감속이 끝나고 진동이 완전히 정지했을 때 발생합니다 */

	UPROPERTY(BlueprintAssignable, Category = "Vibration|Events")

	FOnAGSDVibrationStateChanged OnVibrationStopped;



	/** 진동 강도(0.0 ~ 1.0)가 변하는 가감속 구간 동안 매 프레임 호출됩니다 (사운드 볼륨/피치, Emissive 연동용) */

	UPROPERTY(BlueprintAssignable, Category = "Vibration|Events")

	FOnAGSDVibrationIntensityChanged OnVibrationIntensityChanged;



	// ── 블루프린트 제어 API ──



	/**

	 * 진동을 가동합니다.

	 * @param bInstant true면 시동 가속(Ramp-Up) 없이 즉시 최대 세기로 떨립니다.

	 */

	UFUNCTION(BlueprintCallable, Category = "Vibration")

	void ActivateVibration(bool bInstant = false);



	/**

	 * 진동을 정지하고 타겟을 초기 위치/회전으로 복원합니다.

	 * @param bInstant true면 감속(Ramp-Down) 없이 즉시 멈추고 원점으로 돌아옵니다.

	 */

	UFUNCTION(BlueprintCallable, Category = "Vibration")

	void DeactivateVibration(bool bInstant = false);



	/**

	 * 현재 가동 여부에 따라 진동을 켜거나 끕니다.

	 * @param bInstant true면 가감속 없이 즉시 전환합니다.

	 */

	UFUNCTION(BlueprintCallable, Category = "Vibration")

	void ToggleVibration(bool bInstant = false);



	/**

	 * 진동시킬 단일 타겟 컴포넌트를 코드/블루프린트에서 직접 지정합니다. (기존 타겟 목록은 원점 복구 후 교체됩니다)

	 */

	UFUNCTION(BlueprintCallable, Category = "Vibration|Target")

	void SetTargetComponent(USceneComponent* NewTarget);



	/**

	 * 진동시킬 타겟 컴포넌트를 추가 등록합니다. (중복 등록은 무시됩니다)

	 */

	UFUNCTION(BlueprintCallable, Category = "Vibration|Target")

	void AddTargetComponent(USceneComponent* AdditionalTarget);



	/**

	 * 등록된 모든 타겟을 초기 위치로 복구한 뒤 타겟 목록을 비웁니다.

	 */

	UFUNCTION(BlueprintCallable, Category = "Vibration|Target")

	void ClearTargetComponents();



	/**

	 * 등록된 모든 타겟을 캐싱된 초기 상대 위치/회전으로 즉시 되돌립니다.

	 */

	UFUNCTION(BlueprintCallable, Category = "Vibration|Target")

	void ResetTargetsToInitialTransform();



	// ── 상태 조회 API ──



	/** 현재 진동 상태 머신 값을 반환합니다 */

	UFUNCTION(BlueprintPure, Category = "Vibration")

	EAGSDVibrationState GetVibrationState() const { return CurrentState; }



	/** 현재 진동이 가동 중(시동 중 포함)인지 여부를 반환합니다 */

	UFUNCTION(BlueprintPure, Category = "Vibration")

	bool IsVibrating() const

	{

		return CurrentState == EAGSDVibrationState::RampingUp || CurrentState == EAGSDVibrationState::Running;

	}



	/** 커브가 적용된 현재 진동 강도(0.0 ~ 1.0)를 반환합니다 */

	UFUNCTION(BlueprintPure, Category = "Vibration")

	float GetCurrentIntensityAlpha() const;



private:

	/** 선형 가감속 진행률(0.0 ~ 1.0) */

	float LinearRampProgress = 0.0f;



	/** 직전에 브로드캐스트한 강도 값 (중복 호출 방지용) */

	float LastBroadcastIntensityAlpha = -1.0f;



	/** 장시간 가동 시에도 삼각함수 정밀도를 유지하기 위한 double 누적 시간 */

	double AccumulatedTime = 0.0;



	/** 다중 장치 위상 분산용 시간 오프셋 */

	double PhaseOffsetSeconds = 0.0;



	/** 현재 진동 상태 */

	EAGSDVibrationState CurrentState = EAGSDVibrationState::Stopped;



	/** 거리 이탈로 인해 임시 휴면 상태에 들어갔는지 여부 */

	bool bSuspendedByDistance = false;



	/** 관리 중인 타겟 컴포넌트 및 초기 트랜스폼 캐시 목록 */

	UPROPERTY(Transient)

	TArray<FAGSDVibrationTargetEntry> TargetEntries;



private:

	/** 타겟 목록이 비어 있을 경우 이름 지정 또는 자동 탐색을 통해 타겟을 바인딩합니다 */

	void ResolveDefaultTargetsIfNeeded();



	/** 개별 타겟 컴포넌트를 초기 트랜스폼과 함께 안전하게 등록합니다 */

	bool RegisterTargetInternal(USceneComponent* Target);



	/** 델타 타임에 따라 가감속 상태 머신과 진행률을 갱신합니다 */

	void UpdateRampState(float DeltaTime);



	/** 현재 강도 알파가 변했으면 OnVibrationIntensityChanged를 브로드캐스트합니다 */

	void BroadcastIntensityIfChanged();



	/** 지정된 시간과 강도에 해당하는 위치/회전 오프셋을 순수 계산합니다 */

	void EvaluateVibrationOffset(double TimeSeconds, float IntensityAlpha, FVector& OutLocationOffset, FRotator& OutRotationOffset) const;



	/** 1개 축에 대한 다중 주파수 + Perlin 노이즈 합성 신호(-1.0 ~ 1.0 근처)를 계산합니다 */

	float ComputeAxisSignal(double TimeSeconds, float AxisPhaseOffset, float NoiseSeedOffset) const;



	/** 계산된 오프셋을 모든 유효 타겟의 초기 상대 좌표에 안전하게 적용합니다 */

	void ApplyOffsetsToTargets(const FVector& LocationOffset, const FRotator& RotationOffset);



	/** 최적화 거리 조건 이내에 플레이어 카메라/폰이 존재하는지 검사합니다 */

	bool IsWithinActiveDistance() const;



	/** 정지 상태로 전환하고 원점 복구 및 Tick 비활성화를 수행합니다 */

	void EnterStoppedState();

};



