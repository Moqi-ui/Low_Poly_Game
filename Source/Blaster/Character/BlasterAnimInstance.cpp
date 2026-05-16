// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "../Weapon/Weapon.h"
#include "../BlasterTypes/CombatState.h"
void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());

}
void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if (BlasterCharacter == nullptr) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.0f;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bIsAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bIsElimed = BlasterCharacter->IsElimed();

	//YawOffset和Lean的求值算法不太懂，侧重了解如何在混合空间中使用该数值。
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.0f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.0f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	//if (BlasterCharacter->HasAuthority() && !BlasterCharacter->IsLocallyControlled())
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AimRotation %s: "), *AimRotation.ToString());
	//	UE_LOG(LogTemp, Warning, TEXT("YawOffset %f: "), YawOffset);
	//	UE_LOG(LogTemp, Warning, TEXT("Lean %f: "), Lean);
	//	UE_LOG(LogTemp, Warning, TEXT("MovementRotation %s: "), *MovementRotation.ToString());
	//	//UE_LOG(LogTemp, Warning, TEXT("AimRotation Yaw %f: "), AimRotation.Yaw);
	//	//UE_LOG(LogTemp, Warning, TEXT("MovementRotation Yaw %f: "), MovementRotation.Yaw);
	//}

	//计算Fabric
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		//UE_LOG(LogTemp, Display, TEXT("LeftHandSocket: %s"), *LeftHandSocketTrans.ToString());
		FVector OutPosition;
		FRotator OutRotation;
		//BlasterCharacter->GetMesh()->TransformFromBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
		//UE_LOG(LogTemp, Display, TEXT("OutPosition: %s"), *OutPosition.ToString());
		//UE_LOG(LogTemp, Display, TEXT("LeftHandTransform: %s"), *LeftHandTransform.ToString());

		//修复：HitTarget（十字准星）和枪口对准的方向不一致的问题。通过下面的绘制射线（注释掉的代码）即可明显的看出来。
		if (BlasterCharacter->IsLocallyControlled())
		{
			//只在本地修复这个问题，一来可以避免由于HitTarget在网络中每帧传输导致的高带宽。
			//二来这种细微修复没必要复制到其它客户端，因为这个问题只有本地玩家才能注意到。
			bIsLocallyControlled = true;

			/*
			之所以能够通过EquippedWeapon获得BlasterCharacter中的“Hand_R”Socket，主要是因为角色在拾取武器时EquippedWeapon已经Attach在了角色的SkeletalMeshComponent上了。
			*详情请看CombatComponent.cpp中的EquipWeapon函数中的：HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
			*/
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
			//不懂？？？？？
			FRotator LookAtRotator = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));

			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotator, DeltaTime, 30.f);

			//绘制射线，
			/*FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
			FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);*/
		}
	}
	bUseFABRIK = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	bUseAimOffsets = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !BlasterCharacter->GetDisableGameplay();
	bUseTransformRightHand = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !BlasterCharacter->GetDisableGameplay();
}

