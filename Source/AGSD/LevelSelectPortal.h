// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Struct_MapData.h"
#include "GameFramework/Actor.h"
#include "LevelSelectPortal.generated.h"

class UPortalSelectWidget;

USTRUCT(BlueprintType)
struct FMapStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText MapName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Texture;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TeleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> Level;
};

UCLASS()
class AGSD_API ALevelSelectPortal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelSelectPortal();
	
	void OnWidgetClosed();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	UDataTable* MapStructs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	TSubclassOf<UPortalSelectWidget> PortalSelectWidgetClass;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	UPortalSelectWidget* PortalSelectWidget;

	class AAGSDCharacter* player;
	
public:	
	//오버랩 시작 시 작동할 함수
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	//오버랩 종료 시 작동할 함수
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	FORCEINLINE FStruct_MapData getMapStruct(int index)
	{
		TArray<FName> RowNames = MapStructs->GetRowNames();
		if (RowNames.IsValidIndex(index + 1))
		{
			FName TargetRowName = RowNames[index + 1];
			return *MapStructs->FindRow<FStruct_MapData>(TargetRowName, TEXT("ContextString"));
		}
		else return FStruct_MapData();

	}
};
