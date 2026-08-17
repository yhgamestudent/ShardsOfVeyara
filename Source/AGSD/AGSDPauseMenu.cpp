#include "AGSDPauseMenu.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AGSDCharacter.h"

void UAGSDPauseMenu::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 클릭 이벤트 델리게이트 바인딩 (중복 등록 방지를 위해 먼저 해제 후 등록)
	if (BTN_Resume)
	{
		BTN_Resume->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnResumeClicked);
		BTN_Resume->OnClicked.AddDynamic(this, &UAGSDPauseMenu::OnResumeClicked);
	}
	if (BTN_ReturnToMainMenu)
	{
		BTN_ReturnToMainMenu->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnReturnToMainMenuClicked);
		BTN_ReturnToMainMenu->OnClicked.AddDynamic(this, &UAGSDPauseMenu::OnReturnToMainMenuClicked);
	}
	if (BTN_Settings)
	{
		BTN_Settings->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnSettingsClicked);
		BTN_Settings->OnClicked.AddDynamic(this, &UAGSDPauseMenu::OnSettingsClicked);
	}
	if (BTN_Quit)
	{
		BTN_Quit->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnQuitClicked);
		BTN_Quit->OnClicked.AddDynamic(this, &UAGSDPauseMenu::OnQuitClicked);
	}

	// 캐릭터에 자신을 퍼즈 키로 닫을 수 있는 활성 UI로 등록
	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(GetOwningPlayerPawn()))
	{
		OwningCharacter = Character;
		Character->RegisterCloseableUI(this);
	}
}

void UAGSDPauseMenu::NativeDestruct()
{
	// 이벤트 바인딩 해제
	if (BTN_Resume)
	{
		BTN_Resume->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnResumeClicked);
	}
	if (BTN_ReturnToMainMenu)
	{
		BTN_ReturnToMainMenu->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnReturnToMainMenuClicked);
	}
	if (BTN_Settings)
	{
		BTN_Settings->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnSettingsClicked);
	}
	if (BTN_Quit)
	{
		BTN_Quit->OnClicked.RemoveDynamic(this, &UAGSDPauseMenu::OnQuitClicked);
	}

	// 약참조가 유효한 경우에만 캐릭터에서 등록 해제
	if (OwningCharacter.IsValid())
	{
		OwningCharacter->UnregisterCloseableUI(this);
	}

	Super::NativeDestruct();
}

void UAGSDPauseMenu::CloseUI_Implementation()
{
	// 퍼즈 키 재입력으로 인해 닫힐 때 캐릭터의 일시정지 UI 제거 함수 호출
	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(GetOwningPlayerPawn()))
	{
		Character->RemovePauseUI();
	}
}

void UAGSDPauseMenu::OnResumeClicked()
{
	// Resume 버튼 클릭 시 일시정지 UI 제거 및 게임 재개
	if (AAGSDCharacter* Character = Cast<AAGSDCharacter>(GetOwningPlayerPawn()))
	{
		Character->RemovePauseUI();
	}
}

void UAGSDPauseMenu::OnReturnToMainMenuClicked()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		// 메인 메뉴 레벨로 비동기 전환
		if (!MainMenuLevel.IsNull())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(this, MainMenuLevel, true);
		}

		// 블루프린트 로직과 동일하게 메인 메뉴 진입 전 입력 및 커서 상태 초기화
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
}

void UAGSDPauseMenu::OnSettingsClicked()
{
	// 설정창 위젯 생성 및 표시
	if (SettingsMenuClass)
	{
		UUserWidget* SettingsWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), SettingsMenuClass);
		if (SettingsWidget)
		{
			SettingsWidget->AddToViewport();
		}
	}
}

void UAGSDPauseMenu::OnQuitClicked()
{
	// 게임 종료
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
