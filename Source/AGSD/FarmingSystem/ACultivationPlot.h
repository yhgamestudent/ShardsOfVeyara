// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AGSDGameStateBase.h"
#include "GameFramework/Actor.h"
#include "UCropData.h"
#include "Interaction.h"
#include "Crop.h"
#include "Weeds.h"
#include "ACultivationPlot.generated.h"

UCLASS()
class AGSD_API AACultivationPlot : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AACultivationPlot();

protected:
	void HandleDayPassed(int32 CurrentDay);
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Farming")
	TSubclassOf<ACrop> CropClassToPlant;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	class UDataTable* SeedDataTable;
	
	//작물 정보
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	UUCropData* CropData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	class ACrop* PlantedCrop = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	bool bHasWeeds = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	float weedsProb = 0.1f;
	
	UPROPERTY(EditAnywhere, Category = "Farming")
	TSubclassOf<AWeeds> WeedsActorClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	class AWeeds* WeedsActor = nullptr;

	void PlantCrop();

	UFUNCTION()
	void OnPlantedCropDestroyed();

	FText InteractActionText = FText::FromString(TEXT("작물심기"));

	void GetSeedInfo(FName TargetRowName);
	
	//다음 성장 날짜
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    int32 ScheduledDay = 0;
    //잡초로 인한 누적 지연 날짜
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    int32 DelayedDays = 0;
    //현재 단계
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    int32 CurrentGrowStageIndex = 0;
    //최종 단계
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    int32 FinishGrowStageIndex = 0;
    //다음 단계까지 남은 시간
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    int32 GrowthTimeCounter = 0;
    //작물이 다 자랐는지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
    bool FullyGrown = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
	FName SeedName = NAME_None;

	// 추가 보너스 수확량
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
	int32 BonusYield = 0;

	AAGSDGameStateBase* GS = nullptr;
	USOVGameInstance* GI = nullptr;

	UFUNCTION()
	void OnWeedRemoved();
	void GrowthLogic();
	void SpawnWeeds();

public:	
	UFUNCTION(BlueprintCallable, Category = "Farming")
	void SetSeedInfo(FName InSeedName, UDataTable* InSeedDataTable);

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void SetSeedDataTable(UDataTable* InSeedDataTable) { SeedDataTable = InSeedDataTable; }

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void SetSeedName(FName InSeedName) { SeedName = InSeedName; }

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void ApplyFertilizer(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Farming")
	void ApplyGrowthElixir();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//오버랩 시작 시 작동할 함수
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	//오버랩 종료 시 작동할 함수
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;
	
	FORCEINLINE int32 GetScheduledDay() const { return ScheduledDay; };
    FORCEINLINE int32 GetGrowthTimeCounter() const { return GrowthTimeCounter; };
    FORCEINLINE int32 GetCurrentGrowStageIndex() const { return CurrentGrowStageIndex; };
    FORCEINLINE bool GetFullyGrown() const { return FullyGrown; };
	
    //작물 매니저가 성장 날짜가 되었을 땨 호출하여 성장을 처리하는 함수
    
	UFUNCTION(BlueprintCallable, Category = "Farming")
	void AdvanceGrowth();

	UFUNCTION(BlueprintCallable, Category = "Farming|Effects")
	void SpawnGrowthEffect(class UNiagaraSystem* EffectSystem);
    
private:
	//루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CultivationPlot, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mesh;
	//콜리전 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CultivationPlot, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionBox;

	// 풍요 비료 지속 이펙트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CultivationPlot, meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* FertilizerEffectComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* FertilizerEffectSystem;

	void UpdateFertilizerEffect();
};
