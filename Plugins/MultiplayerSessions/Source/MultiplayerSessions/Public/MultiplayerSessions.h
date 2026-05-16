// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * ============================================================================
 * FMultiplayerSessionsModule —— 插件模块入口
 * ============================================================================
 *
 * 本模块是 MultiplayerSessions 插件的 UE 模块注册入口。
 * 继承自 IModuleInterface，由 IMPLEMENT_MODULE 宏在引擎启动时自动加载。
 *
 * 当前 StartupModule / ShutdownModule 均为空实现，因为核心功能
 * 由 UMultiplayerSessionsSubsystem（UGameInstanceSubsystem）承载，
 * 其生命周期由引擎自动管理。
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMultiplayerSessionsModule : public IModuleInterface
{
public:

	/** 模块加载时调用（当前无自定义初始化逻辑） */
	virtual void StartupModule() override;
	/** 模块卸载时调用（当前无自定义清理逻辑） */
	virtual void ShutdownModule() override;
};
