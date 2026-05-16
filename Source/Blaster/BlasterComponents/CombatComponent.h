// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../HUD/BlasterHUD.h"
#include "../Weapon/WeaponTypes.h"
#include "../BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();

	//友元类，这样做可以让ABlasterCharacter访问UCombatComponent的私有成员变量
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void FinishedReloading();

	void FireButtonPressed(bool bPressed);

protected:
	virtual void BeginPlay() override;

	void EquipWeapon(class AWeapon* WeaponToEquip);

	void SetAiming(bool bIsAiming);

	//客户端调用，服务器执行，bAiming变量又可以复制，所以会将bAiming这个变量复制到各个客户端。
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void Fire();
	
	//将此函数声明为一个要在客户端上调用、但需要在服务器上执行的 RPC
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	//这是一种叫做多播（Multicast）的特殊类型的 RPC 函数。多播 RPC 可以从服务器调用，然后在服务器和当前连接的所有客户端上执行
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	//绘制十字瞄准准星
	void SetHUDCrosshairs(float DeltaTime);

	void Reload();

	UFUNCTION(Server, Reliable)
	void ServerReload();

	int32 AmountToReload();

	void UpdateAmmoValues();

private:
	//在BlasterCharacter中的PostInitializeComponents函数中赋值。
	UPROPERTY()
	class ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* Controller;
	UPROPERTY()
	class ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	//Crosshairs玩家速度缩放因素
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	float DefaultFOV;

	float CurrentFOV;

	float ZoomedFOV = 30.f;

	float ZoomInterpSpeed = 20.f;

	//当瞄准时计算FOV插值，调整视野
	void InterpFOV(float DeltaTime);

	/**----------------防止过快射击------------------*/
	bool bCanFire = true;

	FTimerHandle FireTimer;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();
	/**----------------防止过快射击------------------*/

	//玩家身上携带的弹药（和武器无关）
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	//TMap类型变量无法复制
	/*
	* 携带的弹药数量也可以说是和PlayerState有关，但是它和战斗组件关联性更大
	* 并且Character的复制速度比PlayerState更快。所以将携带的弹药变量放在战斗组件，而没有放在PlayerState中。
	*/
	TMap<EWeaponType, int32> CarriedAmmoMap;

	//游戏开始时，玩家身上初始携带的弹药数量
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void HandleReload();
};
