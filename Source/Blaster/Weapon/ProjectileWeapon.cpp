// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	/*
	我们要确保在服务器上生成子弹，所以要使用HasAuthority()进行权威性判断。
	如果我们的武器没有被复制，那就意味着它在所有的机器上都保持权威性，
	但是如果它复制了，它就只会在服务器上保持权威性。所以我们要设置AWeapon类的属性：bReplicates = true;
	*/
	if (!HasAuthority()) return;

	/*
	 在ABlasterCharacter::GetLifetimeReplicatedProps设置了：
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	表示AWeapon的所有者是ABlasterCharacter。
	所以通过Cast<APawn>(GetOwner());可以转换成APawn。
	*/
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		/*
		* HitTarget 指屏幕2D空间转换到3D空间后，通过射线打到的第一个撞击点的位置
		* SocketTransform.GetLocation() 指枪口的位置
		*/
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		//一个向量通过Rotation（）方法就能得到这个向量的方向
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn)
		{
			/*
			FActorSpawnParameters 结构体用于指定在 UWorld::SpawnActor 函数中生成新 Actor 时的各种参数。
			SpawnParams.Owner 和 SpawnParams.Instigator 是两个常用的成员变量，
			用于设置生成 Actor 的所有者和促使其生成的角色（Instigator）。
			*/
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
					);
			}
		}
	}
}
