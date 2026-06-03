#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * MultiplayerSessionsSubsystem — minimal stub.
 * Steam session logic has been removed in favor of IP-based Dedicated Server.
 * Kept as a UGameInstanceSubsystem for potential future extension.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem() {}
};
