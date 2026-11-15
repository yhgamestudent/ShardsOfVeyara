#include "CreditWidget.h"

#include "CreditEntryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/Spacer.h"
#include "Kismet/GameplayStatics.h"

void UCreditWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentScrollOffset = 0.0f;
	if (CreditScrollBox)
	{
		CreditScrollBox->SetScrollOffset(CurrentScrollOffset);
	}

	// 데이터 테이블 기반 위젯 동적 생성
	BuildCreditEntries();

	bIsScrolling = true;
}

void UCreditWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsScrolling || !CreditScrollBox)
	{
		return;
	}

	// 스크롤 진행
	CurrentScrollOffset += ScrollSpeed * InDeltaTime;
	CreditScrollBox->SetScrollOffset(CurrentScrollOffset);

	// 스크롤 끝(모든 텍스트가 화면 위로 완전히 나간 지점) 도달 검사
	const float EndOffset = CreditScrollBox->GetScrollOffsetOfEnd();
	if (EndOffset > 0.0f && CurrentScrollOffset >= EndOffset)
	{
		bIsScrolling = false;
		EndEndingCredit();
	}
}

void UCreditWidget::BuildCreditEntries()
{
	if (!CreditDataTable || !EntryWidgetClass || !CreditVerticalBox)
	{
		return;
	}

	// 데이터 테이블 전체 행 순회
	static const FString ContextString(TEXT("CreditDataTable Context"));
	TArray<FEndingCreditDataBase*> Rows;
	CreditDataTable->GetAllRows<FEndingCreditDataBase>(ContextString, Rows);

	for (const FEndingCreditDataBase* RowData : Rows)
	{
		if (!RowData)
		{
			continue;
		}

		UCreditEntryWidget* NewEntry = CreateWidget<UCreditEntryWidget>(this, EntryWidgetClass);
		if (NewEntry)
		{
			NewEntry->InitEntryData(*RowData);
			CreditVerticalBox->AddChildToVerticalBox(NewEntry);
		}
	}
	
	// 모든 크레딧이 화면 밖으로 완전히 나갈 수 있도록 하단 종료 여백(Spacer) 복구
	if (USpacer* BottomSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))
	{
		BottomSpacer->SetSize(FVector2D(1.0f, 1440.0f));
		CreditVerticalBox->AddChildToVerticalBox(BottomSpacer);
	}
}

void UCreditWidget::EndEndingCredit()
{
	// 메인 메뉴로 전환
	if (!MainMenuLevel.IsNull())
	{
		const FString LevelPath = MainMenuLevel.GetLongPackageName();
		UGameplayStatics::OpenLevel(this, FName(*LevelPath));
	}
}