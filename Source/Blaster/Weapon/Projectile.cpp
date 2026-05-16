#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"
#include "../Character/BlasterCharacter.h"
#include "../Blaster.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
	SetRootComponent(CollisionBox);

	/*意思：将 CollisionBox 的碰撞通道设置为 ECC_WorldDynamic。
	  作用：标识这个碰撞组件为动态世界对象，这意味着它可以移动并且会参与物理模拟和碰撞检测*/
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

	/*意思：启用 CollisionBox 的碰撞，使其既能进行查询（如射线检测）又能进行物理模拟。
	  作用：确保 CollisionBox 能够参与所有类型的碰撞检测，包括物理模拟和查询操作。*/
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	/*意思：将 CollisionBox 对所有碰撞通道的响应设置为忽略。
	  作用：初始情况下， CollisionBox 不会与任何通道发生碰撞或重叠。这是一个通用设置，之后可以为特定的通道单独设置响应。*/
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	/*意思：将 CollisionBox 对 ECC_Visibility 通道的响应设置为阻挡。
	  作用：使 CollisionBox 能够阻挡可见性检测，如射线投射。这通常用于确保对象在视线检测中被考虑到。*/
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	/*意思：将 CollisionBox 对 ECC_WorldStatic 通道的响应设置为阻挡。
	  作用：使 CollisionBox 能够与静态世界对象发生碰撞，例如地形和建筑物。这确保了子弹在遇到静态对象时会触发物理碰撞。*/
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

	/*
	* 为了获得更准确的射击（比如爆头等），不使用胶囊体来进行碰撞检测，而是使用SkeletalMesh中的物理资产碰撞。
	* 这里创建了一个自定义的碰撞通道，使用SkeletalMesh来进行碰撞检测
	*/
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	//生成子弹光标，让子弹看得见。
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//Character中的Health变量会复制，并使用了变量复制通知OnRep_Health，变量复制比RPC更高效，
	//所以将NetMulticastHit函数中的内容放到OnRep_Health函数中执行。
	/*ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->NetMulticastHit();
	}*/

	Destroy();
}
void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


