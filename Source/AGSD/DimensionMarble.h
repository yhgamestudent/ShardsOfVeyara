// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DimensionMarble.generated.h"

UCLASS()
class AGSD_API ADimensionMarble : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADimensionMarble();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DimensionMarble")
	TArray<UStaticMesh*> marbleArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DimensionMarble")
	class UNiagaraSystem* marbleEffect;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DimensionMarble")
	class USceneComponent* DimensionMarbleRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DimensionMarble")
	class URotatingMovementComponent* RotatingMovementComponent;
};
