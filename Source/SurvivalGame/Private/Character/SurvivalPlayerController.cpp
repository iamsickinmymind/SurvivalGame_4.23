// All rights reserved Dominik Pavlicek

#include "SurvivalPlayerController.h"
#include "Character/SurvivalCharacter.h"
#include "Net/UnrealNetwork.h"

ASurvivalPlayerController::ASurvivalPlayerController() {

	RecoilBumpAmount = FVector2D(0.f, 0.f);
	RecoilResetAmount = FVector2D(0.f, 0.f);
	CurrentRecoilSpeed = 0.f;
	CurrentRecoilResetSpeed = 0.f;
	LastRecoilTime = 0.f;
}

void ASurvivalPlayerController::SetupInputComponent() {

	Super::SetupInputComponent();

	InputComponent->BindAxis("Turn", this, &ASurvivalPlayerController::Turn);
	InputComponent->BindAxis("LookUp", this, &ASurvivalPlayerController::LookUp);

	InputComponent->BindAction("Reload", IE_Pressed, this, &ASurvivalPlayerController::StartReload);
}

void ASurvivalPlayerController::Turn(float Rate)
{
	//If the player has moved their camera to compensate for recoil we need this to cancel out the recoil reset effect
	if (!FMath::IsNearlyZero(RecoilResetAmount.X, 0.01f))
	{
		if (RecoilResetAmount.X > 0.f && Rate > 0.f)
		{
			RecoilResetAmount.X = FMath::Max(0.f, RecoilResetAmount.X - Rate);
		}
		else if (RecoilResetAmount.X < 0.f && Rate < 0.f)
		{
			RecoilResetAmount.X = FMath::Min(0.f, RecoilResetAmount.X - Rate);
		}
	}

	//Apply the recoil over several frames
	if (!FMath::IsNearlyZero(RecoilBumpAmount.X, 0.1f))
	{
		FVector2D LastCurrentRecoil = RecoilBumpAmount;
		RecoilBumpAmount.X = FMath::FInterpTo(RecoilBumpAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		AddYawInput(LastCurrentRecoil.X - RecoilBumpAmount.X);
	}

	//Slowly reset back to center after recoil is processed
	FVector2D LastRecoilResetAmount = RecoilResetAmount;
	RecoilResetAmount.X = FMath::FInterpTo(RecoilResetAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);
	AddYawInput(LastRecoilResetAmount.X - RecoilResetAmount.X);

	AddYawInput(Rate);

}

void ASurvivalPlayerController::LookUp(float Rate) {

	if (!FMath::IsNearlyZero(RecoilResetAmount.Y, 0.01f)) {

		if (RecoilResetAmount.Y > 0.f && Rate > 0.f) {

			RecoilResetAmount.Y = FMath::Max(0.f, RecoilResetAmount.Y - Rate);
		}
		else if (RecoilResetAmount.Y < 0.f && Rate > 0.f) {

			RecoilResetAmount.Y = FMath::Min(0.f, RecoilResetAmount.Y - Rate);
		}
	}

	// Apply recoild over frames
	if (!FMath::IsNearlyZero(RecoilBumpAmount.Y, 0.01f)) {

		FVector2D LastCurrentRecoil = RecoilBumpAmount;
		RecoilBumpAmount.Y = FMath::FInterpTo(RecoilBumpAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		AddPitchInput(LastCurrentRecoil.Y - RecoilBumpAmount.Y);
	}

	FVector2D LastRecoildResetAmount = RecoilResetAmount;
	RecoilResetAmount.Y = FMath::FInterpTo(RecoilResetAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);
	AddPitchInput(LastRecoildResetAmount.Y - RecoilResetAmount.Y);

	AddPitchInput(Rate);
}

void ASurvivalPlayerController::StartReload() {

	if (ASurvivalCharacter* PlayerPawn = Cast<ASurvivalCharacter>(GetPawn())) {

		if (PlayerPawn->IsAlive()) {

			PlayerPawn->StartReload();
		}
		else {

			Respawn();
		}
	}
	else {

		int32 RandomA = 1;
		Respawn();
	}
}

void ASurvivalPlayerController::Respawn() {

	if (ASurvivalCharacter* PlayerPawn = Cast<ASurvivalCharacter>(GetPawn())) {

		if (PlayerPawn->IsAlive()) {

			return;
		}
	}

	UnPossess();

	ChangeState(NAME_Inactive);

	if (!HasAuthority()) {

		ServerRespawn();
	}

	if (HasAuthority()) {

		ServerRestartPlayer();
	}
}

void ASurvivalPlayerController::ServerRespawn_Implementation() {

	Respawn();
}

bool ASurvivalPlayerController::ServerRespawn_Validate() {

	return true;
}

void ASurvivalPlayerController::ApplyRecoil(const FVector2D RecoildAmount, float RecoilSpeed, float RecoilResetSpeed, TSubclassOf<UCameraShake> FireCameraShake) {
	
	if (IsLocalPlayerController()) {

		if (PlayerCameraManager != nullptr && FireCameraShake) {

			PlayerCameraManager->PlayCameraShake(FireCameraShake);
		}

		RecoilBumpAmount += RecoildAmount;
		RecoilResetAmount -= RecoildAmount;

		CurrentRecoilSpeed = RecoilSpeed;
		CurrentRecoilResetSpeed = RecoilResetSpeed;

		LastRecoilTime = GetWorld()->GetTimeSeconds();
	}
}

void ASurvivalPlayerController::ClientShotHitConfirmed_Implementation() {

	//...
}

void ASurvivalPlayerController::ClientShowNotification_Implementation(const FText& Message) {

	ShowNotificationMessage(Message);
}
