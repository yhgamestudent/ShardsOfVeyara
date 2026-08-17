// AGSDSlotWidgetBase.cpp - 공통 슬롯 위젯 베이스 클래스 구현

#include "AGSDSlotWidgetBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "AGSDSlotDragDropOperation.h"
#include "AGSDDragVisualWidget.h"
#include "Input/Events.h"
#include "Struct_ItemData.h"
#include "AGSDCharacter.h"
#include "Chest.h"
#include "Framework/Application/SlateApplication.h"
#include "Inventory/UI/AGSDPlayerHUD.h"
#include "Inventory/UI/AGSDHotbarSlotWidget.h"
#include "Inventory/UI/AGSDItemTooltipWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Engine/Engine.h"

UAGSDSlotWidgetBase::UAGSDSlotWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UAGSDItemTooltipWidget> TooltipClassFinder(TEXT("/Game/HYH/Blueprints/Widgets/UIWidget/Inventory/WBP_AGSDItemTooltip"));
	if (TooltipClassFinder.Succeeded())
	{
		TooltipWidgetClass = TooltipClassFinder.Class;
	}
}

void UAGSDSlotWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateVisual();
}

void UAGSDSlotWidgetBase::NativeDestruct()
{
	if (ActiveTooltipInstance)
	{
		ActiveTooltipInstance->RemoveFromParent();
		ActiveTooltipInstance = nullptr;
	}

	Super::NativeDestruct();
}

void UAGSDSlotWidgetBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (UAGSDInventoryComponent* InvComp = GetInventoryComponent())
	{
		InvComp->HoveredSlotIndex = SlotIndex;
	}

	// 호버 시 설명 툴팁 동적 생성 로직
	if (!SlotItemData.IsEmpty && !SlotItemData.ItemData.ItemID.IsEmpty() && TooltipWidgetClass)
	{
		if (ActiveTooltipInstance)
		{
			ActiveTooltipInstance->RemoveFromParent();
			ActiveTooltipInstance = nullptr;
		}

		ActiveTooltipInstance = CreateWidget<UAGSDItemTooltipWidget>(this, TooltipWidgetClass);
		if (ActiveTooltipInstance)
		{
			ActiveTooltipInstance->SetTooltipData(SlotItemData.ItemData);
			ActiveTooltipInstance->AddToViewport(100); // 인벤토리보다는 위, 페이드 위젯보다는 아래로 배치 (ZOrder 100)

			// 1. 슬롯의 절대 상단 좌측 및 우측 좌표 계산 (LocalToAbsolute를 사용해 정확성 보장)
			FVector2D AbsoluteLeft = InGeometry.GetAbsolutePosition();
			FVector2D AbsoluteRight = InGeometry.LocalToAbsolute(FVector2D(InGeometry.GetLocalSize().X, 0.f));

			// 2. 각각을 뷰포트 좌표로 변환하여 DPI 스케일링 보정
			FVector2D ViewportLeftPixel, ViewportLeft;
			USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), AbsoluteLeft, ViewportLeftPixel, ViewportLeft);

			FVector2D ViewportRightPixel, ViewportRight;
			USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), AbsoluteRight, ViewportRightPixel, ViewportRight);

			float SlotViewportWidth = ViewportRight.X - ViewportLeft.X;

			// 3. 화면 크기 가져오기 및 DPI 스케일링 보정
			FVector2D ViewportSize = FVector2D::ZeroVector;
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->GetViewportSize(ViewportSize);
			}

			float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
			FVector2D TrueViewportSize = ViewportSize;
			if (ViewportScale > 0.0f)
			{
				TrueViewportSize = ViewportSize / ViewportScale;
			}

			if (TrueViewportSize.X > 0.f)
			{
				// 4. 툴팁 위젯의 크기(가로/세로)를 미리 계산
				ActiveTooltipInstance->ForceLayoutPrepass();
				FVector2D TooltipSize = ActiveTooltipInstance->GetDesiredSize();

				FVector2D TargetPosition;
				FVector2D Alignment;

				// 5. 기본적으로 우측에 툴팁 배치 시도
				float RightTargetX = ViewportRight.X + 10.f;

				// 우측 배치 시 툴팁이 화면 가로 해상도를 넘어가 잘리는지 검사
				if (RightTargetX + TooltipSize.X > TrueViewportSize.X)
				{
					// 공간이 부족하면 좌측 배치로 자동 반전 (Alignment X = 1.0f)
					TargetPosition.X = ViewportLeft.X - 10.f;
					Alignment.X = 1.f;
				}
				else
				{
					// 공간이 충분하면 우측 배치 유지 (Alignment X = 0.0f)
					TargetPosition.X = RightTargetX;
					Alignment.X = 0.f;
				}

				// 6. Y축 세로 방향 뷰포트 하단 잘림 감지 및 위로 밀기 보정 (Clamp)
				float SafetyPadding = 35.f; // 텍스트 자동 개행 등으로 인한 동적 세로 크기 증가 대비 안전 여백
				float Margin = 20.f; // 화면 가장자리와의 여유 마진
				float MaxAllowedY = TrueViewportSize.Y - (TooltipSize.Y + SafetyPadding) - Margin;

				// Y축 툴팁 기본 정렬은 슬롯 상단선에 맞춤
				TargetPosition.Y = FMath::Clamp(ViewportLeft.Y, Margin, FMath::Max(Margin, MaxAllowedY));
				Alignment.Y = 0.f; // 상단 기준 정렬 유지

				// bRemoveDPIScale = false 파라미터를 명시하여 이중 DPI 보정 현상 차단
				ActiveTooltipInstance->SetPositionInViewport(TargetPosition, false);
				ActiveTooltipInstance->SetAlignmentInViewport(Alignment);
			}
		}
	}
}

void UAGSDSlotWidgetBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (UAGSDInventoryComponent* InvComp = GetInventoryComponent())
	{
		// 다른 슬롯으로 바로 옮겨갔을 때 덮어씌워진 경우를 대비해 본인 인덱스일 때만 초기화
		if (InvComp->HoveredSlotIndex == SlotIndex)
		{
			InvComp->HoveredSlotIndex = -1;
		}
	}

	// 호버 해제 시 툴팁 인스턴스 파괴
	if (ActiveTooltipInstance)
	{
		ActiveTooltipInstance->RemoveFromParent();
		ActiveTooltipInstance = nullptr;
	}
}

void UAGSDSlotWidgetBase::SetItemData(FStruct_ItemData ItemData, bool bClear)
{
	// 데이터가 변경(교체/초기화)되면 기존 툴팁을 즉시 제거합니다.
	if (ActiveTooltipInstance)
	{
		ActiveTooltipInstance->RemoveFromParent();
		ActiveTooltipInstance = nullptr;
	}

	if (bClear)
	{
		SlotItemData.IsEmpty = true;
		SlotItemData.ItemData = FStruct_ItemData();
		SlotItemData.SlotIndex = SlotIndex;
	}
	else
	{
		SlotItemData.IsEmpty = false;
		SlotItemData.ItemData = ItemData;
		SlotItemData.SlotIndex = SlotIndex;
	}
	UpdateVisual();
}

void UAGSDSlotWidgetBase::UpdateVisual()
{
	// 슬롯 자체는 항상 보이도록 보장하여 빈 슬롯 상태에서도 양피지 배경이 나타나도록 합니다.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (SlotItemData.IsEmpty || SlotItemData.ItemData.ItemID.IsEmpty())
	{
		if (IMG_ItemIcon)
		{
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (TXT_ItemAmount)
		{
			TXT_ItemAmount->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		if (IMG_ItemIcon)
		{
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Visible);
			if (SlotItemData.ItemData.ItemIcon)
			{
				IMG_ItemIcon->SetBrushFromTexture(SlotItemData.ItemData.ItemIcon);
			}
		}

		if (TXT_ItemAmount)
		{
			// MaxQuantity가 1보다 크고, 현재 수량이 1개보다 많을 때만 수량 텍스트를 노출합니다.
			if (SlotItemData.ItemData.MaxQuantity > 1 && SlotItemData.ItemData.CurrentQuantity >= 1)
			{
				TXT_ItemAmount->SetVisibility(ESlateVisibility::Visible);
				TXT_ItemAmount->SetText(FText::AsNumber(SlotItemData.ItemData.CurrentQuantity));
			}
			else
			{
				TXT_ItemAmount->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

FReply UAGSDSlotWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !SlotItemData.IsEmpty)
	{
		// 드래그 디텍팅 트리거 설정 (더블클릭은 별도로 NativeOnMouseButtonDoubleClick에서 처리)
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return FReply::Unhandled();
}

FReply UAGSDSlotWidgetBase::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !SlotItemData.IsEmpty)
	{
		// 더블클릭 이벤트 델리게이트 브로드캐스트 호출 (상자 및 인벤토리 빠른 이동 연동용)
		OnSlotDoubleClicked.Broadcast(this);
		
		// C++ 자동 전송 로직 호출
		HandleSlotDoubleClicked();

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UAGSDSlotWidgetBase::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	// 드래그가 감지되면 화면에 떠 있는 기존 툴팁을 즉시 제거합니다.
	if (ActiveTooltipInstance)
	{
		ActiveTooltipInstance->RemoveFromParent();
		ActiveTooltipInstance = nullptr;
	}

	UAGSDSlotDragDropOperation* DragOp = NewObject<UAGSDSlotDragDropOperation>(this);
	if (DragOp)
	{
		DragOp->SourceSlotIndex = SlotIndex;
		DragOp->SourceSlotData = SlotItemData;
		DragOp->SourceWidget = this;
		DragOp->SourceInventoryComponent = GetInventoryComponent();
		
		// 인벤토리 컴포넌트 핫바 영역 상수 범위(0~9)로 핫바 출발 여부 판단
		DragOp->bFromHotbar = (SlotIndex >= 0 && SlotIndex <= 9);

		// 드래그 비주얼 생성 및 아이템 데이터 주입
		if (DragVisualClass)
		{
			// UAGSDDragVisualWidget으로 직접 생성하여 타입 안전하게 데이터를 전달합니다.
			if (UAGSDDragVisualWidget* DragVisual = CreateWidget<UAGSDDragVisualWidget>(this, DragVisualClass))
			{
				DragVisual->SetDragItemData(SlotItemData.ItemData);
				DragOp->DefaultDragVisual = DragVisual;
				// CenterCenter: 위젯 중심이 마우스 커서에 오도록 설정
				DragOp->Pivot = EDragPivot::CenterCenter;
			}
		}

		OutOperation = DragOp;
	}
}


void UAGSDSlotWidgetBase::NativeOnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDragEnter(MyGeometry, DragDropEvent, Operation);
}

void UAGSDSlotWidgetBase::NativeOnDragLeave(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDragLeave(DragDropEvent, Operation);
}

bool UAGSDSlotWidgetBase::NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation)
{
	Super::NativeOnDrop(MyGeometry, DragDropEvent, Operation);

	if (UAGSDSlotDragDropOperation* SlotOp = Cast<UAGSDSlotDragDropOperation>(Operation))
	{
		UAGSDInventoryComponent* TargetInvComp = GetInventoryComponent();
		UAGSDInventoryComponent* SourceInvComp = SlotOp->SourceInventoryComponent;

		if (!TargetInvComp || !SourceInvComp) return false;

		if (SourceInvComp == TargetInvComp)
		{
			// 같은 인벤토리 내 스왑
			TargetInvComp->SwapSlots(SlotOp->SourceSlotIndex, SlotIndex);
		}
		else
		{
			// 다른 인벤토리 간 교차 이동 (플레이어 ↔ 상자)
			UAGSDInventoryComponent::CrossInventorySwap(SourceInvComp, SlotOp->SourceSlotIndex, TargetInvComp, SlotIndex);
		}
		return true;
	}

	return false;
}

UAGSDInventoryComponent* UAGSDSlotWidgetBase::GetInventoryComponent() const
{
	// 소속 인벤토리 컴포넌트가 설정된 경우 우선 반환 (상자 슬롯 등)
	if (OwningInventoryComponent)
	{
		return OwningInventoryComponent;
	}

	// Fallback: 플레이어 폰의 인벤토리 컴포넌트
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return PlayerPawn->FindComponentByClass<UAGSDInventoryComponent>();
	}
	return nullptr;
}

void UAGSDSlotWidgetBase::HandleSlotDoubleClicked()
{
	// 드래그 중이라면 전송 무시
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().IsDragDropping())
	{
		return;
	}

	if (SlotItemData.IsEmpty || SlotItemData.ItemData.ItemID.IsEmpty()) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return;

	AAGSDCharacter* Character = Cast<AAGSDCharacter>(PlayerPawn);
	if (!Character) return;

	UAGSDInventoryComponent* PlayerInv = Character->InventoryComponent;
	UAGSDInventoryComponent* MyInv = GetInventoryComponent();

	if (!PlayerInv || !MyInv) return;

	// 1. 상자가 열려 있는 경우
	if (Character->OpenedChest)
	{
		UAGSDInventoryComponent* ChestInv = Character->OpenedChest->GetInventoryComponent();
		if (ChestInv)
		{
			if (MyInv == ChestInv)
			{
				// 상자 -> 플레이어 가방으로 이동
				UAGSDInventoryComponent::CrossInventoryTransfer(ChestInv, SlotIndex, PlayerInv);
			}
			else if (MyInv == PlayerInv)
			{
				// 플레이어 가방 -> 상자로 이동
				UAGSDInventoryComponent::CrossInventoryTransfer(PlayerInv, SlotIndex, ChestInv);
			}
		}
	}
	// 2. 상자가 없고 가방 인벤토리 UI만 열려 있는 경우
	else if (Character->PlayerHUDRef && Character->PlayerHUDRef->IsInventoryOpen())
	{
		if (MyInv == PlayerInv)
		{
			if (PlayerInv->IsHotbarSlot(SlotIndex))
			{
				// 핫바 -> 가방 영역으로 자동 이동
				PlayerInv->MoveHotbarToBag(SlotIndex);
			}
			else
			{
				// 가방 영역 -> 핫바로 자동 이동
				PlayerInv->MoveBagToHotbar(SlotIndex);
			}
		}
	}
}
