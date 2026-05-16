/******************************************************
* @copyright	2024, www.imrcao.com
*
* @author		Imrcao
*
* @data			2024年06月4号
*
* @brief		HUD，处理屏幕绘制
*
* @see			SetHUDPackage()	设置准星的UTexture2D
*				DrawHUD()		屏幕绘制
*
*
******************************************************/
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;

	/*Range is [0-1]*/
	float CrosshairsSpread;
	FLinearColor CrosshairsColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;
	//添加生命条UI
	void AddCharacterOverlay();

	//添加公告栏，热身阶段显示相关信息
	void AddAnnouncement();
public:
	UPROPERTY(EditAnywhere, Category = "Player States")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<UUserWidget>  AnnouncementClass;

	UPROPERTY()
	class UAnnouncement* Announcement;

protected:
	virtual void BeginPlay() override;

private:
	FHUDPackage HUDPackage;
	void DrawCrosshairs(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	float CrosshairsSpreadMax = 16.f;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
