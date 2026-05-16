# Dedicated Server (IP) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace Steam listen-server multiplayer with IP-based Dedicated Server, enabling local/cloud testing without Steam accounts.

**Architecture:** Create a `BlasterServer` build target producing a headless DS executable. Switch `DefaultEngine.ini` from Steam to Null OnlineSubsystem with IpNetDriver. Replace Steam session logic in Menu with direct IP `ClientTravel`. Keep Lobby flow and all gameplay replication unchanged.

**Tech Stack:** Unreal Engine 5.1 (GitHub source build), C++, Null OnlineSubsystem, IpNetDriver

---

## File Map

| Action | File | Responsibility |
|--------|------|----------------|
| Create | `Source/BlasterServer/BlasterServer.Target.cs` | DS build target — produces headless server executable |
| Create | `Source/BlasterServer/BlasterServer.Build.cs` | DS module build config — links to Blaster game module |
| Modify | `Config/DefaultEngine.ini` | Switch to Null subsystem, IpNetDriver as primary, remove Steam config |
| Modify | `Blaster.uproject` | Remove OnlineSubsystemSteam plugin reference |
| Modify | `Plugins/MultiplayerSessions/MultiplayerSessions.uplugin` | Remove OnlineSubsystemSteam plugin dependency |
| Modify | `Plugins/MultiplayerSessions/Source/MultiplayerSessions/MultiplayerSessions.Build.cs` | Remove OnlineSubsystemSteam dependency |
| Modify | `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Public/MultiplayerSessionsSubsystem.h` | Remove all Steam session code, simplify to thin wrapper |
| Modify | `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/MultiplayerSessionsSubsystem.cpp` | Remove all Steam session implementation |
| Modify | `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Public/Menu.h` | Add IPAddressField, remove session callback declarations |
| Modify | `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/Menu.cpp` | Replace session logic with IP direct connect |

---

### Task 1: Create Dedicated Server Build Target

**Files:**
- Create: `Source/BlasterServer/BlasterServer.Target.cs`
- Create: `Source/BlasterServer/BlasterServer.Build.cs`

- [ ] **Step 1: Create the BlasterServer directory**

Run: `mkdir -p Source/BlasterServer`

- [ ] **Step 2: Create `Source/BlasterServer/BlasterServer.Target.cs`**

```csharp
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BlasterServerTarget : TargetRules
{
	public BlasterServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
		ExtraModuleNames.Add("Blaster");
		bIsServer = true;
		bUseChecksInShipping = false;
	}
}
```

- [ ] **Step 3: Create `Source/BlasterServer/BlasterServer.Build.cs`**

```csharp
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BlasterServer : ModuleRules
{
	public BlasterServer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem" });
	}
}
```

- [ ] **Step 4: Commit**

```bash
git add Source/BlasterServer/BlasterServer.Target.cs Source/BlasterServer/BlasterServer.Build.cs
git commit -m "feat: add BlasterServer dedicated server build target"
```

---

### Task 2: Switch DefaultEngine.ini from Steam to Null/IP

**Files:**
- Modify: `Config/DefaultEngine.ini`

- [ ] **Step 1: Replace the NetDriverDefinitions line (line 74)**

Old:
```ini
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="OnlineSubsystemUtils.IpNetDriver")
```

New:
```ini
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="OnlineSubsystemUtils.IpNetDriver")
```

- [ ] **Step 2: Replace the OnlineSubsystem section (lines 76-77)**

Old:
```ini
[OnlineSubsystem]
DefaultPlatformService=Steam
```

New:
```ini
[OnlineSubsystem]
DefaultPlatformService=Null
```

- [ ] **Step 3: Remove the entire OnlineSubsystemSteam section (lines 79-82)**

Delete these lines:
```ini
[OnlineSubsystemSteam]
bEnabled=true
SteamDevAppId=480
bInitServerOnClient=true
```

- [ ] **Step 4: Remove the SteamNetDriver section (lines 84-85)**

Delete these lines:
```ini
[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="OnlineSubsystemSteam.SteamNetConnection"
```

- [ ] **Step 5: Verify the IpNetDriver section remains intact (lines 87-88)**

Confirm these lines are still present:
```ini
[/Script/OnlineSubsystemUtils.IpNetDriver]
NetServerMaxTickRate=60
```

- [ ] **Step 6: Commit**

```bash
git add Config/DefaultEngine.ini
git commit -m "feat: switch networking from Steam to Null subsystem with IpNetDriver"
```

---

### Task 3: Remove Steam Plugin Dependencies

**Files:**
- Modify: `Blaster.uproject`
- Modify: `Plugins/MultiplayerSessions/MultiplayerSessions.uplugin`
- Modify: `Plugins/MultiplayerSessions/Source/MultiplayerSessions/MultiplayerSessions.Build.cs`

- [ ] **Step 1: Remove OnlineSubsystemSteam from `Blaster.uproject`**

In the `"Plugins"` array, remove this entry:
```json
{
    "Name": "OnlineSubsystemSteam",
    "Enabled": true
}
```

The file becomes:
```json
{
	"FileVersion": 3,
	"EngineAssociation": "5.1",
	"Category": "",
	"Description": "",
	"Modules": [
		{
			"Name": "Blaster",
			"Type": "Runtime",
			"LoadingPhase": "Default",
			"AdditionalDependencies": [
				"Engine",
				"UMG"
			]
		}
	],
	"Plugins": [
		{
			"Name": "ModelingToolsEditorMode",
			"Enabled": true,
			"TargetAllowList": [
				"Editor"
			]
		}
	]
}
```

- [ ] **Step 2: Remove OnlineSubsystemSteam from `MultiplayerSessions.uplugin`**

In the `"Plugins"` array, remove:
```json
{
    "Name": "OnlineSubsystemSteam",
    "Enabled": true
}
```

The file becomes:
```json
{
	"FileVersion": 3,
	"Version": 1,
	"VersionName": "1.0",
	"FriendlyName": "MultiplayerSessions",
	"Description": "A plugin for handling online multiplayer sessions",
	"Category": "Other",
	"CreatedBy": "Stephen Ulibarri",
	"CreatedByURL": "",
	"DocsURL": "",
	"MarketplaceURL": "",
	"SupportURL": "",
	"CanContainContent": true,
	"IsBetaVersion": false,
	"IsExperimentalVersion": false,
	"Installed": false,
	"Modules": [
		{
			"Name": "MultiplayerSessions",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	],
	"Plugins": [
		{
			"Name": "OnlineSubsystem",
			"Enabled": true
		}
	]
}
```

- [ ] **Step 3: Remove `OnlineSubsystemSteam` from `MultiplayerSessions.Build.cs`**

In `PublicDependencyModuleNames`, remove the line `"OnlineSubsystemSteam",`.

The array becomes:
```csharp
PublicDependencyModuleNames.AddRange(
    new string[]
    {
        "Core",
        "OnlineSubsystem",
        "UMG",
        "Slate",
        "SlateCore"
    }
    );
```

- [ ] **Step 4: Commit**

```bash
git add Blaster.uproject Plugins/MultiplayerSessions/MultiplayerSessions.uplugin Plugins/MultiplayerSessions/Source/MultiplayerSessions/MultiplayerSessions.Build.cs
git commit -m "feat: remove OnlineSubsystemSteam plugin dependencies"
```

---

### Task 4: Gut MultiplayerSessionsSubsystem — Remove Steam Session Code

**Files:**
- Modify: `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Public/MultiplayerSessionsSubsystem.h`
- Modify: `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/MultiplayerSessionsSubsystem.cpp`

This task replaces the entire Steam session subsystem with a minimal stub that compiles but does nothing. The actual connection logic will be in Menu (Task 5).

- [ ] **Step 1: Replace `MultiplayerSessionsSubsystem.h` with minimal version**

```cpp
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
```

- [ ] **Step 2: Replace `MultiplayerSessionsSubsystem.cpp` with minimal version**

```cpp
#include "MultiplayerSessionsSubsystem.h"
```

- [ ] **Step 3: Commit**

```bash
git add Plugins/MultiplayerSessions/Source/MultiplayerSessions/Public/MultiplayerSessionsSubsystem.h Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/MultiplayerSessionsSubsystem.cpp
git commit -m "feat: gut MultiplayerSessionsSubsystem, remove Steam session code"
```

---

### Task 5: Rewrite Menu for IP Direct Connect

**Files:**
- Modify: `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Public/Menu.h`
- Modify: `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/Menu.cpp`

- [ ] **Step 1: Replace `Menu.h` with IP-based version**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu.generated.h"

/**
 * UMenu — main menu widget for IP-based Dedicated Server connection.
 * Host: starts a listen server on the Lobby map (for single-machine testing).
 * Join: connects to the DS IP address entered in the text field.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/Maps/Lobby")));

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* IPAddressField;

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	void MenuTearDown();

	FString PathToLobby{TEXT("")};
	int32 NumPublicConnections{4};
	FString MatchType{TEXT("FreeForAll")};
};
```

- [ ] **Step 2: Replace `Menu.cpp` with IP-based version**

```cpp
#include "Menu.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	if (IPAddressField)
	{
		IPAddressField->SetText(FText::FromString(TEXT("127.0.0.1")));
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	Super::NativeDestruct();
	MenuTearDown();
}

void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	UWorld* World = GetWorld();
	if (World)
	{
		World->ServerTravel(PathToLobby);
	}
}

void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);

	FString IPAddress = TEXT("127.0.0.1");
	if (IPAddressField)
	{
		IPAddress = IPAddressField->GetText().ToString();
		if (IPAddress.IsEmpty())
		{
			IPAddress = TEXT("127.0.0.1");
		}
	}

	FString Address = FString::Printf(TEXT("%s"), *IPAddress);

	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (PlayerController)
	{
		PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
```

- [ ] **Step 3: Commit**

```bash
git add Plugins/MultiplayerSessions/Source/MultiplayerSessions/Public/Menu.h Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/Menu.cpp
git commit -m "feat: rewrite Menu for IP-based direct connect to Dedicated Server"
```

---

### Task 6: Update WBP_Menu Blueprint — Add IP Address Field

**Files:**
- Modify: `Content/Blueprints/WBP_Menu.uasset` (via UE Editor)

This task must be done manually in the Unreal Editor because `.uasset` files are binary and cannot be edited as text.

- [ ] **Step 1: Open UE Editor, navigate to `Content/Blueprints/WBP_Menu`**

- [ ] **Step 2: Open the Widget Blueprint and add an `EditableTextBox` widget**

- Place it between or near the Join button
- Name it exactly `IPAddressField` (must match the `BindWidget` meta in `Menu.h`)
- Set default text to `127.0.0.1`
- Set size/position to match the existing UI style

- [ ] **Step 3: Save the blueprint**

- [ ] **Step 4: Commit**

```bash
git add Content/Blueprints/WBP_Menu.uasset
git commit -m "feat: add IPAddressField to WBP_Menu blueprint"
```

---

### Task 7: Build and Test

**Files:** None (verification only)

- [ ] **Step 1: Generate project files**

Right-click `Blaster.uproject` -> "Generate Visual Studio project files"

- [ ] **Step 2: Build the client target**

In Visual Studio, build `Development Editor | Win64` configuration.
Expected: Build succeeds with no errors.

- [ ] **Step 3: Build the server target**

In Visual Studio, build `Development Server | Win64` configuration.
Expected: Build succeeds, `Binaries/Win64/BlasterServer.exe` is created.

- [ ] **Step 4: Start the Dedicated Server**

```bash
./Binaries/Win64/BlasterServer.exe /Game/Maps/Lobby?listen -log
```

Expected: Server starts, console log shows `Lobby` map loaded and listening on port 7777.

- [ ] **Step 5: Start the client and connect**

1. Open the game client in UE Editor (Play as Standalone Game) or run the built client
2. On the Menu, verify the IP address field shows `127.0.0.1`
3. Click Join
4. Expected: Client connects to the DS and arrives on the Lobby map

- [ ] **Step 6: Test two-player flow**

1. Start DS: `./Binaries/Win64/BlasterServer.exe /Game/Maps/Lobby?listen -log`
2. Start Client 1 (Editor or built): input `127.0.0.1`, click Join
3. Start Client 2 (another Editor instance or built): input `127.0.0.1`, click Join
4. Expected: Both clients arrive on Lobby, after 2 players `LobbyGameMode` travels to BlasterMap, match begins

- [ ] **Step 7: Final commit**

```bash
git add -A
git commit -m "chore: DS migration complete — tested and verified"
```

---

## Post-Implementation Notes

### How to run the DS
```bash
# Local testing
./Binaries/Win64/BlasterServer.exe /Game/Maps/Lobby?listen -log

# Alibaba Cloud (bind all interfaces)
./Binaries/Win64/BlasterServer.exe /Game/Maps/Lobby?listen -log -MultiHome=0.0.0.0
```

### Cloud deployment checklist
- Open port 7777 (UDP) on the cloud firewall
- Upload the entire built `Binaries/Win64/` directory and cooked `Content/` to the server
- Run the DS executable on the server
- Clients enter the server's public IP to connect

### Restoring Steam later (if needed)
1. Revert `DefaultEngine.ini` Steam sections
2. Restore `Blaster.uproject` and `.uplugin` plugin entries
3. Restore `MultiplayerSessionsSubsystem.h/.cpp` from git history
4. Restore `Menu.h/.cpp` from git history
