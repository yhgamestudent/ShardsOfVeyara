#include "TutorialScreenMarkerWidget.h"
#include "TutorialSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

UTutorialScreenMarkerWidget::UTutorialScreenMarkerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsTargetOnScreen = false;
	ArrowAngle = 0.0f;
	TargetDistanceMeters = 0.0f;
	bIsMarkerVisible = false;
}

void UTutorialScreenMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 루트 자체는 UMG 렌더링 최적화로 인해 NativeTick이 중단(Culling)되지 않도록 항상 SelfHitTestInvisible로 유지
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 초기에는 마커 그래픽 요소를 숨김 상태로 시작
	SetMarkerElementsVisibility(false);

	// 튜토리얼 서브시스템 델리게이트 이벤트 바인딩
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>())
		{
			TutSub->OnTutorialStepStarted.AddUniqueDynamic(this, &UTutorialScreenMarkerWidget::HandleTutorialStepStarted);
			TutSub->OnTutorialSequenceCompleted.AddUniqueDynamic(this, &UTutorialScreenMarkerWidget::HandleTutorialSequenceCompleted);
		}
	}
}

void UTutorialScreenMarkerWidget::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>())
		{
			TutSub->OnTutorialStepStarted.RemoveAll(this);
			TutSub->OnTutorialSequenceCompleted.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void UTutorialScreenMarkerWidget::SetMarkerElementsVisibility(bool bVisible)
{
	bIsMarkerVisible = bVisible;
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;

	if (MarkerContainer)
	{
		MarkerContainer->SetVisibility(TargetVisibility);
	}
	else
	{
		SetVisibility(TargetVisibility);
	}
}

void UTutorialScreenMarkerWidget::HandleTutorialStepStarted(const FTutorialStepData& StepData)
{
	UpdateMarkerPosition();
}

void UTutorialScreenMarkerWidget::HandleTutorialSequenceCompleted(FName SequenceName)
{
	SetMarkerElementsVisibility(false);
}

void UTutorialScreenMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateMarkerPosition();
}

void UTutorialScreenMarkerWidget::SetTargetActor(AActor* InTargetActor)
{
	TargetActor = InTargetActor;
	bHasExplicitLocation = false;
}

void UTutorialScreenMarkerWidget::SetTargetLocation(const FVector& InTargetLocation)
{
	ExplicitTargetLocation = InTargetLocation;
	bHasExplicitLocation = true;
	TargetActor = nullptr;
}

FVector UTutorialScreenMarkerWidget::CalculateActorTargetLocation(const AActor* InActor) const
{
	if (!IsValid(InActor))
	{
		return FVector::ZeroVector;
	}

	if (bAutoAdjustToActorBounds)
	{
		FVector Origin = FVector::ZeroVector;
		FVector BoxExtent = FVector::ZeroVector;

		// 1차 시도: 콜리전 컴포넌트 기준 바운즈 측정
		InActor->GetActorBounds(true, Origin, BoxExtent);

		// 콜리전이 없거나 측정되지 않는 경우 비주얼(렌더링) 컴포넌트 포함 전체 측정
		if (BoxExtent.IsNearlyZero())
		{
			InActor->GetActorBounds(false, Origin, BoxExtent);
		}

		if (!BoxExtent.IsNearlyZero())
		{
			// 액터의 실제 꼭대기 상단 높이(Origin.Z + BoxExtent.Z) + 여백(TargetTopPadding)
			return FVector(Origin.X, Origin.Y, Origin.Z + BoxExtent.Z + TargetTopPadding);
		}
	}

	// 바운딩 박스를 구할 수 없거나 자동 조정 옵션이 꺼진 경우 기본 피벗 + WorldZOffset으로 폴백
	return InActor->GetActorLocation() + FVector(0.f, 0.f, WorldZOffset);
}

bool UTutorialScreenMarkerWidget::ResolveTargetWorldLocation(FVector& OutWorldLocation) const
{
	if (TargetActor.IsValid())
	{
		OutWorldLocation = CalculateActorTargetLocation(TargetActor.Get());
		return true;
	}

	if (bHasExplicitLocation)
	{
		OutWorldLocation = ExplicitTargetLocation;
		return true;
	}

	if (bAutoTrackTutorialSubsystem)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UTutorialSubsystem* TutSub = GI->GetSubsystem<UTutorialSubsystem>())
			{
				if (TutSub->IsTutorialActive())
				{
					// 서브시스템에서 현재 웨이포인트 액터가 유효하면 액터의 바운딩 박스 상단 자동 계산
					if (AActor* WaypointActor = TutSub->GetCurrentWaypointActor())
					{
						OutWorldLocation = CalculateActorTargetLocation(WaypointActor);
						return true;
					}

					// 액터 참조 없이 명시적 월드 좌표만 설정된 경우
					FVector Target = TutSub->GetCurrentTargetLocation();
					if (!Target.IsNearlyZero())
					{
						OutWorldLocation = Target + FVector(0.f, 0.f, WorldZOffset);
						return true;
					}
				}
			}
		}
	}

	return false;
}

void UTutorialScreenMarkerWidget::UpdateMarkerPosition()
{
	FVector TargetWorldLoc;
	if (!ResolveTargetWorldLocation(TargetWorldLoc))
	{
		SetMarkerElementsVisibility(false);
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	APlayerCameraManager* CamMgr = PC->PlayerCameraManager;
	if (!CamMgr)
	{
		return;
	}

	FVector CamLoc = CamMgr->GetCameraLocation();
	FRotator CamRot = CamMgr->GetCameraRotation();

	// 거리 계산 (카메라가 아닌 플레이어 캐릭터 기준 측정, 폰이 없을 경우 카메라로 폴백)
	FVector MeasureBaseLoc = PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : CamLoc;
	float DistCm = FVector::Dist(MeasureBaseLoc, TargetWorldLoc);
	TargetDistanceMeters = DistCm / 100.0f;

	// 근접 시 자동 숨김 처리
	if (HideNearDistanceThreshold > 0.0f && DistCm <= HideNearDistanceThreshold)
	{
		SetMarkerElementsVisibility(false);
		return;
	}

	// 뷰포트 크기
	int32 ViewportX = 0, ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		SetMarkerElementsVisibility(false);
		return;
	}

	FVector2D ViewportSize(ViewportX, ViewportY);
	FVector2D ViewportCenter = ViewportSize * 0.5f;

	// 카메라 로컬 축 계산 (전방, 우측, 상단)
	FRotationMatrix CamMatrix(CamRot);
	FVector CamForward = CamMatrix.GetUnitAxis(EAxis::X);
	FVector CamRight = CamMatrix.GetUnitAxis(EAxis::Y);
	FVector CamUp = CamMatrix.GetUnitAxis(EAxis::Z);

	FVector DirToTarget = (TargetWorldLoc - CamLoc).GetSafeNormal();
	float ForwardDot = FVector::DotProduct(CamForward, DirToTarget);
	float RightDot = FVector::DotProduct(CamRight, DirToTarget);
	float UpDot = FVector::DotProduct(CamUp, DirToTarget);

	bool bBehindCamera = (ForwardDot <= 0.0f);

	// 1. 카메라 전방일 경우에만 화면 2D 투영 시도
	FVector2D ProjectedPos = FVector2D::ZeroVector;
	bool bProjectSuccess = false;
	if (!bBehindCamera)
	{
		bProjectSuccess = PC->ProjectWorldLocationToScreen(TargetWorldLoc, ProjectedPos, false);
	}

	// 화면 내부(Frustum) 존재 여부 판정
	bool bInsideFrustum = !bBehindCamera && bProjectSuccess &&
		(ProjectedPos.X >= EdgePadding) && (ProjectedPos.X <= (ViewportSize.X - EdgePadding)) &&
		(ProjectedPos.Y >= EdgePadding) && (ProjectedPos.Y <= (ViewportSize.Y - EdgePadding));

	bIsTargetOnScreen = bInsideFrustum;

	if (bInsideFrustum)
	{
		CurrentScreenPosition = ProjectedPos;
		ArrowAngle = 180.0f; // 화면 내에서는 기본 이미지가 위쪽이므로 180도 회전하여 아래쪽 핀 방향으로 표시
	}
	else
	{
		// 2. 화면 밖 또는 등 뒤에 있을 때: 투영 행렬 왜곡(대각선 쏠림)을 방지하기 위해 카메라 로컬 축 기반으로 2D 방향 벡터 계산
		FVector2D Dir2D;
		if (bBehindCamera)
		{
			// 등 뒤에 있을 때: RightDot이 양수면 우측, 음수면 좌측.
			// 정뒤(ForwardDot = -1.0)에 가까울수록 화면 하단(+Y)으로 100% 수렴
			Dir2D.X = RightDot;
			Dir2D.Y = -UpDot - ForwardDot; // -ForwardDot은 양수가 되어 화면 아래쪽으로 유도
		}
		else
		{
			// 전방이지만 시야각 테두리를 벗어난 경우
			Dir2D.X = RightDot;
			Dir2D.Y = -UpDot; // 화면 좌표계는 아래가 +Y이므로 UpDot 반전
		}

		if (Dir2D.IsNearlyZero())
		{
			Dir2D = FVector2D(0.f, 1.f); // 기본 아래쪽
		}
		else
		{
			Dir2D.Normalize();
		}

		// 3. 화면 테두리 사각형(Bounding Box)과 Dir2D의 교차점 계산
		float HalfW = FMath::Max(10.0f, (ViewportSize.X * 0.5f) - EdgePadding);
		float HalfH = FMath::Max(10.0f, (ViewportSize.Y * 0.5f) - EdgePadding);

		float ScaleX = (Dir2D.X != 0.0f) ? FMath::Abs(HalfW / Dir2D.X) : 9999.0f;
		float ScaleY = (Dir2D.Y != 0.0f) ? FMath::Abs(HalfH / Dir2D.Y) : 9999.0f;
		float MinScale = FMath::Min(ScaleX, ScaleY);

		CurrentScreenPosition = ViewportCenter + (Dir2D * MinScale);
		// 화면 가장자리에서 타겟 방향을 가리키는 각도 + 기본 이미지 오프셋(90도)
		ArrowAngle = FMath::RadiansToDegrees(FMath::Atan2(Dir2D.Y, Dir2D.X)) + ArrowBaseRotationOffset;
	}

	// 마커 그래픽 요소 노출 상태 갱신
	SetMarkerElementsVisibility(true);

	// 1. 컨테이너 위치 갱신 (DPI 스케일 보정)
	if (MarkerContainer)
	{
		float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
		FVector2D WidgetPos = (DPIScale > 0.0f) ? (CurrentScreenPosition / DPIScale) : CurrentScreenPosition;

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MarkerContainer->Slot))
		{
			CanvasSlot->SetPosition(WidgetPos);
		}
	}

	// 2. 화살표 회전 및 가시성 갱신
	if (ArrowWidget)
	{
		if (bHideArrowWhenOnScreen && bIsTargetOnScreen)
		{
			ArrowWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			ArrowWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ArrowWidget->SetRenderTransformAngle(ArrowAngle);
		}
	}

	// 3. 거리 텍스트 갱신
	if (DistanceTextBlock)
	{
		int32 IntDist = FMath::RoundToInt(TargetDistanceMeters);
		FText DistStr = FText::Format(NSLOCTEXT("Tutorial", "DistFormat", "{0}m"), FText::AsNumber(IntDist));
		DistanceTextBlock->SetText(DistStr);
	}

	// 블루프린트 커스텀 이벤트 통지
	OnMarkerStateUpdated(bIsTargetOnScreen, CurrentScreenPosition, ArrowAngle, TargetDistanceMeters);
}
