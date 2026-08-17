// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AGSDCloseableUIInterface.h"
#include "AGSDPauseMenu.generated.h"

/**
 * UAGSDPauseMenu
 * 일시정지 메뉴 위젯의 C++ 베이스 클래스입니다.
 * IUIClosable 인터페이스를 구현하여 퍼즈 키가 다시 눌릴 때 메뉴가 닫히도록 지원합니다.
 */
UCLASS()
class AGSD_API UAGSDPauseMenu : public UUserWidget, public IUIClosable
{
	GENERATED_BODY()

protected:
	// ── UI 바인딩 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UButton* BTN_Resume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UButton* BTN_ReturnToMainMenu;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UButton* BTN_Settings;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UButton* BTN_Quit;

	// ── 설정 데이터 ──
	/** 생성할 설정 메뉴 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUI|Config")
	TSubclassOf<UUserWidget> SettingsMenuClass;

	/** 메인 메뉴 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUI|Config")
	TSoftObjectPtr<UWorld> MainMenuLevel;

public:
	// IUIClosable 인터페이스 구현
	virtual void CloseUI_Implementation() override;

protected:
	TWeakObjectPtr<class AAGSDCharacter> OwningCharacter;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnReturnToMainMenuClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitClicked();
};
