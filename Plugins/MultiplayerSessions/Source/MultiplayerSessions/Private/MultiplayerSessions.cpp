// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * ============================================================================
 * FMultiplayerSessionsModule —— 插件模块实现
 * ============================================================================
 *
 * 模块入口的启动与关闭实现。由于核心联机逻辑由
 * UMultiplayerSessionsSubsystem 承载（生命周期随 GameInstance 自动管理），
 * 此处无需额外的初始化或清理操作。
 */

#include "MultiplayerSessions.h"

#define LOCTEXT_NAMESPACE "FMultiplayerSessionsModule"

void FMultiplayerSessionsModule::StartupModule()
{
	// 模块加载完成后的入口点，当前无需自定义初始化
}

void FMultiplayerSessionsModule::ShutdownModule()
{
	// 模块卸载前的清理入口点，当前无需自定义清理
}

#undef LOCTEXT_NAMESPACE

// 向引擎注册此模块，使其在插件加载时自动初始化
IMPLEMENT_MODULE(FMultiplayerSessionsModule, MultiplayerSessions)
