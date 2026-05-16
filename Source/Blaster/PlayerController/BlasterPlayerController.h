// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	/*
	* 它在玩家控制器成功接收并初始化玩家时被调用。这个函数通常在网络游戏或多人游戏中使用，用于执行一些玩家初始化和设置的操作。
	*/
	virtual void ReceivedPlayer() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);

	virtual float GetServerTime();

	void OnMatchStateSet(FName State);
	void HandleMatchHasStart();
	void HandleCooldown();
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	void SetHUDTime();

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientRequestServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequset);

	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRuningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

	/*主要用来更新HUD上的各种显示参数，例如生命条，得分等。
	* Tick执行
	* 由于切换游戏状态时UCharacterOverlay有可能无效，所以导致初始HUD界面显示值不准确。故Tick执行这个函数。
	*/
	void PollInit();

	//客户端加入服务器时调用，主要用来从GameMode中获取MatchTime、WarmingTime、LevelStartingTime、MatchState等。
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;
	
	//从BlasterGameMode中获取
	float MatchTime = 0.f;
	float WarmingTime = 0.f;
	float CooldownTime = 0.f;
	float LevelStartingTime = 0.f;

	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	bool bInitializeCharacterOverlay = false;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	float HUDDefeats;
};
