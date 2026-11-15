// Fill out your copyright notice in the Description page of Project Settings.



#pragma once



#include "CoreMinimal.h"

#include "Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotify_PlayNiagaraTimed.generated.h"



class UNiagaraSystem;



/**

 * 일반 노티파이처럼 타임라인의 단일 프레임에 배치하되,

 * 지정된 지속시간(Duration) 후 Deactivate를 호출하여 신규 스폰만 멈추고

 * 기존에 튀어나간 잔여 파티클(스파크 등)은 자연스럽게 끝까지 날아가며 소멸하도록 유도하는 애님 노티파이

 */

UCLASS(Const, hidecategories = Object, CollapseCategories, meta = (DisplayName = "Play Niagara Timed"))

class SHARDSOFVEYARA_API UAnimNotify_PlayNiagaraTimed : public UAnimNotify

{

	GENERATED_BODY()



public:

	UAnimNotify_PlayNiagaraTimed();



	// 스폰할 나이아가라 시스템 에셋

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")

	TObjectPtr<UNiagaraSystem> Template;



	// 메인 이펙트 신규 스폰 지속시간 (초). 이 시간이 지나면 Deactivate되어 새 파티클 생성을 멈추고 잔여 스파크는 자연 소멸합니다.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ClampMin = "0.01", UIMin = "0.01"))

	float Duration = 0.15f;



	// 위치 오프셋

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")

	FVector LocationOffset = FVector(0.0f, 0.0f, 50.0f);



	// 회전 오프셋

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")

	FRotator RotationOffset = FRotator(-90.0f, 0.0f, 0.0f);



	// 스케일

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")

	FVector Scale = FVector(1.0f, 1.0f, 1.0f);



	// 캐릭터 소켓에 부착할지 여부

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")

	bool bAttached = true;



	// 부착할 소켓명

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")

	FName SocketName = FName("Weapon_Trail");



	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

};



