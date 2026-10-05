// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UCropData.h"
#include "Interaction.h"
#include "harvest.h"
#include "Crop.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCropHarvestedDelegate);
UCLASS()
class AGSD_API ACrop : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACrop();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Weeds의 SnapWeedsToGround를 기반으로 한 지면/FieldMesh 스냅 및 법선 정렬 함수
	UFUNCTION(BlueprintCallable, Category = "Farming")
	void SnapCropToGround();

	//현재 단계의 작물 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CropMesh;
	UPROPERTY(EditAnywhere, Category = "Farming") // EditDefaultsOnly도 가능
	TSubclassOf<Aharvest> HarvestClass;
	//작물 정보
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	UUCropData* CropData;
	
	void HarvestCrop(int32 Quantity);

	FText InteractActionText = FText::FromString(TEXT("수확하기"));

	bool bIsHarvested = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TEnumAsByte<ECollisionChannel> PlacementTraceChannel = ECC_WorldStatic;

	// 레이저를 상하로 쏠 거리 (cm 단위, 위아래로 이 거리만큼 레이트레이스 탐색)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float TraceDistance = 100.0f;

	// 바닥 스냅 시 지면 위로 띄울 높이 오프셋 (cm 단위)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float GroundZOffset = 0.0f;

	// 추가 보너스 수확량 (풍요 비료 반영용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farming")
	int32 BonusYield = 0;
	
public:	
	//작물이 경작지에 심길 때 실행할 함수
	void SetCropData(UUCropData* CData);

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void SetBonusYield(int32 Amount);

	//오버랩 시작 시 작동할 함수
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//오버랩 종료 시 작동할 함수
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;
	virtual FString GetInteractionActionType_Implementation(AAGSDCharacter* player) override;

    void SetCollisionEnable();
    
    //작물 메시 정보 업데이트
    void MeshUpdate(int32 CurrentGrowStageIndex);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCropHarvestedDelegate OnHarvested;
private:
	//콜리전 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CultivationPlot, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* CollisionBox;
};
