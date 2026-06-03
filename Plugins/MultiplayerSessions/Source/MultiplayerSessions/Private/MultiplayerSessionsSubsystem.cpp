// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

// ============================================================
// 构造函数
// ============================================================
// 在构造函数中完成两件事：
//   1. 将 5 个内部委托（OnXxxComplete 回调）绑定到对应的成员函数
//   2. 获取当前平台的 OnlineSubsystem，并从中取出 SessionInterface 缓存
//
// 委托绑定使用 CreateUObject，确保回调在子系统对象上执行。
// SessionInterface 是 IOnlineSessionPtr（TSharedPtr 的别名），后续所有
// 会话操作（创建/搜索/加入/销毁）都通过它调用。
UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
}

// ============================================================
// CreateSession —— 创建联机会话
// ============================================================
// 流程：
//   1. 安全检查：SessionInterface 是否有效
//   2. 检查是否已有同名会话（NAME_GameSession），若存在则走"销毁后重建"路径
//   3. 注册创建完成委托到 SessionInterface
//   4. 配置会话设置（FOnlineSessionSettings）：
//      - bIsLANMatch: 当使用 NULL 子系统（本地测试）时为 true，否则为 false
//      - NumPublicConnections: 房间最大公开连接数
//      - bAllowJoinInProgress / bAllowJoinViaPresence / bShouldAdvertise / bUsesPresence:
//        允许游戏中间加入、通过 Presence 加入、广播到服务器列表、使用 Presence
//      - MatchType: 自定义键值对，通过 ViaOnlineServiceAndPing 广播，用于搜索过滤
//      - bUseLobbiesIfAvailable: Steam 平台使用 Lobby 系统
//   5. 调用 SessionInterface->CreateSession，若同步调用失败则清理委托并广播失败
void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	// 检查是否已存在会话，若存在则先销毁，销毁成功后自动重建
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
	}

	// 将创建完成委托注册到 SessionInterface 的委托列表，获取句柄以便后续移除
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// 配置会话设置
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	// NULL 子系统表示本地单机测试，使用 LAN 模式；Steam 等在线子系统使用在线模式
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	// 设置自定义 MatchType 键值，搜索时用此值过滤房间
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;
	LastSessionSettings->bUseLobbiesIfAvailable = true;

	// 获取本地玩家的唯一网络 ID，用于标识会话创建者
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// 调用底层 CreateSession（异步），若返回 false 表示同步阶段失败
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		// 同步失败：清理已注册的委托句柄
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// 广播失败结果给所有监听者
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

// ============================================================
// FindSessions —— 搜索可用的联机会话
// ============================================================
// 流程：
//   1. 安全检查
//   2. 注册搜索完成委托
//   3. 配置搜索参数：
//      - MaxSearchResults: 限制返回结果数量
//      - bIsLanQuery: 同 CreateSession 的 LAN 判断逻辑
//      - QuerySettings: 要求搜索结果 Presence 为 true（即只找在线会话）
//   4. 调用 SessionInterface->FindSessions
void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// 配置搜索对象
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	// NULL 子系统使用 LAN 搜索，否则使用在线搜索
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	// 仅搜索设置了 Presence 的会话（UE 5.7 已移除 SEARCH_PRESENCE 宏，改用 bUsesPresence 过滤）

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// 调用底层 FindSessions（异步），若返回 false 表示同步阶段失败
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		// 广播空结果与失败标记
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

// ============================================================
// JoinSession —— 加入指定的会话
// ============================================================
// 流程：
//   1. 安全检查，无效时直接广播错误
//   2. 注册加入完成委托
//   3. 调用 SessionInterface->JoinSession
void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// 调用底层 JoinSession（异步），若返回 false 表示同步阶段失败
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

// ============================================================
// DestroySession —— 销毁当前会话
// ============================================================
// 流程：
//   1. 安全检查
//   2. 注册销毁完成委托
//   3. 调用 SessionInterface->DestroySession
// 注意：若 bCreateSessionOnDestroy 为 true，销毁成功后会在 OnDestroySessionComplete 中自动重建
void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

// ============================================================
// StartSession —— 开始当前会话（预留接口，暂未实现）
// ============================================================
void UMultiplayerSessionsSubsystem::StartSession()
{
}

// ============================================================
// 内部回调实现
// ============================================================

/**
 * OnCreateSessionComplete —— 底层创建会话完成回调
 *
 * 1. 清理已注册的委托句柄（防止重复调用）
 * 2. 广播自定义委托，通知外部创建结果
 */
void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		// 从 SessionInterface 的委托列表中移除本回调，避免内存泄漏
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	// 向所有绑定了 MultiplayerOnCreateSessionComplete 的对象广播结果
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

/**
 * OnFindSessionsComplete —— 底层搜索会话完成回调
 *
 * 1. 清理委托句柄
 * 2. 若无搜索结果，广播空数组 + false
 * 3. 否则广播实际的搜索结果数组
 */
void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	// 检查搜索结果是否为空
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		// 无结果：广播空数组，标记为失败
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// 有结果：广播完整的搜索结果数组
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

/**
 * OnJoinSessionComplete —— 底层加入会话完成回调
 *
 * 1. 清理委托句柄
 * 2. 广播加入结果（枚举类型，如 Success、SessionIsFull、UnknownError 等）
 */
void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

/**
 * OnDestroySessionComplete —— 底层销毁会话完成回调
 *
 * 1. 清理委托句柄
 * 2. 若销毁成功且 bCreateSessionOnDestroy 为 true，用保存的参数重新创建会话
 * 3. 广播销毁结果
 *
 * 这是"销毁后重建"模式的执行点：CreateSession 发现已有会话时设置标志并销毁，
 * 销毁完成后回调到此处，自动用 LastNumPublicConnections 和 LastMatchType 重建。
 */
void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	// "销毁后重建"：若标志为 true 且销毁成功，使用之前保存的参数重新创建会话
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

/**
 * OnStartSessionComplete —— 底层开始会话完成回调（预留，暂未实现）
 */
void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
