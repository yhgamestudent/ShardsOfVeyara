// Fill out your copyright notice in the Description page of Project Settings.


#include "DimensionMarble.h"
#include "GameFramework/RotatingMovementComponent.h"

// Sets default values
ADimensionMarble::ADimensionMarble()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DimensionMarbleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DimensionMarbleRoot"));
	RootComponent = DimensionMarbleRoot;

	RotatingMovementComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComponent"));
}

// Called when the game starts or when spawned
void ADimensionMarble::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADimensionMarble::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

