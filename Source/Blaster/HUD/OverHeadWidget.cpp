// Fill out your copyright notice in the Description page of Project Settings.


#include "OverHeadWidget.h"
#include "Components/TextBlock.h"

void UOverHeadWidget::NativeDestruct()
{
	Super::NativeDestruct();
	RemoveFromParent();
}

void UOverHeadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	ENetRole LocalRole = InPawn->GetLocalRole();
	//ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;
	switch (LocalRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
		
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("AutomousProxy");
		break;

	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("SimulateProxy");
		break;

	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}
	FString LocalRoleString = FString::Printf(TEXT("Local Role: %s"), *Role);
	SetDisplayText(LocalRoleString);
}

void UOverHeadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

