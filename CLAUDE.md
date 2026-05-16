# Blaster Project Overview

## 项目定位
- 项目名：`Blaster`
- 引擎版本：`Unreal Engine 5.1`
- 类型：`C++ + Blueprint` 混合开发的多人射击项目
- 联机方案：启用了 `OnlineSubsystemSteam`，Steam Dev App ID 为 `480`
- 主要特征：角色移动/瞄准/开火/换弹、武器系统、投射物、HUD、多人 Session/Lobby/对战地图流程

## 目录结构
- `Source/Blaster/`：主游戏 C++ 代码
  - `BlasterComponents/`：组件逻辑，当前核心是 `CombatComponent`
  - `Character/`：角色与动画实例
  - `Weapon/`：武器、弹壳、投射物、武器类型
  - `GameMode/`：比赛流程、Lobby 流程
  - `GameState/`：比赛状态同步
  - `PlayerController/`：玩家控制器与 HUD 同步逻辑
  - `PlayerState/`：玩家状态数据
  - `HUD/`：HUD、角色覆盖层、公告、头顶组件
  - `BlasterTypes/`：枚举/类型定义，如 `CombatState`、`TurningInPlace`
  - `Interfaces/`：交互接口，如 `InteractWithCrosshairsInterface`
- `Plugins/MultiplayerSessions/`：联机 Session 插件，负责创建/查找/加入/销毁房间
- `Content/Blueprints/`：与 C++ 配合使用的蓝图资源
- `Content/Maps/`：地图资源
  - `GameStartUpMap`
  - `Lobby`
  - `BlasterMap`
  - `TransitionMap`
- `Config/`：项目配置、输入、引擎、打包、在线子系统配置

## 核心系统概览

### 1. 角色系统
核心类：`Source/Blaster/Character/BlasterCharacter.h`
- 角色继承自 `ACharacter`
- 负责输入绑定、移动、视角、装备、瞄准、开火、换弹、受伤、淘汰、重生前状态处理
- 角色身上挂载 `CombatComponent` 处理战斗逻辑
- 包含动画蒙太奇、原地转向、Aim Offset、淘汰溶解效果、淘汰特效/音效等逻辑
- 使用复制与 RPC 支持多人同步

### 2. 战斗组件
核心类：`Source/Blaster/BlasterComponents/CombatComponent.h`
- 负责装备武器、瞄准、开火、换弹、准星 HUD、FOV 插值、携带弹药管理
- 包含 `ServerFire`、`NetMulticastFire`、`ServerReload`、`ServerSetAiming` 等 RPC
- 当前 `CombatState` 至少包含：
  - `ECS_Unoccupied`
  - `ECS_Reloading`

### 3. 武器系统
核心目录：`Source/Blaster/Weapon/`
- `Weapon`：基础武器类
- `ProjectileWeapon`：发射投射物的武器
- `Projectile` / `ProjectileBullet` / `ProjectileRocket`：不同投射物实现
- `Casing`：弹壳
- `WeaponTypes.h`：武器类型定义

### 4. 比赛流程
核心类：`Source/Blaster/GameMode/BlasterGameMode.h`
- 包含 Warmup / Match / Cooldown 三阶段时间控制
- 提供玩家淘汰与重生接口：
  - `PlayerEliminated(...)`
  - `RequestRespawn(...)`
- `LobbyGameMode` 用于大厅流程

### 5. 联机 Session
核心位置：`Plugins/MultiplayerSessions/`
- `UMultiplayerSessionsSubsystem` 继承自 `UGameInstanceSubsystem`
- 提供：
  - `CreateSession`
  - `FindSessions`
  - `JoinSession`
  - `DestroySession`
  - `StartSession`
- 使用自定义委托向 UI/Menu 回调 Session 结果

## 关键配置

### 地图配置
`Config/DefaultEngine.ini`
- `GameDefaultMap=/Game/Maps/GameStartUpMap`
- `EditorStartupMap=/Game/Maps/GameStartUpMap`
- `TransitionMap=/Game/Maps/TransitionMap`
- `ServerDefaultMap=/Game/Maps/BlasterMap`

### OnlineSubsystem / Steam
`Config/DefaultEngine.ini`
- `DefaultPlatformService=Steam`
- `bEnabled=true`
- `SteamDevAppId=480`
- 游戏网络驱动优先使用 `SteamNetDriver`

### 输入配置
`Config/DefaultInput.ini`
当前主要键位：
- `WASD`：移动
- `MouseX / MouseY`：视角
- `Space`：跳跃
- `E`：装备
- `LeftShift`：蹲伏
- `RightMouseButton`：瞄准
- `LeftMouseButton`：开火
- `R`：换弹

## 资源组织习惯
- `Content/Assets/`：美术资源、动画、材质、音效、角色、武器等
- `Content/Blueprints/`：按功能分类的蓝图资源
- `Content/Maps/`：地图
- 当前项目明显是“C++ 提供底层逻辑 + Blueprint 负责资源装配与表现”的组织方式

## 当前代码状态观察
- 项目已具备多人射击项目的基础骨架
- 当前打开文件 `CombatState.h` 很精简，说明部分系统仍处于逐步扩展阶段
- `Blaster.Build.cs` 目前依赖较少，主模块公开依赖为：
  - `Core`
  - `CoreUObject`
  - `Engine`
  - `InputCore`
- `UMG` 通过 `.uproject` 模块附加依赖启用

## 新会话快速切入建议
如果是第一次接手这个项目，优先查看以下文件：
1. `Source/Blaster/Character/BlasterCharacter.h`
2. `Source/Blaster/BlasterComponents/CombatComponent.h`
3. `Source/Blaster/Weapon/Weapon.h`
4. `Source/Blaster/GameMode/BlasterGameMode.h`
5. `Plugins/MultiplayerSessions/Source/MultiplayerSessions/Public/MultiplayerSessionsSubsystem.h`
6. `Config/DefaultEngine.ini`
7. `Config/DefaultInput.ini`

## 后续维护建议
- 当新增核心玩法时，同步更新本文件中的“核心系统概览”
- 当修改默认地图、联机方案、输入键位时，同步更新“关键配置”
- 当增加新的主要模块或插件时，把目录入口补充到“目录结构”
- 如果后续你愿意，我可以把这个文件继续升级成：
  - 面向开发者的长期项目记忆文件
  - 包含常见调试入口/多人测试步骤/代码约定的团队说明文件
  - 同时补一个更精简的 `README.md`
