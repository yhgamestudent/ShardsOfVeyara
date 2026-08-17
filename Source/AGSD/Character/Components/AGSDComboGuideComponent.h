#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AGSDComboGuideComponent.generated.h"

class AAGSDCharacter;

USTRUCT(BlueprintType)
struct FComboGuideEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo UI")
	FText InputText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo UI")
	FText AttackNameText;

	FComboGuideEntry() {}
	FComboGuideEntry(const FText& InInput, const FText& InName)
		: InputText(InInput), AttackNameText(InName) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboGuideUpdatedSignature, const TArray<FComboGuideEntry>&, GuideEntries, const FText&, CurrentAttackName);

/**
 * UAGSDComboGuideComponent
 * 플레이어 캐릭터의 이동, 공격 및 콤보 상태를 모니터링하여
 * 몬스터 헌터 스타일 콤보 조작 가이드 UI 텍스트 델리게이트를 송출하는 독립 액터 컴포넌트입니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AGSD_API UAGSDComboGuideComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGSDComboGuideComponent();

	/** 콤보 가이드 UI 갱신 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Combo UI")
	FOnComboGuideUpdatedSignature OnComboGuideUpdated;

	/** 현재 소유 캐릭터의 상태에 맞춰 가이드 UI 텍스트를 계산하고 델리게이트를 호출합니다 */
	UFUNCTION(BlueprintCallable, Category = "Combo UI")
	void UpdateComboGuideUI();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<AAGSDCharacter> OwnerCharacter;
};
