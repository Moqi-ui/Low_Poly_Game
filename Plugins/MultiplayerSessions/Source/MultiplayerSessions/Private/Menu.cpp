// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

// ============================================================
// MenuSetup —— 菜单初始化
// ============================================================
// 此方法应在控件创建后、显示前调用（通常由蓝图调用）。
// 主要完成以下工作：
//   1. 保存参数并拼接 Lobby 路径（加 "?listen" 使其成为监听服务器）
//   2. 将控件添加到视口
//   3. 切换输入模式为 UIOnly（仅响应 UI 输入，显示鼠标光标）
//   4. 获取 MultiplayerSessionsSubsystem 并绑定 5 个联机回调
void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	// 拼接 "?listen" 后缀，使 ServerTravel 时以监听服务器模式打开地图
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;

	// 将控件添加到视口并设为可见、可聚焦
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	// 切换到 UIOnly 输入模式：只响应 UI 交互，不传递游戏输入
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

	// 获取联机 Session 子系统实例
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	// 绑定子系统的 5 个自定义委托到对应的回调函数
	if (MultiplayerSessionsSubsystem)
	{
		// DYNAMIC 委托使用 AddDynamic 绑定（支持蓝图）
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		// 普通委托使用 AddUObject 绑定（仅 C++）
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

// ============================================================
// Initialize —— UMG 控件初始化
// ============================================================
// 在控件初始化时绑定两个按钮的点击事件。
// 注意：此时蓝图中的 BindWidget 控件已经完成绑定，可以安全使用。
bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// 绑定 Host 按钮点击事件
	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	// 绑定 Join 按钮点击事件
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

// ============================================================
// NativeDestruct —— 控件销毁回调
// ============================================================
// 当控件从视口移除或关卡卸载时触发，调用 MenuTearDown 清理输入模式。
// 替代了旧版的 OnLevelRemovedFromWorld 方案。
void UMenu::NativeDestruct()
{
	Super::NativeDestruct();
	MenuTearDown();
}

// ============================================================
// OnCreateSession —— 创建房间完成回调
// ============================================================
// 创建成功：以监听服务器模式旅行到 Lobby 地图（ServerTravel）
// 创建失败：打印调试信息并重新启用 Host 按钮
void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// 创建成功：ServerTravel 到 Lobby 地图
		// ServerTravel 将当前玩家变为监听服务器，其他玩家可加入
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		// 创建失败：显示错误信息并重新启用按钮
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("Failed to create session!"))
			);
		}
		HostButton->SetIsEnabled(true);
	}
}

// ============================================================
// OnFindSessions —— 搜索房间完成回调
// ============================================================
// 遍历搜索结果，找到第一个 MatchType 匹配的会话并加入。
// 若无匹配结果或搜索失败，重新启用 Join 按钮。
void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr)
	{
		return;
	}

	// 遍历所有搜索结果，查找 MatchType 匹配的房间
	for (auto Result : SessionResults)
	{
		FString SettingsValue;
		// 从会话设置中读取 MatchType 自定义键
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
		if (SettingsValue == MatchType)
		{
			// 找到匹配的房间，调用 JoinSession 加入
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}

	// 未找到匹配房间或搜索失败：重新启用 Join 按钮
	if (!bWasSuccessful || SessionResults.Num() == 0)
	{
		JoinButton->SetIsEnabled(true);
	}
}

// ============================================================
// OnJoinSession —— 加入房间完成回调
// ============================================================
// 通过 SessionInterface 获取服务器连接地址，然后 ClientTravel 加入。
void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	// 获取 OnlineSubsystem 和 SessionInterface
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// 获取解析后的连接字符串（服务器地址）
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			// 通过 PlayerController 的 ClientTravel 连接到服务器
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

// ============================================================
// OnDestroySession / OnStartSession —— 预留回调（暂未实现）
// ============================================================
void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}

// ============================================================
// HostButtonClicked —— Host 按钮点击处理
// ============================================================
void UMenu::HostButtonClicked()
{
	// 禁用按钮防止重复点击
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		// 调用子系统创建会话，结果通过 OnCreateSession 回调处理
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

// ============================================================
// JoinButtonClicked —— Join 按钮点击处理
// ============================================================
void UMenu::JoinButtonClicked()
{
	// 禁用按钮防止重复点击
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		// 搜索最多 10000 个会话，结果通过 OnFindSessions 回调处理
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

// ============================================================
// MenuTearDown —— 菜单清理
// ============================================================
// 从父控件移除、恢复 GameOnly 输入模式、隐藏鼠标光标。
// 在 NativeDestruct 中自动调用。
void UMenu::MenuTearDown()
{
	// 从视口移除控件
	RemoveFromParent();

	// 恢复 GameOnly 输入模式：仅响应游戏输入，隐藏鼠标光标
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
