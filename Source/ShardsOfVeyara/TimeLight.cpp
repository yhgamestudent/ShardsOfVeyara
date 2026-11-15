// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeLight.h"

#include "FarmingGameMode.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ATimeLight::ATimeLight()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;
	
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(RootComponent);
	SunLight->SetIntensity(3.f);

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
	MoonLight->SetupAttachment(RootComponent);
	MoonLight->SetIntensity(3.f);
	MoonLight->SetLightColor(FColor(152, 168, 231));
	MoonLight->SetAtmosphereSunLightIndex(1);
	
	ExponentialHeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("ExponentialHeightFog"));
	ExponentialHeightFog->SetupAttachment(RootComponent);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ATimeLight::BeginPlay()
{
	Super::BeginPlay();
	
	GameState = Cast<AAGSDGameStateBase>(UGameplayStatics::GetGameState(GetWorld()));
}

// Called every frame
void ATimeLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GameState) return;
	if (!Timeflow) return;
	CurrentTime = GameState->GetCurrentTime();
	/*

	//float TimeOffset = 360.0f;
	const float TotalTime = 2400.0f; 

	
    
	// 2. 현재 시간을 총 시간으로 나누어 0.0 ~ 1.0 사이의 회전 비율을 구합니다.
	// % 연산을 사용하여 하루가 끝나면 비율이 0으로 리셋되도록 합니다.
	float TimeInCycle = FMath::Fmod(CurrentTime + TotalTime, TotalTime);
	float TimeRatio = TimeInCycle / TotalTime;
	
	// 3. 0.0 ~ 1.0 비율을 360도 회전 각도로 변환합니다.
	float NewPitch = TimeRatio * 360.0f; // Pitch (X축 회전)이 하늘을 가리킵니다.
    
	// 4. 절대 회전 값을 설정합니다. (AddActorLocalRotation 대신 SetActorRelativeRotation 사용)
	// SetActorRelativeRotation을 사용해야 누적 회전이 아닌 원하는 각도로 정확히 설정됩니다.
	// 기존 코드에서 Roll(X) 축 회전만 사용했으므로 Pitch(X)로 가정합니다.
	
	SunLight->SetRelativeRotation(FRotator(0.f - NewPitch, 0.f, 0.f));
	MoonLight->SetRelativeRotation(FRotator(180.f - NewPitch, 0.f, 0.f));
	*/
}

