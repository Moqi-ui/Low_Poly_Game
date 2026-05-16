/******************************************************
* @copyright	2024, www.imrcao.com
*
* @author		Imrcao
*
* @data			2024年05月14号
*
* @brief		子弹，弹药基类，
*
* @see			Destroyed()		子弹销毁时执行额外的逻辑，比如撞击特效和音效等。
*				OnHit()			碰撞事件，执行Destroy()
*
*
******************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;

	//添加UFUNCTION()非常重要，那些绑定到重叠和碰撞事件的回调函数总是需要UFUNCITON()。
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);


protected:

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

private:

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionBox; 

	// 让子弹移动起来，UE内置的一个组件，和CharacterMovementComponent一样，它自带复制功能。
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	//让子弹看得见，子弹的光标特效。
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;
	
};
