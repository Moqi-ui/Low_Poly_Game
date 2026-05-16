
#include "CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

#define TRACE_LENGTH 40000.f

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;

}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);

	//仅通知本地客户端。
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);

}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	//bFireButtonPressed这个变量由本地客户端管理，
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		//无论是在客户端调用，还是服务器中调用，都只会在服务器中执行
		ServerFire(HitTarget);
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 0.85f;
		}
		StartFireTimer();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	NetMulticastFire(TraceHitTarget);
}

void UCombatComponent::NetMulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	//使用NetMulticast标记的函数，客户端上调用时，只会在客户端上执行，服务器调用时，会在服务器和与其连接的所有客户端上执行。
	//Node:每次调用RPC函数时，都会通过网络发送数据，所以尽管RPC函数很好用，但也要注意使用频率，减少带宽，在多人游戏中，通过网络发送的数据越少越好。
	if (EquippedWeapon == nullptr) return;
	//如果CombatState还没有从服务器复制到客户端时，不能开火，所以要判断CombatState == ECombatState::ECS_Unoccupied
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon && Character)
	{
		Character->GetWorldTimerManager().SetTimer(
			FireTimer,
			this,
			&UCombatComponent::FireTimerFinished,
			EquippedWeapon->FireDelay
		);
	}
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;

	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	if (EquippedWeapon->IsAmmoEmpty())
	{
		Reload();
	}
}
bool UCombatComponent::CanFire()
{
	if (EquippedWeapon)
	{
		return !EquippedWeapon->IsAmmoEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
	}

	return false;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	//这里加一行是为了调用的客户端更快的响应，经过测试注释掉这一行相应时间没有什么差别。
	bAiming = bIsAiming;

	//这里不做是否为服务器的判断，是因为这个RPC函数可以在服务器中调用。判断不判断都可以。
	ServerSetAiming(bIsAiming);

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	//此函数只会在服务器执行，

	if (Character == nullptr || WeaponToEquip == nullptr) return;

	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = WeaponToEquip;

	//这个需要注意，详情看函数实现。
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		/*AttachActor 函数在 Unreal Engine 中是一个实用工具，主要用于将一个 AActor 附加到一个 USkeletalMeshComponent 的插槽上。
		它通过将 Actor 附加到 SkelComp，使得 Actor 可以随 SkelComp 一起移动和变换。这对于实现角色装备、物品拾取等功能非常有用。*/
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	//在Actor中Owner是一个复制变量，使用了复制通知：OnRep_Onwner();
	EquippedWeapon->SetOwner(Character);
	//装备武器后立即更新HUD上的弹药数量；
	EquippedWeapon->SetHUDAmmo();

	/*****---------begin:装备武器时更新携带的弹药数量-----------*****/
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	/*****---------End:装备武器时更新携带的弹药数量-----------*****/

	if (EquippedWeapon->EquippSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquippSound, Character->GetActorLocation());
	}

	if (EquippedWeapon->IsAmmoEmpty())
	{
		Reload();
	}

	//不明白为什么在本身客户端没有生效，需要再写一个OnRep_EquippedWeapon
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}
void UCombatComponent::Reload()
{
	//只在服务端执行
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

//计算需要装弹的数量
int32 UCombatComponent::AmountToReload()
{
	if (CarriedAmmo <= 0 || EquippedWeapon == nullptr) return 0;

	//计算弹夹可装填的弹药数量
	int RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		//玩家携带的弹药数量
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

		int32 Least = FMath::Min(RoomInMag, AmountCarried);

		//一般来讲武器弹夹容量肯定大于武器上的弹药数量。
		// 为了防止我们在蓝图编辑器填错这两个变量的数值（MagCapacity、GetAmmo），这里用Clamp，保险一点。
		return FMath::Clamp(RoomInMag, 0, Least);
	}


	return 0;
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr|| EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		//从其它状态返回未占用状态时检查是否可以开火。
		if (bFireButtonPressed) { Fire(); }
		break;
	case ECombatState::ECS_Reloading:
		HandleReload();

		break;
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

void UCombatComponent::FinishedReloading()
{
	//换弹蒙太奇中有个事件通知，这个函数在通知事件中执行。
	if (!Character) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
		//换弹结束后判断是否按下了开火按钮。
		if (bFireButtonPressed)
		{
			Fire();
		}
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	int ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	//更新玩家身上的携带弹药数量
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	//更新武器上的弹药数量
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		/*-----------------why-----------------*/
		/*
		* 这一段代码和EuiqpWeapon中的一段代码重复，原因如下：
		* EquipWeapon只会在服务器中执行，其中有两个动作会复制到客户端：1、在执行SetWeaponState时；2、AttachActor时。
		* 当玩家尝试捡起一个被丢弃的武器时（此时武器的状态为：EWeaponState::EWS_Dropped），当武器被丢弃时，武器的物理属性会被打开。
		* 假如在复制过程中，客户端1先执行SetWeaponState，再执行AttachActor，那么此时不会有问题，因为此时武器的物理属性是关闭的。
		* 但是在网络复制过程中，事情并不总是这样，也有可能AttachActor先在客户端中执行，那么此时玩家是无法拾取一个具有物理属性被打开的武器的。AttachActor()会返回false；
		* 为了确保在AttachActor前，武器的状态为EWS_Equipped，所以在客户端中执行以下一段和服务器中一样的代码。
		*/
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		/*----------------------------------*/

		if (EquippedWeapon->EquippSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquippSound, Character->GetActorLocation());
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	//绘制Debug线段，
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairScreenLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairScreenLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		//解决玩家会瞄准自己的问题；解决队友或敌人靠太近时（处于相结和CharacterMesh之间）会误瞄准的问题。
		if (Character)
		{
			float FromStartDistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (FromStartDistanceToCharacter + 100.f);
		}

		FVector End = CrosshairWorldPosition + CrosshairWorldDirection * TRACE_LENGTH;

		//FCollisionQueryParams CollisionParams;
		//CollisionParams.AddIgnoredActor(GetOwner()); // 忽略玩家自身
		//CollisionParams.AddIgnoredActor(Character->GetEquippedWeapon()); //忽略武器

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
			//UE_LOG(LogTemp, Log, TEXT("Implements"));
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
			//UE_LOG(LogTemp, Log, TEXT("NOImplements"));
		}

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		//else
		//{
		//	//Debug：绘制Debug球，调试用。
		//	DrawDebugSphere(
		//		GetWorld(),
		//		TraceHitResult.ImpactPoint,
		//		6.f,
		//		6,
		//		FColor::Red);
		//}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}

			//根据玩家行走的速度缩放准星。将速度值映射到【0-1】
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector CharacterVelocity = Character->GetVelocity();
			CharacterVelocity.Z = 0;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, CharacterVelocity.Size());

			//根据玩家是否在空中来影响准星
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, 30.f);

			HUDPackage.CrosshairsSpread =
				0.6 +
				CrosshairVelocityFactor +
				CrosshairInAirFactor -
				CrosshairAimFactor +
				CrosshairShootingFactor;

			//if(HitResult)


			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;
	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	//设置视野
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	//CarriedAmmo是有条件复制，只复制到拥有角色的客户端，通常就是本地客户端。
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

