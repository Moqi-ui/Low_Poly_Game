// Fill out your copyright notice in the Description page of Project Settings.

/******************************************************
* @copyright	2024, www.imrcao.com
*
* @author		Imrcao
*
* @data			2024年05月03号
*
* @brief		所有武器的基类，处理与武器相关的事件
*
* @see			GetLifetimeReplicatedProps			管理需要复制的变量
*				Fire()		虚函数，处理开火的逻辑
*
******************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Inittal State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")

};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;

public:

	//定义成虚函数，子类重写
	virtual void Fire(const FVector& HitTarget);
	void Dropped();
	
	//设置弹药数量
	void SetHUDAmmo();
	void AddAmmo(int32 AmmoToAdd);
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
		virtual void OnSphereOverlap(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex,
			bool bFromSweep,
			const FHitResult& SweepResult
		);

	UFUNCTION()
		virtual void OnSphereEndlap(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex
		);

private:

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class USkeletalMeshComponent* WeaponMesh;

	//碰撞事件只会发生在服务器中，所以此属性不需要复制
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class UWidgetComponent* PickupWidget;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
		EWeaponState WeaponState;

	UFUNCTION()
		void OnRep_WeaponState();

		//
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
		class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
		TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo);
		int32 Ammo;

		//弹夹(Mag)容量，MaxCapacity一定大于等于Ammo。
	UPROPERTY(EditAnywhere)
		int32 MaxCapacity;

	UFUNCTION()
	void OnRep_Ammo();

	//负责：1、开火时减少弹药的数量；2、更新HUD。
	void SpendRound();

	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
public:
	//十字瞄准星
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;

	//设置视野，瞄准时改变视野
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	//视野缩放速度
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/**----------------自动武器------------------*/
	//开火间隔
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f;
	//是否为自动武器
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;
	/**----------------自动武器------------------*/

	UPROPERTY(EditDefaultsOnly)
	class USoundCue* EquippSound;


public:
	void ShowPickupWidget(bool bShowWidget);
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	bool IsAmmoEmpty();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MaxCapacity; }
};
