#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TextLog.generated.h"

UCLASS()
class LOGDATA_API UTextLog : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    
	// 키워드 값만 있으면 가능한 로그 작성 함수
	UFUNCTION(BlueprintCallable, Category = "LogData_TextLogger")
	static void WriteTextLogByKeyword(const FString& Keyword);
	
	/** 문자열(String) 형태의 값을 받아 로그를 작성합니다. */
	UFUNCTION(BlueprintCallable, Category = "LogData_TextLogger")
	static void WriteTextLogByString(const FString& Keyword, const FString& Value);
    
	/** 실수(Float) 형태의 값을 받아 로그를 작성합니다. */
	UFUNCTION(BlueprintCallable, Category = "LogData_TextLogger")
	static void WriteTextLogByFloat(const FString& Keyword, float Value);
	
	/** 문자열(String) + 실수(Float) 형태의 값을 받아 로그를 작성합니다. */
	UFUNCTION(BlueprintCallable, Category = "LogData_TextLogger")
	static void WriteTextLogByStringAndFloat(const FString& Keyword, const FString& StringValue, float FloatValue);
	
};