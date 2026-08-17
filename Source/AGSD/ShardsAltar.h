// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AGSDCharacter.h"
#include "Interaction.h"
#include "GameFramework/Actor.h"
#include "ShardsAltar.generated.h"

UCLASS()
class AGSD_API AShardsAltar : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShardsAltar();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	class UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Altar")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Altar")
	class USceneComponent* DimensionMarbleRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	int32 ShardsAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	TArray<UStaticMesh*> DimensionMarbleStructs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	class UNiagaraSystem* ShardEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	FRotator RotationSpeed = FRotator(0.f, -10.f, 0.f);

	USOVGameInstance* GI;

public:	
	//오버랩 시작 시 작동할 함수
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	//오버랩 종료 시 작동할 함수
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;
	
	FText InteractActionText = FText::FromString(TEXT("조각보기"));

	FORCEINLINE int32 getShardsAmount() { return ShardsAmount; }

	void SpawnShards(int32 i);
};
