// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"
#include "Net/UnrealNetwork.h"
#include "../PlayerState/BlasterPlayerState.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);

}
#pragma optimize("", off)

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* PlayerState)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(PlayerState);
		TopScore = PlayerState->GetScore();
	}
	else if (PlayerState->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(PlayerState);
	}
	else if (PlayerState->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.Add(PlayerState);
		TopScore = PlayerState->GetScore();
	}
}
#pragma optimize("", on)

