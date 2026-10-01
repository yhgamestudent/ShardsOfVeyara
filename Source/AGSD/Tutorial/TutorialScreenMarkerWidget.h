#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialTypes.h"
#include "TutorialScreenMarkerWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * 튜토리얼 목표 웨이포인트의 3D 월드 위치를 2D 화면에 투영하고,
 * 화면 밖으로 벗어났을 때 화면 가장자리에 클램핑 및 회전 화살표와 거리를 표시하는 위젯 베이스 클래스입니다.
 */
UCLASS()
class AGSD_API UTutorialScreenMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTutorialScreenMarkerWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 마커 하위 요소들의 가시성 제어 (위젯 루트 틱 유지를 위해 MarkerContainer 우선 토글) */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|ScreenMarker")
	void SetMarkerElementsVisibility(bool bVisible);

	UFUNCTION()
	void HandleTutorialStepStarted(const FTutorialStepData& StepData);

	UFUNCTION()
	void HandleTutorialSequenceCompleted(FName SequenceName);

public:
	/**
	 * 마커가 가리킬 목표 액터를 직접 지정합니다.
	 * @param InTargetActor 추적할 월드 액터
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|ScreenMarker")
	void SetTargetActor(AActor* InTargetActor);

	/**
	 * 마커가 가리킬 목표 월드 좌표를 직접 지정합니다.
	 * @param InTargetLocation 추적할 3D 월드 좌표
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|ScreenMarker")
	void SetTargetLocation(const FVector& InTargetLocation);

	/** 튜토리얼 서브시스템 자동 추적 모드 토글 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|ScreenMarker")
	void SetAutoTrackTutorialSubsystem(bool bEnable) { bAutoTrackTutorialSubsystem = bEnable; }

	/** 마커 상태 업데이트 시 블루프린트에서 추가 비주얼 연출을 입힐 수 있는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|ScreenMarker")
	void OnMarkerStateUpdated(bool bOnScreen, FVector2D ScreenPos, float InArrowAngle, float DistanceMeters);

	// --- UMG 바인딩 위젯 (선택 사항: UMG 계층 구조에 같은 이름의 위젯이 있으면 자동 연동) ---

	/** 위치를 이동시킬 마커 루트 컨테이너 (CanvasPanelSlot을 가진 위젯 권장) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|ScreenMarker")
	TObjectPtr<UWidget> MarkerContainer;

	/** 방향을 가리킬 회전 화살표 위젯 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|ScreenMarker")
	TObjectPtr<UWidget> ArrowWidget;

	/** 잔여 거리를 출력할 텍스트 블록 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial|ScreenMarker")
	TObjectPtr<UTextBlock> DistanceTextBlock;

	// --- 설정 파라미터 ---

	/** 화면 테두리 클램핑 여백 (픽셀 단위, 기본 60px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker")
	float EdgePadding = 60.0f;

	/** 타겟 액터의 원점 대비 높이 오프셋 (cm 단위, 바운딩 박스 미사용 또는 폴백 시 머리 위 높이) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker")
	float WorldZOffset = 120.0f;

	/** 타겟 액터의 바운딩 박스를 계산하여 실제 물체 상단(꼭대기) 높이를 자동으로 맞출지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker")
	bool bAutoAdjustToActorBounds = true;

	/** 바운딩 박스 상단 위에 띄울 여유 여백 (cm 단위, 기본 30cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker", meta = (EditCondition = "bAutoAdjustToActorBounds"))
	float TargetTopPadding = 30.0f;

	/** UTutorialSubsystem의 목표 위치를 자동으로 가져와 추적할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker")
	bool bAutoTrackTutorialSubsystem = true;

	/** 타겟이 화면 내에 완전히 들어왔을 때 회전 화살표를 숨길지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker")
	bool bHideArrowWhenOnScreen = true;

	/** 타겟이 이 거리(cm)보다 가까워지면 마커를 자동으로 숨김 (0이면 항상 표시, 기본값: 0.0f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker")
	float HideNearDistanceThreshold = 0.0f;

	/** 화살표 기본 텍스처/이미지의 방향 보정 오프셋 (기본 90도: 위쪽(Up)을 가리키는 이미지 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|ScreenMarker")
	float ArrowBaseRotationOffset = 90.0f;

	// --- 실시간 상태 값 (블루프린트 조회용) ---

	/** 타겟이 현재 카메라 시야(화면 내부)에 있는지 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|ScreenMarker")
	bool bIsTargetOnScreen = false;

	/** 클램핑이 적용된 최종 2D 뷰포트 화면 좌표 */
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|ScreenMarker")
	FVector2D CurrentScreenPosition = FVector2D::ZeroVector;

	/** 화면 테두리에서 타겟 방향을 가리키는 회전 각도 (0~360도) */
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|ScreenMarker")
	float ArrowAngle = 0.0f;

	/** 플레이어와 타겟 간의 미터(m) 단위 거리 */
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|ScreenMarker")
	float TargetDistanceMeters = 0.0f;

	/** 마커가 현재 활성화되어 화면에 노출되어야 하는지 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial|ScreenMarker")
	bool bIsMarkerVisible = false;

protected:
	/** 매 프레임 위치 및 회전 계산 */
	virtual void UpdateMarkerPosition();

	/** 액터로부터 목표 3D 월드 좌표 계산 (바운딩 박스 상단 계산 포함) */
	FVector CalculateActorTargetLocation(const AActor* InActor) const;

	/** 목표 3D 월드 좌표 계산 */
	bool ResolveTargetWorldLocation(FVector& OutWorldLocation) const;

private:
	TWeakObjectPtr<AActor> TargetActor = nullptr;
	FVector ExplicitTargetLocation = FVector::ZeroVector;
	bool bHasExplicitLocation = false;
};
