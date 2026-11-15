#include "AGSDSlotDragDropOperation.h"
#include "AGSDCharacter.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Inventory/UI/AGSDHotbarSlotWidget.h"
#include "Inventory/UI/AGSDPlayerHUD.h"
#include "Kismet/GameplayStatics.h"

void UAGSDSlotDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);

	if (SourceInventoryComponent && SourceWidget)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(SourceInventoryComponent, 0);
		if (PlayerPawn)
		{
			if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(PlayerPawn))
			{
				// 상자가 열려 있거나, 인벤토리 UI가 열려 있는 경우
				bool bIsUIOpen = Character->OpenedChest != nullptr || (Character->PlayerHUDRef && Character->PlayerHUDRef->IsInventoryOpen());
				if (bIsUIOpen)
				{
					// 핫바 복원 동작 중인지 확인
					bool bIsHotbarRestoring = false;
					if (bFromHotbar)
					{
						if (UAGSDHotbarSlotWidget* HotbarSlot = Cast<UAGSDHotbarSlotWidget>(SourceWidget))
						{
							if (HotbarSlot->bDragToHotbar && HotbarSlot->SelectedHotbar)
							{
								bIsHotbarRestoring = true;
							}
						}
					}

					// 핫바 복원이 수행되지 않고 그냥 캔슬된 경우 (위젯 밖 드롭)
					if (!bIsHotbarRestoring)
					{
						SourceInventoryComponent->DropItem(SourceSlotIndex);
					}
				}
			}
		}
	}
}
