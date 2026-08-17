// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction.h"
#include "AppleTreeSaveData.h"
#include "AppleTree.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UStaticMesh;
class USOVGameInstance;

UCLASS()
class AGSD_API AAppleTree : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAppleTree();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 메인 나무 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AppleTree|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TreeMesh;

	// 상호작용 오버랩 콜리전 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AppleTree|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionBox;

	// 사과가 없는 나무 스태틱 메시 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppleTree|Assets")
	TObjectPtr<UStaticMesh> NoAppleMesh;

	// 사과가 달린 나무 스태틱 메시 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppleTree|Assets")
	TObjectPtr<UStaticMesh> AppleMesh;

	// 매일 아침 사과가 열릴 확률 (0.1f = 10%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppleTree|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float AppleSpawnProbability = 0.1f;

	// 현재 사과가 달렸는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppleTree|Settings")
	bool bHasApple = false;

	// 수확 시 지급할 사과 아이템 ID (기본: "Item_Apple")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppleTree|Settings")
	FString AppleItemID = TEXT("Item_Apple");

	// 한 번 수확 시 획득할 사과 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppleTree|Settings")
	int32 AppleHarvestAmount = 1;

	// 마지막으로 갱신 확인한 날짜
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AppleTree|Settings")
	int32 LastCheckedDay = 1;

	FText InteractActionText = FText::FromString(TEXT("사과 수확하기"));

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void HandleDayPassed(int32 CurrentDay);
	void UpdateTreeMesh();
	void SaveStateToGI();

public:	
	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;
};
