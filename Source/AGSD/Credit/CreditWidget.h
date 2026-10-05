#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreditWidget.generated.h"


UENUM(BlueprintType)
enum class EEndingCreditType : uint8
{
	PartAndName  UMETA(DisplayName = "Part and Name"), // 파트 & 이름
	QA_TwoColumn UMETA(DisplayName = "QA (2-Column)"),  // QA 2열 배치
	License      UMETA(DisplayName = "License"),        // 1줄 라이선스/에셋 정보
	SectionTitle UMETA(DisplayName = "Section Title"),  // 대제목 (예: SPECIAL THANKS)
	ImageOnly    UMETA(DisplayName = "Image Only")      // 타이틀/엔진 로고 등 이미지
};

USTRUCT(BlueprintType)
struct FEndingCreditDataBase : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndingCredit")
	EEndingCreditType EntryType = EEndingCreditType::PartAndName;

	// 텍스트 1 (직책, QA 좌측 이름, 라이선스 내용, 큰 텍스트 파일)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndingCredit")
	FText PrimaryText;

	// 텍스트 2 (이름, QA 우측 이름, 라이선스 제작자)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndingCredit")
	FText SecondaryText;

	// 이미지 전용 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndingCredit")
	TObjectPtr<UTexture2D> EntryImage = nullptr;
};


UCLASS()
class AGSD_API UCreditWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UScrollBox> CreditScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UVerticalBox> CreditVerticalBox;

	// 데이터 테이블 및 자식 위젯 클래스 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "자체설정")
	TObjectPtr<class UDataTable> CreditDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "자체설정")
	TSubclassOf<class UUserWidget> EntryWidgetClass;

	// 스크롤 속도 (초당 픽셀 이동 수치)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "자체설정")
	float ScrollSpeed = 80.0f;

	// 스크롤 멈춤 후 메인 메뉴 이동까지 대기 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EndingCredit|Config")
	float FinishDelay = 3.0f;
	
	// 자동 스크롤 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "자체설정")
	bool bIsScrolling = false;
	
	// 크레딧 종료 후 이동할 메인 메뉴 레벨 이름 (에디터 디테일 패널에서 설정 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "자체설정")
	TSoftObjectPtr<class UWorld> MainMenuLevel;
	
private:
	float CurrentScrollOffset = 0.0f;
	FTimerHandle FinishTimerHandle;
	bool bIsHoldingAtCenter = false;

	void BuildCreditEntries();
	
	void EndEndingCredit();
};
