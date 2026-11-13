#include "Character/Components/AGSDComboGuideComponent.h"
#include "Character/AGSDCharacter.h"
#include "SpearComboData.h"

UAGSDComboGuideComponent::UAGSDComboGuideComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAGSDComboGuideComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AAGSDCharacter>(GetOwner());
}

void UAGSDComboGuideComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateComboGuideUI();
}

void UAGSDComboGuideComponent::UpdateComboGuideUI()
{
	TArray<FComboGuideEntry> Entries;
	FText CurrentAttackName;

	if (!OwnerCharacter || OwnerCharacter->HoldingWeapon != EHoldingWeapon::Spear || !OwnerCharacter->SpearComboDataTable)
	{
		OnComboGuideUpdated.Broadcast(Entries, CurrentAttackName);
		return;
	}

	// 1. 공격 진행 중인 경우
	if (OwnerCharacter->bIsAttacking && OwnerCharacter->CurrentComboData)
	{
		// 1-A. 현재 수행 중인 공격 이름 계산
		if (OwnerCharacter->CurrentComboData->Stages.IsValidIndex(OwnerCharacter->CurrentStageIndex))
		{
			const FSpearStageData& CurStage = OwnerCharacter->CurrentComboData->Stages[OwnerCharacter->CurrentStageIndex];
			if (!CurStage.AttackName.IsEmpty())
			{
				CurrentAttackName = CurStage.AttackName;
			}
			else if (OwnerCharacter->CurrentComboData->DirectionRequirement == ESpearAttackDirection::Parry)
			{
				CurrentAttackName = FText::FromString(TEXT("카운터 공격"));
			}
			else
			{
				CurrentAttackName = FText::FromString(FString::Printf(TEXT("찌르기 %d타"), OwnerCharacter->CurrentStageIndex + 1));
			}
		}

		// 1-B. 바로 다음 연계 콤보 조작 가이드 안내
		int32 NextIndex = OwnerCharacter->CurrentStageIndex + 1;
		FText InputText = FText::FromString(TEXT("[LMB / 좌클릭]"));
		FText NextAttackNameText;

		if (OwnerCharacter->CurrentComboData->Stages.IsValidIndex(NextIndex))
		{
			const FSpearStageData& NextStage = OwnerCharacter->CurrentComboData->Stages[NextIndex];

			// 다음 스테이지에서 요구하는 입력 버튼 조건에 따라 UI 조작 키 텍스트 동적 설정
			switch (NextStage.InputRequirement)
			{
			case ESpearAttackInput::RMB:
				InputText = FText::FromString(TEXT("[RMB / 우클릭]"));
				break;
			case ESpearAttackInput::Both_LMB_RMB:
				InputText = FText::FromString(TEXT("[LMB + RMB]"));
				break;
			case ESpearAttackInput::LMB:
			default:
				InputText = FText::FromString(TEXT("[LMB / 좌클릭]"));
				break;
			}

			if (!NextStage.AttackName.IsEmpty())
			{
				NextAttackNameText = NextStage.AttackName;
			}
			else if (OwnerCharacter->CurrentComboData->DirectionRequirement == ESpearAttackDirection::Parry)
			{
				NextAttackNameText = FText::FromString(TEXT("카운터 공격"));
			}
			else
			{
				NextAttackNameText = FText::FromString(FString::Printf(TEXT("연계 찌르기 %d타"), NextIndex + 1));
			}
		}
		else
		{
			ESpearAttackDirection CurrentDir = OwnerCharacter->GetAttackDirection();
			FSpearComboData* NextComboData = OwnerCharacter->GetComboDataByDirection(CurrentDir);
			if (NextComboData && NextComboData->Stages.Num() > 0 && !NextComboData->Stages[0].AttackName.IsEmpty())
			{
				NextAttackNameText = NextComboData->Stages[0].AttackName;
			}
			else
			{
				NextAttackNameText = FText::FromString(TEXT("연계 찌르기"));
			}
		}

		Entries.Add(FComboGuideEntry(InputText, NextAttackNameText));
	}
	// 2. 비공격 상태 또는 평상시 가이드
	else
	{
		ESpearAttackDirection CurrentDir = OwnerCharacter->GetAttackDirection();

		// 달리기(스프린트) 상태인 경우 -> 대시 내려찍기 우선 가이드
		if (CurrentDir == ESpearAttackDirection::Sprint)
		{
			FSpearComboData* SprintCombo = OwnerCharacter->GetComboDataByDirection(ESpearAttackDirection::Sprint);
			FText SprintAttackName = (SprintCombo && SprintCombo->Stages.Num() > 0 && !SprintCombo->Stages[0].AttackName.IsEmpty())
				? SprintCombo->Stages[0].AttackName : FText::FromString(TEXT("대시 내려찍기"));
			Entries.Add(FComboGuideEntry(FText::FromString(TEXT("[Shift + LMB]")), SprintAttackName));
		}
		// 뒤로 이동(Backward / S 키) 상태인 경우 -> 후퇴 찌르기 안내
		else if (CurrentDir == ESpearAttackDirection::Backward)
		{
			FSpearComboData* BackCombo = OwnerCharacter->GetComboDataByDirection(ESpearAttackDirection::Backward);
			FText BackAttackName = (BackCombo && BackCombo->Stages.Num() > 0 && !BackCombo->Stages[0].AttackName.IsEmpty())
				? BackCombo->Stages[0].AttackName : FText::FromString(TEXT("후퇴 찌르기"));
			Entries.Add(FComboGuideEntry(FText::FromString(TEXT("[S + LMB / 후퇴]")), BackAttackName));
		}
		// 전진 이동(Forward / W 키) 상태인 경우 -> 전진 찌르기 안내
		else if (CurrentDir == ESpearAttackDirection::Forward)
		{
			FSpearComboData* ForwardCombo = OwnerCharacter->GetComboDataByDirection(ESpearAttackDirection::Forward);
			FText ForwardName = (ForwardCombo && ForwardCombo->Stages.Num() > 0 && !ForwardCombo->Stages[0].AttackName.IsEmpty())
				? ForwardCombo->Stages[0].AttackName : FText::FromString(TEXT("전진 찌르기"));
			Entries.Add(FComboGuideEntry(FText::FromString(TEXT("[W + LMB / 전진]")), ForwardName));
		}
		// 정지 상태인 경우 -> 정지 찌르기(Neutral) 안내
		else
		{
			FSpearComboData* NeutralCombo = OwnerCharacter->GetComboDataByDirection(ESpearAttackDirection::Neutral);
			FText NeutralName = (NeutralCombo && NeutralCombo->Stages.Num() > 0 && !NeutralCombo->Stages[0].AttackName.IsEmpty())
				? NeutralCombo->Stages[0].AttackName : FText::FromString(TEXT("찌르기"));
			Entries.Add(FComboGuideEntry(FText::FromString(TEXT("[LMB / 좌클릭]")), NeutralName));
		}

		// 좌+우클릭 동시 입력 스킬 콤보가 데이터 테이블에 존재하는지 확인하여 가이드 항목으로 함께 표시
		FSpearComboData* SkillCombo = OwnerCharacter->GetComboDataByDirectionAndInput(CurrentDir, ESpearAttackInput::Both_LMB_RMB);
		if (!SkillCombo && CurrentDir != ESpearAttackDirection::Neutral)
		{
			SkillCombo = OwnerCharacter->GetComboDataByDirectionAndInput(ESpearAttackDirection::Neutral, ESpearAttackInput::Both_LMB_RMB);
		}

		if (SkillCombo && SkillCombo->Stages.Num() > 0)
		{
			FText SkillName = !SkillCombo->Stages[0].AttackName.IsEmpty() 
				? SkillCombo->Stages[0].AttackName : FText::FromString(TEXT("특수 스킬"));
			Entries.Add(FComboGuideEntry(FText::FromString(TEXT("[LMB + RMB]")), SkillName));
		}
	}

	OnComboGuideUpdated.Broadcast(Entries, CurrentAttackName);
}
