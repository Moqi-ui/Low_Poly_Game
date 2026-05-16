// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 * ============================================================================
 * UMenu —— 多人联机主菜单控件
 * ============================================================================
 *
 * 基于 UMG (Unreal Motion Graphics) 的用户控件，提供 Host/Join 按钮界面。
 * 是 MultiplayerSessions 插件的 UI 层，通过 UMultiplayerSessionsSubsystem
 * 驱动联机流程。
 *
 * 【使用方式】
 *   1. 在蓝图中创建继承自本类的 Widget Blueprint
 *   2. 在蓝图中放置名为 "HostButton" 和 "JoinButton" 的 Button 控件
 *      （通过 UPROPERTY meta=(BindWidget) 自动绑定）
 *   3. 调用 MenuSetup() 初始化菜单
 *
 * 【交互流程】
 *   Host 流程：点击 HostButton → 禁用按钮 → 调用 CreateSession →
 *             创建成功 → ServerTravel 到 Lobby 地图
 *
 *   Join 流程：点击 JoinButton → 禁用按钮 → 调用 FindSessions →
 *             搜索完成 → 遍历结果找匹配的 MatchType → 调用 JoinSession →
 *             加入成功 → ClientTravel 到主机地址
 *
 * 【输入模式管理】
 *   MenuSetup 显示菜单时切换到 UIOnly 输入模式（显示鼠标光标）
 *   MenuTearDown 关闭菜单时切换回 GameOnly 输入模式（隐藏鼠标光标）
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	/**
	 * 初始化菜单，由蓝图或 C++ 调用
	 * @param NumberOfPublicConnections  房间最大公开连接数，默认 4
	 * @param TypeOfMatch                比赛类型标识，默认 "FreeForAll"
	 * @param LobbyPath                  Lobby 地图路径，默认指向 Lobby 地图
	 *
	 * 内部操作：
	 *   - 将 LobbyPath 追加 "?listen" 后缀（用于监听服务器模式）
	 *   - 将控件添加到视口并设为可见/可聚焦
	 *   - 切换到 UIOnly 输入模式，显示鼠标光标
	 *   - 获取 MultiplayerSessionsSubsystem 并绑定 5 个联机回调委托
	 */
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/ThirdPersonCPP/Maps/Lobby")));

protected:
	/** UMG 控件初始化回调，绑定按钮点击事件 */
	virtual bool Initialize() override;

	/**
	 * 控件销毁回调（替代已注释的 OnLevelRemovedFromWorld）
	 * 当控件被销毁时自动调用 MenuTearDown 清理输入模式
	 */
	virtual void NativeDestruct() override;

	// ============================================================
	// Subsystem 委托回调 —— 处理联机异步操作的结果
	// ============================================================

	/** 创建房间完成回调：成功则 ServerTravel 到 Lobby，失败则重新启用按钮 */
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	/**
	 * 搜索房间完成回调：遍历搜索结果，找到 MatchType 匹配的房间后调用 JoinSession
	 * 若无结果或搜索失败，重新启用 JoinButton
	 */
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

	/** 加入房间完成回调：通过 SessionInterface 获取服务器地址，ClientTravel 加入 */
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	/** 销毁房间完成回调（预留，暂未实现具体逻辑） */
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	/** 开始游戏完成回调（预留，暂未实现具体逻辑） */
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:
	// ============================================================
	// UI 控件绑定 —— 通过 BindWidget 元数据自动与蓝图中的同名控件关联
	// ============================================================

	/** "主机" 按钮，点击后创建联机房间 */
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	/** "加入" 按钮，点击后搜索并加入联机房间 */
	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	// ============================================================
	// 按钮点击处理
	// ============================================================

	/** Host 按钮点击处理：禁用按钮 → 调用 CreateSession */
	UFUNCTION()
	void HostButtonClicked();

	/** Join 按钮点击处理：禁用按钮 → 调用 FindSessions */
	UFUNCTION()
	void JoinButtonClicked();

	/**
	 * 菜单清理：从视口移除控件、恢复 GameOnly 输入模式、隐藏鼠标光标
	 * 在 NativeDestruct 中自动调用
	 */
	void MenuTearDown();

	// ============================================================
	// 内部状态
	// ============================================================

	/** 联机 Session 子系统指针，负责所有联机操作 */
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	/** 房间最大公开连接数（从 MenuSetup 传入） */
	int32 NumPublicConnections{4};

	/** 比赛类型标识，用于过滤搜索结果（从 MenuSetup 传入） */
	FString MatchType{TEXT("FreeForAll")};

	/** Lobby 地图路径，带 "?listen" 后缀（从 MenuSetup 传入） */
	FString PathToLobby{TEXT("")};
};
