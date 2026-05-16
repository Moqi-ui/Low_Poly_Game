#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"


void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/*
	* 这里使用UGameplayStatics::ApplyDamage时，需要注意AController参数的获取，
	* GetOwner()返回的是一个AActor，为什么可以转换为ACharacter?原因如下：
	* Projecttile是在ProjecttileWeapon.cpp中的Fire函数中生成，
	* 生成时设置了它的所有者为ProjecttileWeapon，
	* 而在收取武器时，设置了武器的所有者为Character。
	* 所以在此处调用GetOwner()实际返回的是Character。
	*/
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}

	//父类中会执行Destroy,所以这里最后执行父类中的逻辑。
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
