// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DragEnterWidget.generated.h"

/**
 * UDragEnterWidget
 * WBP_DragEnterWidget 위젯의 C++ 부모 클래스입니다.
 */
UCLASS()
class AGSD_API UDragEnterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alchemy Drag")
	bool bHovered = false;

protected:
	virtual void NativeOnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& DragDropEvent, UDragDropOperation* Operation) override;
};
