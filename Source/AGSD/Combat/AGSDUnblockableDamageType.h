#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "AGSDUnblockableDamageType.generated.h"

/**
 * 가드(Guard) 및 저스트 가드(Parry)가 불가능한 피해(환경 바닥 함정, 독가스, 보스 특수기 등) 전용 기본 데미지 타입
 * 블루프린트에서 ApplyDamage 호출 시 Damage Type Class로 지정하여 사용합니다.
 */
UCLASS(Blueprintable, BlueprintType)
class AGSD_API UAGSDUnblockableDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UAGSDUnblockableDamageType();
};
