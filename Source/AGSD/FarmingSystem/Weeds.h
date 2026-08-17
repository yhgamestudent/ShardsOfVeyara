// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AGSDCharacter.h"
#include "Interaction.h"
#include "GameFramework/Actor.h"
#include "Weeds.generated.h"

#define ECC_WeedPlace ECC_GameTraceChannel2

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeedingDelegate);
UCLASS()
class AGSD_API AWeeds : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeeds();

protected:
	UFUNCTION(BlueprintCallable)
	void SnapWeedsToGround();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//오버랩 시작 시 작동할 함수
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//오버랩 종료 시 작동할 함수
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	bool Holding = false;
	
	FText InteractActionText = FText::FromString(TEXT("잡초제거"));
	bool bIsActionActive = false;
	AAGSDPlayerController* PC;
	
	AAGSDCharacter* Player;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interact")
	void WeedingInteract(AAGSDCharacter* player);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TEnumAsByte<ECollisionChannel> PlacementTraceChannel;

	void OnMontageEnded(UAnimMontage* AnimMontage, bool bArg);
	void PlayPullPlant(AAGSDCharacter* Player);
	
public:
	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeedingDelegate OnWeeding;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	UAnimMontage* PullPlant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	class UNiagaraSystem* WeedEffect;
	
private:
	//콜리전 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CultivationPlot, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionBox;
};
