// Fill out your copyright notice in the Description page of Project Settings.

#include "AGSDChestAndInventory.h"
#include "AGSDCharacter.h"
#include "Components/Button.h"
#include "Inventory/UI/AGSDInventoryWidget.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Chest.h"
#include "GameFramework/PlayerController.h"

void UAGSDChestAndInventory::NativeConstruct()
{
	Super::NativeConstruct();

	// 닫기 버튼 이벤트 바인딩 (중복 등록 방지를 위해 먼저 해제 후 등록)
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UAGSDChestAndInventory::OnCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UAGSDChestAndInventory::OnCloseButtonClicked);
	}

	// UI가 최초 열릴 때 마우스 표시 및 입력 모드 설정 적용 (화면 돌아감 및 포커스 유실 방지)
	if (APlayerController* PC = Cast<APlayerController>(GetOwningPlayer()))
	{
		PC->SetShowMouseCursor(true);

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}

	// 캐릭터에 자신을 퍼즈 키로 닫을 수 있는 활성 UI로 등록
	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(GetOwningPlayerPawn()))
	{
		OwningCharacter = Character;
		Character->RegisterCloseableUI(this);

		// 상자가 열려 있는 동안은 다른 상자가 중복 상호작용으로 열리지 않도록 제어
		Character->SetCanOpenChest(false);

		// 블루프린트 위젯 트리에서 하위 위젯을 런타임 검색 (BindWidgetOptional 충돌 방지)
		UAGSDInventoryWidget* InvUI = Cast<UAGSDInventoryWidget>(GetWidgetFromName(TEXT("WBP_InventoryUI")));
		UAGSDInventoryWidget* ChestUI = Cast<UAGSDInventoryWidget>(GetWidgetFromName(TEXT("WBP_ChestUI")));

		// 플레이어 가방 인벤토리 UI 초기화 (StartSlotIndex 기본값 10 = 가방 영역)
		if (InvUI && Character->InventoryComponent)
		{
			InvUI->InitializeInventory(Character->InventoryComponent);
		}

		// 상자 자체의 인벤토리 UI 초기화 (슬롯 0번부터 전체 표시)
		if (ChestUI && Character->OpenedChest && Character->OpenedChest->GetInventoryComponent())
		{
			ChestUI->StartSlotIndex = 0;
			ChestUI->InitializeInventory(Character->OpenedChest->GetInventoryComponent());
		}
	}
}

void UAGSDChestAndInventory::NativeDestruct()
{
	// 이벤트 바인딩 해제
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UAGSDChestAndInventory::OnCloseButtonClicked);
	}

	// 캐릭터에서 등록 해제 및 상태 원복
	if (OwningCharacter.IsValid())
	{
		OwningCharacter->UnregisterCloseableUI(this);
		OwningCharacter->SetCanOpenChest(true);
		OwningCharacter->OpenedChest = nullptr; // 상자 포인터 초기화 (카메라 회전 복구를 위해 필수)
	}

	Super::NativeDestruct();
}

void UAGSDChestAndInventory::CloseUI_Implementation()
{
	// 상자 UI를 뷰포트 및 부모(CanvasPanel)에서 제거합니다.
	RemoveFromParent();

	// UI가 닫힐 때 마우스 커서를 숨기고 입력 모드를 GameOnly로 복구합니다.
	if (APlayerController* PC = Cast<APlayerController>(GetOwningPlayer()))
	{
		PC->SetShowMouseCursor(false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

void UAGSDChestAndInventory::OnCloseButtonClicked()
{
	CloseUI_Implementation();
}
