// Fill out your copyright notice in the Description page of Project Settings.
#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator();

	AController* FiringController = FiringPawn->GetController();
	if (FiringController)
	{
		UGameplayStatics::ApplyRadialDamageWithFalloff(
			this,//伤害内圈的基础伤害值
			Damage,
			10.f,
			GetActorLocation(),
			200.f,
			500.f,
			1.f,
			UDamageType::StaticClass(),
			TArray<AActor*>(),
			this,
			FiringController
			);
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
