// Fill out your copyright notice in the Description page of Project Settings.


#include "Cooking/CookingUI.h"
#include "AGSDCharacter.h"
#include "Components/Button.h"

void UCookingUI::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	// 1. Farmer 캐릭터 가져오기 및 Closeable UI 등록
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(PC->GetPawn()))
		{
			Farmer = Character;
			Character->RegisterCloseableUI(this);
		}
	}

	// 2. 닫기 버튼 바인딩 및 이벤트 연결
	if (BTN_Close)
	{
		BTN_Close->OnClicked.RemoveDynamic(this, &UCookingUI::OnCloseButtonClicked);
		BTN_Close->OnClicked.AddDynamic(this, &UCookingUI::OnCloseButtonClicked);
	}

	// 3. 핫바 슬롯 위젯들을 배열에 저장
	InsertedCropSlotArray.Empty();
	InsertedCropSlotArray.Add(WBP_CookHotbar_1);
	InsertedCropSlotArray.Add(WBP_CookHotbar_2);
	InsertedCropSlotArray.Add(WBP_CookHotbar_3);
	InsertedCropSlotArray.Add(WBP_CookHotbar_4);
	InsertedCropSlotArray.Add(WBP_CookHotbar_5);
	InsertedCropSlotArray.Add(WBP_CookHotbar_6);
	InsertedCropSlotArray.Add(WBP_CookHotbar_7);
	InsertedCropSlotArray.Add(WBP_CookHotbar_8);
	InsertedCropSlotArray.Add(WBP_CookHotbar_9);
	InsertedCropSlotArray.Add(WBP_CookHotbar_10);
}

void UCookingUI::NativeDestruct()
{
	// UI 파괴 시 등록 해제
	if (Farmer)
	{
		Farmer->UnregisterCloseableUI(this);
	}
	
	Super::NativeDestruct();
}

void UCookingUI::OnCloseButtonClicked()
{
	// 인터페이스 함수인 CloseUI 호출
	IUIClosable::Execute_CloseUI(this);
}

void UCookingUI::CloseUI_Implementation()
{
	// 부모 위젯으로부터 제거하여 UI 닫기
	RemoveFromParent();
}
