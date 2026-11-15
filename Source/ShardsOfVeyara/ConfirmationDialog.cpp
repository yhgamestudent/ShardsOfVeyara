// Fill out your copyright notice in the Description page of Project Settings.


#include "ConfirmationDialog.h"

#include "Components/Button.h"

void UConfirmationDialog::NativeConstruct()
{
	Super::NativeConstruct();

	NoButton->OnClicked.AddDynamic(this, &UConfirmationDialog::OnClickCloseButton);
	CloseButton->OnClicked.AddDynamic(this, &UConfirmationDialog::OnClickCloseButton);
}

void UConfirmationDialog::OnClickCloseButton()
{
	NoButton->OnClicked.RemoveAll(this);
	CloseButton->OnClicked.RemoveAll(this);
	YesButton->OnClicked.Clear();
	RemoveFromParent();
}
