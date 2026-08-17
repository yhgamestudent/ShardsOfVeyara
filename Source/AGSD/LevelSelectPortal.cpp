// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelSelectPortal.h"

#include "AGSDCharacter.h"
#include "PortalSelectWidget.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/GarbageCollectionSchema.h"

// Sets default values
ALevelSelectPortal::ALevelSelectPortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Portal"));
	RootComponent = CollisionBox;

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ALevelSelectPortal::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &ALevelSelectPortal::OnEndOverlap);
}

// Called when the game starts or when spawned
void ALevelSelectPortal::BeginPlay()
{
	Super::BeginPlay();
	
}

void ALevelSelectPortal::OnWidgetClosed()
{
	// 플레이어 컨트롤러 가져오기
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	// 입력 모드를 다시 게임 전용으로 변경
	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = false;

	// 캐릭터 상태 및 테이블 상태 복구
	if (player)
	{
		player->Mining = false; // 이동 가능하게 변경
	}
}

//오버랩 시 위젯 노출
void ALevelSelectPortal::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	player = Cast<AAGSDCharacter>(OtherActor);
	if (player)
	{
		player->Mining = true;
		
		//위젯 클래스가 있고 위젯이 생성되지 않았으면 위젯 생성
		if (PortalSelectWidgetClass && !PortalSelectWidget)
			PortalSelectWidget = CreateWidget<UPortalSelectWidget>(GetWorld(), PortalSelectWidgetClass);

		//위젯이 생성되지 않으면 리턴
		if (!PortalSelectWidget) return;
		
		PortalSelectWidget->LevelSelectPortal = this;		
		PortalSelectWidget->AddToViewport();
		
		//입력모드 변경
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PortalSelectWidget->TakeWidget());
		player->getPlayerController()->SetInputMode(InputMode);
		player->getPlayerController()->bShowMouseCursor = true;
	}
}

void ALevelSelectPortal::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
}

