// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerController.generated.h"

class UInventoryComponent;

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ASurvivalPlayerController();

	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowIngameUI();

	UFUNCTION(Client, Reliable)
	void ClientShowNotification(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowNotificationMessage(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowDeathScreen(class ASurvivalCharacter* Killer);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowLootMenu(const UInventoryComponent* LootSource);

	UFUNCTION(BlueprintImplementableEvent)
	void HideLootMenu();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

	UFUNCTION(BlueprintCallable, Category = "Repawn")
	void Respawn();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRespawn();

	// Applies recoil to camera
	// @param RecoilAmount		the amount of recoil by X (yaw) and Y (pitch)
	// @param RecoilSpeed		the speed to bump the camera up per second from the recoil
	// @param RecoilResetSpee	the speed the camera will return to the center per second after finished recoil
	// @param Shake				optional camera shake to play with recoil 
	void ApplyRecoil(const FVector2D RecoildAmount, float RecoilSpeed, float RecoilResetSpeed, TSubclassOf<UCameraShake> FireCameraShake);

	UFUNCTION(Client, Unreliable)
	void ClientShotHitConfirmed();

protected:

	void Turn(float Rate);
	void LookUp(float Rate);
	void StartReload();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilBumpAmount;

	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilResetAmount;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilResetSpeed;

	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	float LastRecoilTime;


};
