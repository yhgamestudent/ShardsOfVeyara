// Fill out your copyright notice in the Description page of Project Settings.



#pragma once



#include "CoreMinimal.h"

#include "AGSDCharacter.h"

#include "Interaction.h"

#include "GameFramework/Actor.h"

#include "ShardsAltar.generated.h"



class UCameraComponent;

class USoundBase;



UCLASS()

class SHARDSOFVEYARA_API AShardsAltar : public AActor, public IInteraction

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



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Altar|Cutscene")

	UCameraComponent* CutsceneCameraComp;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")

	int32 ShardsAmount = 0;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")

	TArray<UStaticMesh*> DimensionMarbleStructs;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")

	class UNiagaraSystem* ShardEffect;



	// ─── [구슬 소환 연출 이펙트 및 사운드] ───

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Effects")

	class UNiagaraSystem* ShardSpawnBurstEffect;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Effects")

	class USoundBase* ShardSpawnSound;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Effects")

	FVector SpawnEffectScale = FVector(1.0f);

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")

	FRotator RotationSpeed = FRotator(0.f, -10.f, 0.f);



	// ─── [시네마틱 컷신 연출 설정] ───

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene")

	bool bEnableCutscene = true;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene")

	bool bUseManualCutsceneCameraTransform = true;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene", meta = (ClampMin = "0.1"))

	float CutsceneBlendInDuration = 1.2f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene", meta = (ClampMin = "0.0"))

	float CutsceneHoldDuration = 1.5f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene", meta = (ClampMin = "0.1"))

	float CutsceneBlendOutDuration = 1.2f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene", meta = (ClampMin = "0.05"))

	float RewardPopDuration = 0.35f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene")

	FName RegularDialogueID = FName("AltarShardOffered_Regular");



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Cutscene")

	FName FinalDialogueID = FName("AltarShardOffered_Final");



	USOVGameInstance* GI;



	// ─── [컷신 단계별 함수] ───

	void StartOfferingCutscene(AAGSDCharacter* Player);



	UFUNCTION()

	void OnCutsceneBlendInFinished();



	UFUNCTION()

	void UpdateRewardPopScale();



	UFUNCTION()

	void StartCutsceneBlendOut();



	UFUNCTION()

	void RestorePlayerAfterCutscene();



	void PlayShardSpawnEffects(const FVector& Location, const FRotator& Rotation);



private:

	TWeakObjectPtr<AAGSDCharacter> CachedPlayerCharacter;

	TWeakObjectPtr<APlayerController> CachedPlayerController;

	TWeakObjectPtr<UStaticMeshComponent> NewlySpawnedShardMesh;



	FTimerHandle CutsceneTimerHandle;

	FTimerHandle RewardPopTimerHandle;

	float PopElapsedTime = 0.0f;



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

	virtual FString GetInteractionActionType_Implementation(AAGSDCharacter* player) override;

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")

	FText InteractActionText = FText::FromString(TEXT("조각보기"));



	FORCEINLINE int32 getShardsAmount() { return ShardsAmount; }

	/** [치트] 아이템 소모 없이 구슬 1개 봉헌 (구슬 생성/이펙트/대사/엔딩 플래그까지 실제 봉헌과 동일, 컷신은 생략) */
	void CheatOfferShard(AAGSDCharacter* Player, bool bPlayDialogue);

	/** 봉헌 가능한 최대 구슬 수 */
	int32 GetMaxShards() const { return DimensionMarbleStructs.Num() > 0 ? DimensionMarbleStructs.Num() : 4; }



	UStaticMeshComponent* SpawnShards(int32 i);

};

