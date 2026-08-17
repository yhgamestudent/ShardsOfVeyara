// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction.h"
#include "Chest.generated.h"

UCLASS()
class AGSD_API AChest : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AChest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Open_Chest_UI();
	
public:
	/** 생성할 상자 UI 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChestUI")
	TSubclassOf<class UAGSDChestAndInventory> ChestWidgetClass;

	/** C++ 인벤토리 컴포넌트 Getter */
	FORCEINLINE class UAGSDInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

private:
	//루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* SkeletalMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* Mesh;
    	
	//콜리전 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionBox;

	/** C++ 인벤토리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	class UAGSDInventoryComponent* InventoryComponent;

	FText InteractActionText = FText::FromString(TEXT("열기"));
};
