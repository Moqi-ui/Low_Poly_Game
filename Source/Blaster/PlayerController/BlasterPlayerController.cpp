// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "../Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "../GameMode/BlasterGameMode.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "../BlasterComponents/CombatComponent.h"
#include "../GameState/BlasterGameState.h"
#include "../PlayerState/BlasterPlayerState.h"


/*
* HUD中的倒计时相关执行逻辑
* GameMode只存在于服务端，真正的倒计时计算是在BlasterGameMode中的Tick函数中计算执行的。执行大概顺序如下：
* 1、在BlasterGameMode的构造函数中设置：bDelayedStart = true，这会让游戏状态处于WaitingToStart
* 2、接着会自动执行ABlasterGameMode::OnMatchStateSet()，这里执行PlayerController->OnMatchStateSet。
* 3、PlayerController->OnMatchStateSet。会处理每个游戏状态该显示哪个HUD。
* ABlasterPlayerController::Tick负责实时更新HUD显示和倒计时。
*/


void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	//客户端中途加入时调用
	ServerCheckMatchState();
	/*if (IsLocalController())
	{
		FString str = GetDebugName(this);
		UE_LOG(LogTemp, Display, TEXT("LocalControllerName: %s"), *str);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("LocalControllerName: %s"), *str));
	}
	else
	{
		FString str = GetDebugName(this);
		UE_LOG(LogTemp, Display, TEXT("ServerControllerName: %s"), *str);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ServerControllerName: %s"), *str));
	}*/
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	//定时计算客户端和服务器之间的时间差。
	CheckTimeSync(DeltaTime);

	PollInit();
}
void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}

}
//#pragma optimize("", off)
void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	//玩家重生时,和游戏开始时执行
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}
void ABlasterPlayerController::SetHUDTime()
{
	/*
	* FMath::CeilToInt将浮点数向上取整为整数；
	* FMath::RoundToInt：将浮点数四舍五入为整数。
	* FMath::TruncToInt：将浮点数截断为整数（去掉小数部分）。
	*/

	/*uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetWorld()->GetTimeSeconds());
	if (CountdownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(MatchTime - GetWorld()->GetTimeSeconds());
	}
	CountdownInt = SecondsLeft;*/

	float TimeLeft = 0.f;


	//服务器玩家可以直接获取倒计时
	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode)
		{
			TimeLeft = BlasterGameMode->GetCountdownTime();
		}
	}
	else
	{	//加上LevelStartingTime是因为
		if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmingTime - GetServerTime() + LevelStartingTime;
		else if (MatchState == MatchState::InProgress) TimeLeft = WarmingTime + MatchTime - GetServerTime() + LevelStartingTime;
		else if (MatchState == MatchState::Cooldown) TimeLeft = WarmingTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);


	//为什么不直接写SecondsLeft == 0呢，因为这样写会每帧更新执行里面的逻辑。
	//而CountdownInt != SecondsLeft这样写，会每秒执行一次。
	if (CountdownInt != SecondsLeft)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, FString::Printf(TEXT("WarmingTime: %f---ServerTime: %f----LevelStartingTime: %f---TimeLeft: %f"), WarmingTime, GetServerTime(), LevelStartingTime, TimeLeft));
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}
void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText;

	if (bHUDValid)
	{
		const float Percent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(Health/MaxHealth);

		//FMath::CeiToInt是一个用于将浮点数向上取整并返回整数值的函数，例如如果Health是5.3，则返回6
		FString HealthPercent = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthPercent));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}
//#pragma optimize("", on)
void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->Score;
	if (bHUDValid)
	{
		FString StrScore = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->Score->SetText(FText::FromString(StrScore));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsNum = FString::FromInt(Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsNum));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoNum = FString::FromInt(Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoNum));
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoNum = FString::FromInt(CarriedAmmo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoNum));
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		//防止模式转换时，倒计时出现负数
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString("Zero"));
			return;
		}

		//FloorToInt将浮点数向下取整为整数；
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		//防止模式转换时，倒计时出现负数
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString("Zero"));
			return;
		}

		//FloorToInt将浮点数向下取整为整数；
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

//TimeOfClientRequest是客户端调用ServerRequestServerTime时，客户端上的时间
void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{

	/*
	* 客户端调用，服务器执行。如果服务器调用了被Server标记的函数时，此函数将弃用。
	*/
	//服务器收到客户端请求时，服务器上的时间
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

	//立即将客户端请求时的时间和服务器收到请求时的时间返回给客户端。（ClientRequestServerTime：服务器调用，客户端执行）
	ClientRequestServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientRequestServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequset)
{
	//计算往返的时间
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	//在客户端上计算服务器的时间
	float CurrentServerTime = TimeServerReceivedClientRequset - (0.5f * RoundTripTime);
	//客户端和服务器的时间差（服务器先开始，所以服务器时间大于客户端时间）
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRuningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRuningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRuningTime = 0.f;
	}
}
void ABlasterPlayerController::PollInit()
{
	//初始时显示HUD上的参数的默认值。
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}

}
void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	//BeginPlay执行
	//这个函数和ClientJoinMidgame主要是用来给客户端和服务器获取Game Mode中的WarmingTime和Matchtime。
	 BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;

	if (BlasterGameMode)
	{
		WarmingTime = BlasterGameMode->WarmupTime;
		MatchTime = BlasterGameMode->MatchTime;
		CooldownTime = BlasterGameMode->CooldownTime;
		/*
		* 在服务器中执行时，BlasterGameMode->LevelStartingTime有可能获取失败，原因如下：
		* GameMode中的LevelStartingTime是在BeginPlay中赋值的，而这里的逻辑也是BeginPlay时执行，
		* 当BlasterPlayerControler的BeginPlay先执行时，Game Mode中的LevelStartingTime还是0.
		*/
		LevelStartingTime = BlasterGameMode->LevelStartingTime;

		MatchState = BlasterGameMode->GetMatchState();

		//服务器调用，客户端执行，不让服务器执行。
		ClientJoinMidgame(MatchState, WarmingTime, MatchTime, CooldownTime, LevelStartingTime);

		if (BlasterHUD && MatchState == MatchState::WaitingToStart)
		{
			BlasterHUD->AddAnnouncement();
			//UE_LOG(LogTemp, Display, TEXT("ServerCheckMatchState_Implementation"));
			
		}
	}
}
void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	//视频中并没有这一行
	/*
	* 参考官方文档对PRC的理解https://dev.epicgames.com/documentation/zh-cn/unreal-engine/rpcs-in-unreal-engine?application_version=5.1
	* 当服务器调用Client标记的函数时，如果此Actor所有权属于服务器，那么此函数将在服务器中执行
	* 在这里具体表现为，当在服务器中调用了此函数，那么此函数将会在服务器中运行，那么BlasterHUD->AddAnnouncement();就会执行两次。导致画面上Widget重叠。
	*/
	//imrcao add
	if (HasAuthority()) return;

	WarmingTime = Warmup;
	MatchTime = Match;
	LevelStartingTime = StartingTime;
	CooldownTime = Cooldown;
	OnMatchStateSet(StateOfMatch);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
		//UE_LOG(LogTemp, Display, TEXT("ClientJoinMidgame_Implementation"));
	}
}
void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStart();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStart();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}
void ABlasterPlayerController::HandleMatchHasStart()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();

		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
#pragma optimize("", off)
void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		//imrcao add
		//BlasterHUD->AddAnnouncement();

		bool bHUDVliad = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->AnnouncementText &&
			BlasterHUD->Announcement->InfoText;

		if (bHUDVliad)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString("New Match Start In:"));

			FString InfoTextString;
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;

				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("The winner is : %s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win: \n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
			}
			BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
		}
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
	{
		BlasterCharacter->bDisableGameplay = true;
		//当玩家按下开火键时，进入了CooldownState，防止继续开火。
		BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}
#pragma optimize("", on)

/*


*/