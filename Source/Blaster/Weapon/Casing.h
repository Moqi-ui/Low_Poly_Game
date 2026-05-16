/******************************************************
* @copyright	2024, www.imrcao.com 
* 
* @author		Imrcao
* 
* @data			2024年06月03号
* 
* @brief		弹壳类，开火时弹出弹壳
* 
* @see			OnHit()			当弹壳落地时执行相应的逻辑，比如比如播放落地音效
* 
******************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

protected:
	virtual void BeginPlay() override;

	//添加UFUNCTION()非常重要，那些绑定到重叠和碰撞事件的回调函数总是需要UFUNCITON()。
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* CasingMovement;

	//初始冲击力，弹壳弹出的速度
	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse;

	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound;
};
