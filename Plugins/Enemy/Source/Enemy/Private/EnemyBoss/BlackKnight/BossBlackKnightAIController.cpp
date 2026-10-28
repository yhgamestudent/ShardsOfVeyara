#include "EnemyBoss/BlackKnight/BossBlackKnightAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyBoss/BlackKnight/BossBlackKnight.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

void ABossBlackKnightAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledEnemy = Cast<ABossBlackKnight>(GetPawn());

	if (BehaviorTree)
	{
		ForEachObjectWithOuter(BehaviorTree, [](UObject* SubObj)
		{
			if (!SubObj) return;
			const FString ObjName = SubObj->GetName();

			if (ObjName.StartsWith(TEXT("BTDecorator_Blackboard_4")))
			{
				if (FFloatProperty* FloatProp = FindFProperty<FFloatProperty>(SubObj->GetClass(), TEXT("FloatValue")))
				{
					FloatProp->SetPropertyValue_InContainer(SubObj, 400.0f);
				}
			}
			else if (ObjName.StartsWith(TEXT("BTTask_GetRandomFloatValue")))
			{
				if (FProperty* MinProp = SubObj->GetClass()->FindPropertyByName(TEXT("MinValue")))
				{
					if (FFloatProperty* FMin = CastField<FFloatProperty>(MinProp)) FMin->SetPropertyValue_InContainer(SubObj, 1.0f);
					else if (FDoubleProperty* DMin = CastField<FDoubleProperty>(MinProp)) DMin->SetPropertyValue_InContainer(SubObj, 1.0);
				}
				if (FProperty* MaxProp = SubObj->GetClass()->FindPropertyByName(TEXT("MaxValue")))
				{
					if (FFloatProperty* FMax = CastField<FFloatProperty>(MaxProp)) FMax->SetPropertyValue_InContainer(SubObj, 1.5f);
					else if (FDoubleProperty* DMax = CastField<FDoubleProperty>(MaxProp)) DMax->SetPropertyValue_InContainer(SubObj, 1.5);
				}
			}
			else if (ObjName.StartsWith(TEXT("BTDecorator_TimeLimit")))
			{
				if (FStructProperty* StructProp = FindFProperty<FStructProperty>(SubObj->GetClass(), TEXT("TimeLimit")))
				{
					void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(SubObj);
					if (FFloatProperty* DefProp = FindFProperty<FFloatProperty>(StructProp->Struct, TEXT("DefaultValue")))
					{
						DefProp->SetPropertyValue_InContainer(StructPtr, 1.5f);
					}
				}
			}
			else if (ObjName.StartsWith(TEXT("BTTask_SetKeyValueFloat")))
			{
				if (FStructProperty* StructProp = FindFProperty<FStructProperty>(SubObj->GetClass(), TEXT("Value")))
				{
					void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(SubObj);
					if (FFloatProperty* DefProp = FindFProperty<FFloatProperty>(StructProp->Struct, TEXT("DefaultValue")))
					{
						DefProp->SetPropertyValue_InContainer(StructPtr, 1.5f);
					}
				}
			}
		}, true);
	}
}

void ABossBlackKnightAIController::SetBlackboardKey()
{
	Super::SetBlackboardKey();

	if ( BlackboardComp == nullptr || ControlledEnemy == nullptr ) return;

	BlackboardComp->SetValueAsBool("CanAttack", true);
	BlackboardComp->SetValueAsFloat("AttackDelay", ControlledEnemy->CommonAttackDelay);

	BlackboardComp->SetValueAsObject(TEXT("TargetCharacter"), TargetPawn);

	const float InitialDist = TargetPawn
		? FVector::Dist(ControlledEnemy->GetActorLocation(), TargetPawn->GetActorLocation())
		: 9999.0f;
	const bool bIsCloseRange = (InitialDist <= 400.0f);

	BlackboardComp->SetValueAsFloat(TEXT("W_RushAttack"), bIsCloseRange ? 0.0f : ControlledEnemy->AttackStruct.RushAttackWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_Guard"), bIsCloseRange ? ControlledEnemy->AttackStruct.GuardWeight : 0.0f);
	BlackboardComp->SetValueAsFloat(TEXT("W_NormalAttack"), bIsCloseRange ? ControlledEnemy->AttackStruct.NormalAttackWeight : 0.0f);
	BlackboardComp->SetValueAsFloat(TEXT("W_ChargeAttack"), ControlledEnemy->AttackStruct.ChargeAttackWeight);
	BlackboardComp->SetValueAsFloat(TEXT("W_ZapAttack"), bIsCloseRange ? 0.0f : ControlledEnemy->AttackStruct.ZapAttackWeight);
}
