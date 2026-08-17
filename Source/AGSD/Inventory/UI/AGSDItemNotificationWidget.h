#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AGSDItemNotificationWidget.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;
class UTexture2D;

/**
 * UAGSDItemNotificationWidget
 * 아이템 획득 알림 위젯의 C++ 베이스 클래스입니다.
 * 애니메이션을 통해 화면에 나타났다가 딜레이 후 사라지는 연출을 제어합니다.
 */
UCLASS()
class AGSD_API UAGSDItemNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 알림창에 필요한 정보(이름, 수량, 아이콘)를 설정하고 연출을 시작합니다. */
	UFUNCTION(BlueprintCallable, Category = "Item Notification")
	void SetupNotification(const FString& InName, int32 InAmount, UTexture2D* InIcon);

protected:
	// ── UMG 바인딩 위젯 ──
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UImage> IMG_ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> TXT_ItemNameAmount;

	// ── 애니메이션 바인딩 ──
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Animation_SlideRight;

	// ── 내부 상태 데이터 ──
	UPROPERTY(BlueprintReadOnly, Category = "Notification Data")
	FString ItemName;

	UPROPERTY(BlueprintReadOnly, Category = "Notification Data")
	int32 ItemAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Notification Data")
	TObjectPtr<UTexture2D> ItemIcon;

private:
	FTimerHandle SlideOutTimerHandle;
	FTimerHandle DestroyTimerHandle;

	/** 슬라이드 아웃 애니메이션을 역재생(Reverse)합니다. */
	void StartSlideOut();

	/** 위젯을 부모로부터 완전히 제거합니다. */
	void DestroyNotification();
};
