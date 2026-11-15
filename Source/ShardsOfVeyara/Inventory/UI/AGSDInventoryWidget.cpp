// AGSDInventoryWidget.cpp - 가방 인벤토리 UI 위젯 구현

#include "AGSDInventoryWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "AGSDInventorySlotWidget.h"

void UAGSDInventoryWidget::InitializeInventory(UAGSDInventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent) return;

	InventoryComponent = InInventoryComponent;

	// 델리게이트 바인딩 (중복 등록 방지를 위해 먼저 해제)
	InventoryComponent->OnInventorySlotUpdated.RemoveAll(this);
	InventoryComponent->OnInventorySlotUpdated.AddDynamic(this, &UAGSDInventoryWidget::OnInventorySlotUpdated);

	InventoryComponent->OnInventoryFullyUpdated.RemoveAll(this);
	InventoryComponent->OnInventoryFullyUpdated.AddDynamic(this, &UAGSDInventoryWidget::OnInventoryFullyUpdated);

	// 그리드 패널 초기화
	if (UGP_InventorySlots)
	{
		UGP_InventorySlots->ClearChildren();
	}
	InventorySlotWidgets.Empty();

	// 가방 영역(10번 슬롯 이후) 생성 및 그리드 배치
	if (SlotWidgetClass)
	{
		const int32 MaxSlots = InventoryComponent->GetMaxSlots();
		const int32 SlotStart = StartSlotIndex;

		for (int32 i = SlotStart; i < MaxSlots; ++i)
		{
			UAGSDInventorySlotWidget* NewSlot = CreateWidget<UAGSDInventorySlotWidget>(this, SlotWidgetClass);
			if (NewSlot)
			{
				NewSlot->SlotIndex = i;
				NewSlot->OwningInventoryComponent = InventoryComponent;

				// 백엔드 데이터 동기화
				FStruct_InventorySlotData SlotData = InventoryComponent->GetSlotData(i);
				NewSlot->SetInventoryItem(SlotData.ItemData, SlotData.IsEmpty);

				// 행/열 좌표 계산 (SlotsPerRow가 0 이하로 들어오지 않도록 방어)
				const int32 EffectiveSlotsPerRow = FMath::Max(1, SlotsPerRow);
				const int32 RelIndex = i - SlotStart;
				const int32 Row = RelIndex / EffectiveSlotsPerRow;
				const int32 Col = RelIndex % EffectiveSlotsPerRow;

				if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(UGP_InventorySlots->AddChildToUniformGrid(NewSlot, Row, Col)))
				{
					GridSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
					GridSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
				}
				InventorySlotWidgets.Add(NewSlot);
			}
		}
	}
}

void UAGSDInventoryWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// 에디터 프로퍼티 변경 시 SlotsPerRow 최소값(1) 안전 보장
	SlotsPerRow = FMath::Max(1, SlotsPerRow);

#if WITH_EDITOR
	// 에디터 UMG 디자이너 창에서 SlotsPerRow 프로퍼티 변경 시 실시간 미리보기 갱신
	if (IsDesignTime() && UGP_InventorySlots && SlotWidgetClass)
	{
		UGP_InventorySlots->ClearChildren();

		const int32 PreviewSlotCount = 10;
		const int32 EffectiveSlotsPerRow = FMath::Max(1, SlotsPerRow);

		for (int32 i = 0; i < PreviewSlotCount; ++i)
		{
			UAGSDInventorySlotWidget* NewSlot = CreateWidget<UAGSDInventorySlotWidget>(this, SlotWidgetClass);
			if (NewSlot)
			{
				const int32 Row = i / EffectiveSlotsPerRow;
				const int32 Col = i % EffectiveSlotsPerRow;

				if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(UGP_InventorySlots->AddChildToUniformGrid(NewSlot, Row, Col)))
				{
					GridSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
					GridSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
				}
			}
		}
	}
#endif
}

void UAGSDInventoryWidget::RefreshInventory()
{
	if (!InventoryComponent) return;

	const int32 SlotStart = StartSlotIndex;

	for (int32 i = 0; i < InventorySlotWidgets.Num(); ++i)
	{
		const int32 SlotIndex = SlotStart + i;
		if (InventoryComponent->IsValidSlotIndex(SlotIndex))
		{
			FStruct_InventorySlotData SlotData = InventoryComponent->GetSlotData(SlotIndex);
			InventorySlotWidgets[i]->SetInventoryItem(SlotData.ItemData, SlotData.IsEmpty);
		}
	}
}

void UAGSDInventoryWidget::OnInventorySlotUpdated(int32 SlotIndex)
{
	if (!InventoryComponent) return;

	const int32 SlotStart = StartSlotIndex;

	// 업데이트된 슬롯이 가방 영역에 해당하는 경우에만 처리
	if (SlotIndex >= SlotStart && SlotIndex < InventoryComponent->GetMaxSlots())
	{
		const int32 WidgetIndex = SlotIndex - SlotStart;
		if (InventorySlotWidgets.IsValidIndex(WidgetIndex))
		{
			FStruct_InventorySlotData SlotData = InventoryComponent->GetSlotData(SlotIndex);
			InventorySlotWidgets[WidgetIndex]->SetInventoryItem(SlotData.ItemData, SlotData.IsEmpty);
		}
	}
}

void UAGSDInventoryWidget::OnInventoryFullyUpdated()
{
	RefreshInventory();
}

void UAGSDInventoryWidget::NativeDestruct()
{
	// 델리게이트 해제 (댕글링 콜백 방지)
	if (InventoryComponent)
	{
		InventoryComponent->OnInventorySlotUpdated.RemoveDynamic(this, &UAGSDInventoryWidget::OnInventorySlotUpdated);
		InventoryComponent->OnInventoryFullyUpdated.RemoveDynamic(this, &UAGSDInventoryWidget::OnInventoryFullyUpdated);
	}

	Super::NativeDestruct();
}
