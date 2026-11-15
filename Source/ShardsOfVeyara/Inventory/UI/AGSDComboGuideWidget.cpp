#include "Inventory/UI/AGSDComboGuideWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UAGSDComboGuideWidget::UpdateComboGuide(const TArray<FComboGuideEntry>& GuideEntries, const FText& CurrentAttackName)
{
	// ── 현재 수행 중인 공격 명칭 텍스트 및 페이드아웃 처리 ──
	if (CurrentAttackText)
	{
		if (!CurrentAttackName.IsEmpty())
		{
			// 공격 중: 재생 중이던 페이드아웃 애니메이션 중지 및 opacity 리셋 후 텍스트 표시
			if (CurrentAttackFadeOut && IsAnimationPlaying(CurrentAttackFadeOut))
			{
				StopAnimation(CurrentAttackFadeOut);
			}
			CurrentAttackText->SetRenderOpacity(1.0f);
			CurrentAttackText->SetText(CurrentAttackName);
			CurrentAttackText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			bWasAttacking = true;
		}
		else
		{
			// 공격 막 종료됨: 페이드아웃 애니메이션 재생
			if (bWasAttacking)
			{
				bWasAttacking = false;
				if (CurrentAttackFadeOut)
				{
					PlayAnimation(CurrentAttackFadeOut);
				}
				else
				{
					CurrentAttackText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			else if (!CurrentAttackFadeOut || !IsAnimationPlaying(CurrentAttackFadeOut))
			{
				CurrentAttackText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// ── 첫 번째 조작 가이드 항목 ──
	if (GuideEntries.IsValidIndex(0))
	{
		if (ActionInputText)
		{
			ActionInputText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ActionInputText->SetText(GuideEntries[0].InputText);
		}
		if (AttackNameText)
		{
			AttackNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AttackNameText->SetText(GuideEntries[0].AttackNameText);
		}
	}
	else
	{
		if (ActionInputText) ActionInputText->SetVisibility(ESlateVisibility::Collapsed);
		if (AttackNameText) AttackNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ── 두 번째 조작 가이드 항목 ──
	if (GuideEntries.IsValidIndex(1))
	{
		if (SecondaryActionInputText)
		{
			SecondaryActionInputText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SecondaryActionInputText->SetText(GuideEntries[1].InputText);
		}
		if (SecondaryAttackNameText)
		{
			SecondaryAttackNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SecondaryAttackNameText->SetText(GuideEntries[1].AttackNameText);
		}
	}
	else
	{
		if (SecondaryActionInputText) SecondaryActionInputText->SetVisibility(ESlateVisibility::Collapsed);
		if (SecondaryAttackNameText) SecondaryAttackNameText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
