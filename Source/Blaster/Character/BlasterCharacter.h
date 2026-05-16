// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../BlasterTypes/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "../BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	void PlayFireMontage(bool bAiming);
	void PlayElimMontage();
	void PlayReloadMontage();

	/*UFUNCTION(NetMulticast, Unreliable)
	void NetMulticastHit();*/

	virtual void OnRep_ReplicatedMovement() override;

	//仅服务器中执行
	void Elim();

	//玩家淘汰时执行
	UFUNCTION(NetMultiCast, Reliable)
	void MulticastElim();

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void ReloadButtonPressed();
	virtual void Jump()override;
	void FireButtonPressed();
	void FireButtonReleased();
	float CalculateSpeed();
	void PlayHitReactMontage();

	UFUNCTION()
	void OnReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
	void UpdateHUDHealth();


	//计算偏移值，只在“装备武器”状态下执行计算
	void AimOffset(float DeltaTime);

	void CalculateAO_Pitch();

	//解决处于模拟代理的角色转向时不流畅的问题：原因是在动画蓝图中，并不会每帧更新模拟代理角色的动画
	void SimProxiesTurn();

	UPROPERTY(EditAnywhere, Category = "Player State")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player State")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	//轮询任何相关类，来初始化HUD，比如PlayerState在游戏开始前几帧是无效的，无法使用PlayerState中默认值来初始化HUD。
	void PollInit();

	void RotatorInPlace(float DeltaTime);

private:

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverHeadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	/*
	Remote Procedure Calls （RPC）远程过程调用，客户端申请调用，在服务器中执行
	Reliable 当我们制作 RBC 时，必须指定 RBC 是可靠的还是不可靠的。现在的区别是：
	可靠的 RBC 可以保证执行，而不可靠的 RBC 在从客户端向服务器发送信息的过程中可能会被丢弃。
	*/
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float AO_Pitch;
	float InterpAO_Yaw;
	FRotator StartingAimRotation;
	
	//旋转状态
	ETurningInPlace TurningInPlace;

	void TurnInPlace(float DeltalTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;


	bool bRotateRootBone;

	UPROPERTY(EditAnywhere)
	float TurnThreshold = 5.5f;

	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;

	float TimeSinceLastMovementReplication;

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	//是否被淘汰
	bool bIsElimed = false;

	FTimerHandle ElimHandle;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	/***--- 角色溶解 ---***/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialInstance;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	/***--- 角色溶解 ---***/
	void StartDissolve();

	//玩家淘汰时在玩家头上生成一个机器人特效并播放相应音效
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	//UPROPERTY(VisiableAnywhere)
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotEffectComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;
public:

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FVector GetHitTarget() const;
	AWeapon* GetEquippedWeapon();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimed() const { return bIsElimed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	ECombatState GetCombatState();
	FORCEINLINE UCombatComponent* GetCombatComponent() { return Combat; }
	FORCEINLINE bool GetDisableGameplay() { return bDisableGameplay; }
};
