#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionOwnerInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractionOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class AGSD_API IInteractionOwnerInterface
{
	GENERATED_BODY()

public:
	// 상호작용 컴포넌트를 반환하는 순수 가상 함수
	virtual class UAGSDInteractionComponent* GetInteractionComponent() const = 0;
};
