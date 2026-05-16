/******************************************************
* @copyright	2024, www.imrcao.com
*
* @author		Imrcao
*
* @data			2024年05月08号
*
* @brief		发射子弹类的武器的基类，比如自动步枪，狙击步枪等
*
* @see			Fire()		重写父类虚函数，处理开火的逻辑
*				
*
******************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;
	
private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;
};
