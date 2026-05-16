// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * ============================================================================
 * MultiplayerSessions 插件 —— 多人联机 Session 管理子系统
 * ============================================================================
 *
 * 【插件用途】
 * 本插件封装了 Unreal Engine 的 Online Session 接口，为项目提供统一的多人联机会话管理能力。
 * 支持完整的联机生命周期：创建房间 → 搜索房间 → 加入房间 → 销毁房间 → 开始游戏。
 *
 * 【架构概述】
 * 插件分为三层：
 *   1. 模块层 (MultiplayerSessionsModule) —— UE 模块注册入口，无特殊初始化逻辑
 *   2. 子系统层 (UMultiplayerSessionsSubsystem) —— 本文件，核心联机逻辑
 *   3. UI 层 (UMenu) —— 基于 UMG 的主菜单控件，提供 Host/Join 按钮交互
 *
 * 【数据流】
 * 用户点击按钮 → Menu 调用 Subsystem 的公开方法 → Subsystem 调用 Online Session Interface
 * → 异步回调触发 → Subsystem 广播自定义委托 → Menu 回调处理结果（旅行、重新启用按钮等）
 *
 * 【子系统设计】
 * UMultiplayerSessionsSubsystem 继承自 UGameInstanceSubsystem：
 *   - 生命周期与 GameInstance 绑定，整个游戏运行期间始终存在
 *   - 可通过 GetGameInstance()->GetSubsystem<UMultiplayerSessionsSubsystem>() 从任意位置访问
 *   - 内部持有 IOnlineSessionPtr，即平台（Steam/Null）提供的 Session 接口
 *
 * 【委托设计】
 * 定义了 5 个自定义委托，供外部（如 Menu）绑定回调：
 *   - MultiplayerOnCreateSessionComplete   —— 创建房间完成
 *   - MultiplayerOnFindSessionsComplete    —— 搜索房间完成
 *   - MultiplayerOnJoinSessionComplete     —— 加入房间完成
 *   - MultiplayerOnDestroySessionComplete  —— 销毁房间完成
 *   - MultiplayerOnStartSessionComplete    —— 开始游戏完成
 *
 * 其中 Create/Destroy/Start 使用 DECLARE_DYNAMIC_MULTICAST_DELEGATE（支持蓝图绑定），
 * Find/Join 使用 DECLARE_MULTICAST_DELEGATE（仅 C++ 绑定，因为参数类型不兼容蓝图）。
 *
 * 【"销毁后重建"模式】
 * 当调用 CreateSession 时若已有会话存在，子系统会先销毁旧会话，然后在销毁回调中
 * 自动用保存的参数重新创建。通过 bCreateSessionOnDestroy 标志和 Last* 成员实现。
 */

// ============================================================
// 自定义委托声明
// ============================================================

/** 创建房间完成回调。DYNAMIC 版本，支持蓝图绑定 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);

/** 搜索房间完成回调。普通多播委托，参数含搜索结果数组（TArray 不兼容蓝图） */
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

/** 加入房间完成回调。普通多播委托，参数为枚举类型 */
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);

/** 销毁房间完成回调。DYNAMIC 版本，支持蓝图绑定 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);

/** 开始游戏完成回调。DYNAMIC 版本，支持蓝图绑定 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

/**
 * UMultiplayerSessionsSubsystem
 *
 * 多人联机 Session 管理子系统，封装 Online Session 的完整生命周期。
 * 作为 UGameInstanceSubsystem 存在，全局可访问，负责：
 *   - 创建/搜索/加入/销毁/开始 会话
 *   - 管理与底层 Online Session Interface 的委托绑定与清理
 *   - 通过自定义委托向 UI 层广播异步操作结果
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	// ============================================================
	// 公开接口 —— Menu 及其他调用方使用这些方法驱动联机流程
	// ============================================================

	/**
	 * 创建一个新的联机会话
	 * @param NumPublicConnections  房间最大公开连接数（玩家上限）
	 * @param MatchType             比赛类型标识（如 "FreeForAll"），用于搜索时过滤房间
	 *
	 * 内部逻辑：
	 *   - 若已存在会话，先设置"销毁后重建"标志并销毁旧会话
	 *   - 配置会话设置（LAN/在线、Presence、最大连接数等）
	 *   - 调用底层 CreateSession，结果通过 MultiplayerOnCreateSessionComplete 委托广播
	 */
	void CreateSession(int32 NumPublicConnections, FString MatchType);

	/**
	 * 搜索当前可用的联机会话
	 * @param MaxSearchResults  最大搜索结果数量
	 *
	 * 内部逻辑：
	 *   - 配置搜索参数（LAN/在线、Presence 过滤）
	 *   - 调用底层 FindSessions，结果通过 MultiplayerOnFindSessionsComplete 委托广播
	 */
	void FindSessions(int32 MaxSearchResults);

	/**
	 * 加入指定的会话
	 * @param SessionResult  从搜索结果中选择的会话
	 *
	 * 内部逻辑：
	 *   - 调用底层 JoinSession，结果通过 MultiplayerOnJoinSessionComplete 委托广播
	 */
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);

	/**
	 * 销毁当前会话
	 *
	 * 内部逻辑：
	 *   - 调用底层 DestroySession，结果通过 MultiplayerOnDestroySessionComplete 委托广播
	 *   - 若 bCreateSessionOnDestroy 为 true，销毁成功后会自动重新创建会话
	 */
	void DestroySession();

	/** 开始当前会话（预留接口，当前未实现具体逻辑） */
	void StartSession();

	// ============================================================
	// 自定义委托 —— 外部绑定这些委托来接收异步操作结果
	// ============================================================

	/** 创建房间完成时广播 */
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	/** 搜索房间完成时广播，携带搜索结果数组 */
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	/** 加入房间完成时广播，携带结果枚举 */
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	/** 销毁房间完成时广播 */
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	/** 开始游戏完成时广播 */
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected:
	// ============================================================
	// 内部回调 —— 绑定到 Online Session Interface 的委托列表
	// 当底层异步操作完成时被调用，内部处理后广播自定义委托
	// ============================================================

	/** 底层创建会话完成回调 */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	/** 底层搜索会话完成回调 */
	void OnFindSessionsComplete(bool bWasSuccessful);
	/** 底层加入会话完成回调 */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	/** 底层销毁会话完成回调 */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	/** 底层开始会话完成回调 */
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	// ============================================================
	// 底层 Online Session 接口与会话数据
	// ============================================================

	/** 平台（Steam/Null）提供的 Session 接口指针，所有会话操作通过它执行 */
	IOnlineSessionPtr SessionInterface;

	/** 上一次创建会话时使用的会话设置（连接数、MatchType、Presence 等） */
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	/** 上一次搜索会话时使用的搜索对象（包含搜索参数与结果） */
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// ============================================================
	// 委托与句柄 —— 用于注册到底层 Session Interface 的回调
	// ============================================================

	/** 创建会话完成委托（绑定到 OnCreateSessionComplete） */
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	/** 创建会话委托的注册句柄，用于后续从委托列表中移除 */
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	/** 搜索会话完成委托（绑定到 OnFindSessionsComplete） */
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;

	/** 加入会话完成委托（绑定到 OnJoinSessionComplete） */
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	/** 销毁会话完成委托（绑定到 OnDestroySessionComplete） */
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	/** 开始会话完成委托（绑定到 OnStartSessionComplete） */
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	// ============================================================
	// "销毁后重建"状态 —— 当创建会话时已有旧会话存在时使用
	// ============================================================

	/** 标记是否需要在销毁旧会话后自动重新创建 */
	bool bCreateSessionOnDestroy{ false };

	/** 保存的创建参数，用于销毁后重建时传入 */
	int32 LastNumPublicConnections;
	FString LastMatchType;
};
