// Fill out your copyright notice in the Description page of Project Settings.



#pragma once



#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "AGSDWarningNoticeWidget.generated.h"



class UTextBlock;



/**

 * UAGSDWarningNoticeWidget

 * 화면 중앙 상단에 부드럽게 페이드 인/아웃되는 시스템 경고 및 알림 토스트 위젯입니다.

 * 체력 만복, 포션 쿨다운 등 다양한 인게임 시스템 알림에 공용으로 사용됩니다.

 */

UCLASS()

class SHARDSOFVEYARA_API UAGSDWarningNoticeWidget : public UUserWidget

{

	GENERATED_BODY()



public:

	/** 알림 메시지를 설정하고 페이드 인 연출을 시작합니다. (연타 시 표시 시간 자동 연장) */

	UFUNCTION(BlueprintCallable, Category = "UI|Notice")

	void ShowNotice(const FText& InMessage, float InDuration = 1.5f);



	/** 알림 텍스트만 변경 */

	UFUNCTION(BlueprintCallable, Category = "UI|Notice")

	void SetNoticeText(const FText& InMessage);



	/** 목표 불투명도 직접 설정 */

	void SetTargetOpacity(float NewOpacity);



protected:

	UPROPERTY(meta = (BindWidgetOptional))

	TObjectPtr<UTextBlock> NoticeTextBlock;



	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;



private:

	float CurrentOpacity = 0.0f;

	float TargetOpacity = 0.0f;

	float FadeSpeed = 8.0f;



	FTimerHandle HideTimerHandle;



	void StartFadeOut();

};



