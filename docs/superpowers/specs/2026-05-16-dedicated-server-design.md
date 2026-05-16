# Dedicated Server (IP) Design

**Date:** 2026-05-16
**Status:** Approved
**Scope:** Replace Steam listen-server multiplayer with IP-based Dedicated Server

## Background

The project currently uses Steam OnlineSubsystem (App ID 480) for multiplayer. Testing requires two Steam accounts on two machines. The goal is to switch to a pure IP Dedicated Server for easier local/lan testing and future cloud deployment (Alibaba Cloud).

## Requirements

- Pure IP connection, no Steam dependency
- Dedicated Server build target (headless server executable)
- Manual IP input in client UI for connecting
- Keep existing Lobby flow (wait for 2 players, then travel to BlasterMap)
- Support both LAN (local testing) and WAN (cloud deployment)
- Engine: UE 5.1 GitHub source build

## Architecture

```
                    +---------------------+
                    |  BlasterServer.exe   |  <-- headless DS process
                    |  (Lobby -> BlasterMap)|
                    +---------+-----------+
                              | IP:7777
                    +---------+----------+
              +-----+-----+        +-----+-----+
              | Client 1  |        | Client 2  |
              | (IP input)|        | (IP input)|
              +-----------+        +-----------+
```

## New Files

### `Source/BlasterServer/BlasterServer.Target.cs`

Standard UE dedicated server target. Key settings:
- `TargetType = Game;`
- `bIsServer = true;`
- `ExtraModuleNames.Add("Blaster");`
- `bUseChecksInShipping = false;`

### `Source/BlasterServer/BlasterServer.Build.cs`

Minimal build module linking to the Blaster game module.

## Modified Files

### `Config/DefaultEngine.ini`

Changes:
- `DefaultPlatformService` = `Null` (was `Steam`)
- Remove `SteamNetDriver` as primary net driver, use `IpNetDriver` directly
- Remove `[OnlineSubsystemSteam]` section
- Remove `[/Script/OnlineSubsystemSteam.SteamNetDriver]` section
- Remove `bInitServerOnClient=true`
- Keep `[/Script/OnlineSubsystemUtils.IpNetDriver]` with `NetServerMaxTickRate=60`
- Update `NetDriverDefinitions` to use `IpNetDriver` as primary

### `Plugins/MultiplayerSessions/Source/.../Public/Menu.h`

- Add `UPROPERTY(meta = (BindWidget)) UEditableTextBox* IPAddressField;`
- Set default IP to `"127.0.0.1"`

### `Plugins/MultiplayerSessions/Source/.../Private/Menu.cpp`

**Join flow:**
- Read IP from `IPAddressField->GetText().ToString()`
- Construct address: `FString::Printf(TEXT("%s:7777"), *IPAddress)`
- Call `PlayerController->ClientTravel(Address, TRAVEL_Absolute)`
- Remove Steam FindSessions/JoinSession calls

**Host flow:**
- Keep as listen-server shortcut for single-machine testing
- Use `World->ServerTravel(PathToLobby)` without Steam session creation

### `Plugins/MultiplayerSessions/Source/.../Public/MultiplayerSessionsSubsystem.h`
### `Plugins/MultiplayerSessions/Source/.../Private/MultiplayerSessionsSubsystem.cpp`

- Remove `IOnlineSessionPtr` and all Steam session interface code
- Remove Create/Find/Join/Destroy/Start session methods and delegates
- Either gut the class or remove it entirely (Menu can connect directly without it)
- Decision: keep the class as a thin wrapper for potential future extension, but remove Steam dependencies

### `Plugins/MultiplayerSessions/MultiplayerSessions.Build.cs`

- Remove `OnlineSubsystemSteam` from public dependencies
- Keep `OnlineSubsystem` (needed for Null subsystem)

## Unchanged Files

All gameplay code remains untouched:
- `LobbyGameMode` -- PostLogin count + ServerTravel logic unchanged
- `BlasterGameMode` -- match lifecycle unchanged
- `BlasterCharacter` / `CombatComponent` -- replication unchanged
- `BlasterPlayerController` / `BlasterPlayerState` / `BlasterGameState` -- unchanged
- All weapon, HUD, animation code -- unchanged
- Map assets -- unchanged

## Server Startup

```bash
# Local testing
BlasterServer.exe /Game/Maps/Lobby?listen -log

# Cloud deployment (Alibaba Cloud)
BlasterServer.exe /Game/Maps/Lobby?listen -log -MultiHome=0.0.0.0
```

## Client Connection Flow

1. Launch game client -> GameStartUpMap -> Menu widget
2. Enter DS IP address in text field (e.g. `192.168.1.100`)
3. Click Join -> `ClientTravel("192.168.1.100:7777")`
4. Arrive on Lobby map via DS
5. `LobbyGameMode::PostLogin` counts players
6. When 2 players connected -> `ServerTravel("/Game/Maps/BlasterMap?listen")`
7. All players seamlessly travel to BlasterMap, match begins

## Testing Workflow

| Scenario | Steps |
|----------|-------|
| Single machine | Terminal: `BlasterServer.exe /Game/Maps/Lobby?listen -log`; Client: input `127.0.0.1` -> Join |
| LAN two machines | Machine A: run DS + client Join `127.0.0.1`; Machine B: client Join `<Machine A LAN IP>` |
| Cloud | Deploy DS to Alibaba Cloud, open port 7777; Clients: input `<public IP>` -> Join |

## Build Instructions

1. Generate project files (right-click .uproject -> Generate Visual Studio project files)
2. Build `BlasterServer` target in Development Server configuration
3. Build `Blaster` client as usual
4. Server executable output: `Binaries/Win64/BlasterServer.exe`

## Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| Steam-specific code in other files missed | Grep for `OnlineSubsystemSteam`, `Steam`, `SessionInterface` across codebase |
| Default map not loading on DS | Verify `ServerDefaultMap` in DefaultEngine.ini points to BlasterMap, but DS startup uses Lobby via command line |
| Port 7777 blocked on cloud | Document firewall configuration for Alibaba Cloud |
| UPREFIX property replication unaffected | All replication uses UE's built-in system, independent of OnlineSubsystem |
