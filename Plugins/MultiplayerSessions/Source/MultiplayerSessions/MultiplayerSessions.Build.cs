// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * ============================================================================
 * MultiplayerSessions.Build.cs —— 模块构建配置
 * ============================================================================
 *
 * 定义 MultiplayerSessions 模块的编译依赖和包含路径。
 *
 * 【公开依赖 (PublicDependencyModuleNames)】
 *   - Core:              UE 核心模块
 *   - OnlineSubsystem:   在线子系统抽象层，提供 Session/Presence 等接口
 *   - OnlineSubsystemSteam: Steam 平台在线子系统实现
 *   - UMG:               Unreal Motion Graphics，Menu 控件继承自 UUserWidget
 *   - Slate / SlateCore: UI 框架底层，UMG 基于 Slate 构建
 *
 * 【私有依赖 (PrivateDependencyModuleNames)】
 *   - CoreUObject:       UE 反射系统（UCLASS、UPROPERTY 等宏支持）
 *   - Engine:            引擎基础类（APlayerController、UWorld 等）
 *   - Slate / SlateCore: UI 框架（私有编译也需要）
 */

using UnrealBuildTool;

public class MultiplayerSessions : ModuleRules
{
	public MultiplayerSessions(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths here ...
			}
			);

		// 公开依赖：其他模块依赖本模块时也会自动链接这些模块
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"OnlineSubsystem",
				"OnlineSubsystemSteam",
				"UMG",
				"Slate",
				"SlateCore"
				// ... add other public dependencies that you statically link with here ...
			}
			);

		// 私有依赖：仅本模块内部使用，不会传递给依赖本模块的其他模块
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
