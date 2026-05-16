#include "Casing.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	//生成的子弹会和相机视角发生碰撞，导致玩家视角偶尔会发生变化。所以要忽略相机碰撞
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	//打开子弹的模拟物理属性和允许重力。
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);

	//用于启用或禁用刚体组件的碰撞通知。允许Actor与其他物体发生碰撞时接收通知。要使用OnHit事件，必须将其开启。
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	//AddImpulse是一个用来施加一个瞬时力到物体上的一个方法。
	//GetActorForwardVector返回的是一个单位向量（X轴向）。
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);

	//绑定OnHit事件
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (CasingMesh)
	{
		//由于ACasingMesh开启了模拟物理，在弹壳落地的时候会多次弹起，导致多次执行Onhit事件，所以这里移除绑定，让OnHit事件只执行一次，
		CasingMesh->OnComponentHit.RemoveDynamic(this, &ACasing::OnHit);
	}
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}
	//三秒后销毁，使用Lambda表达式。
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			Destroy();
		}, 3.f, false);
}
