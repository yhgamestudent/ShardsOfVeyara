// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction.h"
#include "GameFramework/Actor.h"
#include "AGSDCharacter.h"
#include "PickUpItem.generated.h"
/**
 * 
 */
class UDataTable;

UCLASS()
class AGSD_API APickUpItem : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:
	APickUpItem();

	virtual void BeginPlay() override;;

	//오버랩 시작 시 작동할 함수
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//오버랩 종료 시 작동할 함수
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	bool Holding = false;

	/** 아이템 고유 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pick Up Item")
	FString ItemID;

	/** 아이템 수량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pick Up Item")
	int32 Amount = 1;
	
	FText InteractActionText = FText::FromString(TEXT("수집하기"));

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interact")
	void PickUpInteract(AAGSDCharacter* player);
	
public:
	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;

	/** 손에 들어져 장착 상태가 될 때 물리 및 모든 콜리전을 강제 비활성화합니다. */
	void DisableCollisionForHolding();

	FORCEINLINE UStaticMeshComponent* GetMeshComponent() const { return Mesh; };
	FORCEINLINE void SetAmount(int32 InAmount) { Amount = InAmount; }
	FORCEINLINE void SetItemID(const FString& InItemID) { ItemID = InItemID; }
private:
	//콜리전 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CultivationPlot, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* CollisionBox;
};
