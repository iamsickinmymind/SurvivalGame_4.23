// All rights reserved Dominik Pavlicek

#include "WeaponActor.h"

#include "SurvivalGame.h"
#include "Character//SurvivalCharacter.h"
#include "Character/SurvivalPlayerController.h"
#include "Items/AmmoItem.h"
#include "Items/EquippableItem.h"

#include "Components/AudioComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Curves/CurveVector.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

#include "Net/UnrealNetwork.h"

AWeaponActor::AWeaponActor()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->bReceivesDecals = false;
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = WeaponMesh;

	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::EWS_Idle;
	AttachSocket1P = FName("GripPoint");
	AttachSocket3P = FName("GripPoint");
	MuzzleAttachPoint = FName("Muzzle");

	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	ADSTime = 0.5f;
	RecoilResetSpeed = 5.f;
	RecoilSpeed = 10.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}
void AWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponActor, PawnOwner);

	DOREPLIFETIME_CONDITION(AWeaponActor, CurrentAmmoInClip, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponActor, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeaponActor, bPendingReload, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeaponActor, Item, COND_InitialOnly);
}

void AWeaponActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	DetachMeshFromPawn();
}

void AWeaponActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		PawnOwner = Cast<ASurvivalCharacter>(GetOwner());
	}
}

void AWeaponActor::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

void AWeaponActor::UseClipAmmo()
{
	if (HasAuthority())
	{
		--CurrentAmmoInClip;
	}
}

void AWeaponActor::ConsumeAmmo(const int32 Amount)
{
	if (HasAuthority() && PawnOwner)
	{
		if (UInventoryComponent* Inventory = PawnOwner->GetPlayerInventory())
		{
			if (UItem* AmmoItem = Inventory->FindItemByClass(WeaponConfig.AmmoClass))
			{
				Inventory->ConsumeItem(AmmoItem, Amount);
			}
		}
	}
}

void AWeaponActor::ReturnAmmoToInventory()
{
	//When the weapon is unequipped, try return the players ammo to their inventory
	if (HasAuthority())
	{
		if (PawnOwner && CurrentAmmoInClip > 0)
		{
			if (UInventoryComponent* Inventory = PawnOwner->GetPlayerInventory())
			{
				Inventory->TryAddItemFromClass(WeaponConfig.AmmoClass, CurrentAmmoInClip);
			}
		}
	}
}

void AWeaponActor::OnEquip()
{
	AttachMeshToPawn();

	bPendingEquip = true;
	DetermineWeaponState();

	OnEquipFinished();


	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

void AWeaponActor::OnEquipFinished()
{
	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState();

	if (PawnOwner)
	{
		// try to reload empty clip
		if (PawnOwner->IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}
}

void AWeaponActor::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}

	ReturnAmmoToInventory();
	DetermineWeaponState();
}

bool AWeaponActor::IsEquipped() const
{
	return bIsEquipped;
}

bool AWeaponActor::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}

void AWeaponActor::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AWeaponActor::StopFire()
{
	if ((Role < ROLE_Authority) && PawnOwner && PawnOwner->IsLocallyControlled())
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

void AWeaponActor::StartReload(bool bFromReplication /*= false*/)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = .5f;
		}

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AWeaponActor::StopReload, AnimDuration, false);
		if (HasAuthority())
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AWeaponActor::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}

		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}

	if (!CanReload()) {

		if (ASurvivalPlayerController* PlayerCon = Cast<ASurvivalPlayerController>(Cast<ASurvivalCharacter>(GetOwner())->GetController())) {

			if (CurrentAmmoInClip >= WeaponConfig.AmmoPerClip) {

				PlayerCon->ShowNotificationMessage(FText::FromString("Clip full. No need to reload."));
			}
			if (GetCurrentAmmo() == 0) {

				PlayerCon->ShowNotificationMessage(FText::FromString("Not enough ammo to reload."));
			}
		}
	}
}

void AWeaponActor::StopReload()
{
	if (CurrentState == EWeaponState::EWS_Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

void AWeaponActor::ReloadWeapon()
{
	const int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, GetCurrentAmmo() /*- CurrentAmmoInClip*/);

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
		ConsumeAmmo(ClipDelta);
	}
}

bool AWeaponActor::CanFire() const
{
	bool bCanFire = PawnOwner != nullptr;
	bool bStateOKToFire = ((CurrentState == EWeaponState::EWS_Idle) || (CurrentState == EWeaponState::EWS_Firing));
	return ((bCanFire == true) && (bStateOKToFire == true) && (bPendingReload == false));
}

bool AWeaponActor::CanReload() const
{
	bool bCanReload = PawnOwner != nullptr;
	bool bGotAmmo = (CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (GetCurrentAmmo() > 0); /*- CurrentAmmoInClip > 0);*/
	bool bStateOKToReload = ((CurrentState == EWeaponState::EWS_Idle) || (CurrentState == EWeaponState::EWS_Firing));
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true));
}

EWeaponState AWeaponActor::GetCurrentState() const
{
	return CurrentState;
}

int32 AWeaponActor::GetCurrentAmmo() const
{
	if (PawnOwner)
	{
		if (UInventoryComponent* Inventory = PawnOwner->GetPlayerInventory())
		{
			if (UItem* Ammo = Inventory->FindItemByClass(WeaponConfig.AmmoClass))
			{
				return Ammo->GetQuantity();
			}
		}
	}

	return 0;
}

int32 AWeaponActor::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 AWeaponActor::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

USkeletalMeshComponent* AWeaponActor::GetWeaponMesh() const
{
	return WeaponMesh;
}

class ASurvivalCharacter* AWeaponActor::GetPawnOwner() const
{
	return PawnOwner;
}

void AWeaponActor::SetPawnOwner(ASurvivalCharacter* SurvivalCharacter)
{
	if (PawnOwner != SurvivalCharacter)
	{
		Instigator = SurvivalCharacter;
		PawnOwner = SurvivalCharacter;
		// net owner for RPC calls
		SetOwner(SurvivalCharacter);
	}
}

float AWeaponActor::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float AWeaponActor::GetEquipDuration() const
{
	return EquipDuration;
}

void AWeaponActor::ClientStartReload_Implementation()
{
	StartReload();
}

void AWeaponActor::ServerStartFire_Implementation()
{
	StartFire();
}

bool AWeaponActor::ServerStartFire_Validate()
{
	return true;
}

void AWeaponActor::ServerStopFire_Implementation()
{
	StopFire();
}

bool AWeaponActor::ServerStopFire_Validate()
{
	return true;
}

void AWeaponActor::ServerStartReload_Implementation()
{
	StartReload();
}

bool AWeaponActor::ServerStartReload_Validate()
{
	return true;
}

void AWeaponActor::ServerStopReload_Implementation()
{
	StopReload();
}

bool AWeaponActor::ServerStopReload_Validate()
{
	return true;
}

void AWeaponActor::OnRep_PawnOwner()
{

}

void AWeaponActor::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AWeaponActor::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload();
	}
	else
	{
		StopReload();
	}
}

void AWeaponActor::SimulateWeaponFire()
{
	if (HasAuthority() && CurrentState != EWeaponState::EWS_Firing)
	{
		return;
	}

	if (MuzzleFX)
	{
		if (!bLoopedMuzzleFX || MuzzlePSC == nullptr)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if ((PawnOwner != nullptr) && (PawnOwner->IsLocallyControlled() == true))
			{
				AController* PlayerCon = PawnOwner->GetController();
				if (PlayerCon != nullptr)
				{
					WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = true;
				}
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
			}
		}
	}


	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		FWeaponAnim AnimToPlay = FireAnim; //PawnOwner->IsAiming() || PawnOwner->IsLocallyControlled() ? FireAimingAnim : FireAnim;
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	if (bLoopedFireSound)
	{
		if (FireAC == nullptr)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	ASurvivalPlayerController* PC = (PawnOwner != nullptr) ? Cast<ASurvivalPlayerController>(PawnOwner->Controller) : nullptr;
	if (PC != nullptr && PC->IsLocalController())
	{
		if (FireCameraShake != nullptr)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
		if (FireForceFeedback != nullptr)
		{
			FForceFeedbackParameters FFParams;
			FFParams.Tag = "Weapon";
			PC->ClientPlayForceFeedback(FireForceFeedback, FFParams);
		}
	}
}

void AWeaponActor::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX)
	{
		if (MuzzlePSC != nullptr)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = nullptr;
		}
		if (MuzzlePSCSecondary != nullptr)
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = nullptr;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAimingAnim);
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = nullptr;

		PlayWeaponSound(FireFinishSound);
	}
}

void AWeaponActor::HandleHit(const FHitResult& Hit, class ASurvivalCharacter* HitPlayer /*= nullptr*/)
{
	if (Hit.GetActor())
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit actor %s"), *Hit.GetActor()->GetName());
	}

	ServerHandleHit(Hit, HitPlayer);

	if (HitPlayer && PawnOwner)
	{
		if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(PawnOwner->GetController()))
		{
			PC->OnHitPlayer();
		}
	}
}

void AWeaponActor::ServerHandleHit_Implementation(const FHitResult& Hit, class ASurvivalCharacter* HitPlayer /*= nullptr*/)
{
	if (PawnOwner)
	{
		float DamageMultiplier = 1.f;

		/**Certain bones like head might give extra damage if hit. Apply those.*/
		for (auto& BoneDamageModifier : HitScanConfig.BoneDamageModifiers)
		{
			if (Hit.BoneName == BoneDamageModifier.Key)
			{
				DamageMultiplier = BoneDamageModifier.Value;
				break;
			}
		}

		if (HitPlayer)
		{
			UGameplayStatics::ApplyPointDamage(HitPlayer, HitScanConfig.Damage * DamageMultiplier, (Hit.TraceStart - Hit.TraceEnd).GetSafeNormal(), Hit, PawnOwner->GetController(), this, HitScanConfig.DamageType);
		}
	}
}

bool AWeaponActor::ServerHandleHit_Validate(const FHitResult& Hit, class ASurvivalCharacter* HitPlayer /*= nullptr*/)
{
	return true;
}

void AWeaponActor::FireShot()
{
	if (PawnOwner)
	{
		if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(PawnOwner->GetController()))
		{
			if (RecoilCurve)
			{
				const FVector2D RecoilAmount(RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).X, RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).Y);
				//PC->ApplyRecoil(RecoilAmount, RecoilSpeed, RecoilResetSpeed, FireCameraShake);
			}

			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);

			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(PawnOwner);

			FVector FireDir = CamRot.Vector();// PawnOwner->IsAiming() ? CamRot.Vector() : FMath::VRandCone(CamRot.Vector(), FMath::DegreesToRadians(PawnOwner->IsAiming() ? 0.f : 5.f));
			FVector TraceStart = CamLoc;
			FVector TraceEnd = (FireDir * HitScanConfig.Distance) + CamLoc;

			if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, COLLISION_WEAPON, QueryParams))
			{
				ASurvivalCharacter* HitChar = Cast<ASurvivalCharacter>(Hit.GetActor());

				HandleHit(Hit, HitChar);

				FColor PointColor = FColor::Red;
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 5.f, PointColor, false, 30.f);
			}

		}
	}

}

void AWeaponActor::HandleReFiring()
{
	UWorld* MyWorld = GetWorld();

	float SlackTimeThisFrame = FMath::Max(0.0f, (MyWorld->TimeSeconds - LastFireTime) - WeaponConfig.TimeBetweenShots);

	if (bAllowAutomaticWeaponCatchup)
	{
		TimerIntervalAdjustment -= SlackTimeThisFrame;
	}

	HandleFiring();
}

void AWeaponActor::HandleFiring()
{

	if ((CurrentAmmoInClip > 0) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			FireShot();
			UseClipAmmo();

			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			ASurvivalPlayerController* MyPC = Cast<ASurvivalPlayerController>(PawnOwner->Controller);
		}

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::EWS_Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeaponActor::HandleReFiring, FMath::Max<float>(WeaponConfig.TimeBetweenShots + TimerIntervalAdjustment, SMALL_NUMBER), false);
			TimerIntervalAdjustment = 0.f;
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

void AWeaponActor::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeaponActor::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void AWeaponActor::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;

	// reset firing interval adjustment
	TimerIntervalAdjustment = 0.0f;
}

void AWeaponActor::SetWeaponState(EWeaponState NewState)
{
	const EWeaponState PrevState = CurrentState;

	if (PrevState == EWeaponState::EWS_Firing && NewState != EWeaponState::EWS_Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::EWS_Firing && NewState == EWeaponState::EWS_Firing)
	{
		OnBurstStarted();
	}
}

void AWeaponActor::DetermineWeaponState()
{
	EWeaponState NewState = EWeaponState::EWS_Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::EWS_Reloading;
			}
		}
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::EWS_Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::EWS_Equipped;
	}

	SetWeaponState(NewState);
}

void AWeaponActor::AttachMeshToPawn() {
	if (PawnOwner)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		if (USkeletalMeshComponent* PawnMesh = PawnOwner->GetMesh())
		{
			const FName AttachSocket = PawnOwner->IsLocallyControlled() ? AttachSocket1P : AttachSocket3P;
			AttachToComponent(PawnOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
		}
	}
}

void AWeaponActor::DetachMeshFromPawn() {

	// Currently not in use, because there is no switching weapons.
}

UAudioComponent* AWeaponActor::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = nullptr;
	if (Sound && PawnOwner)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, PawnOwner->GetRootComponent());
	}

	return AC;
}

float AWeaponActor::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (PawnOwner)
	{
		UAnimMontage* UseAnim = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			Duration = PawnOwner->PlayAnimMontage(UseAnim);
		}
	}

	return Duration;
}

void AWeaponActor::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (PawnOwner)
	{
		UAnimMontage* UseAnim = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			PawnOwner->StopAnimMontage(UseAnim);
		}
	}
}

FVector AWeaponActor::GetCameraAim() const
{
	ASurvivalPlayerController* const PlayerController = Instigator ? Cast<ASurvivalPlayerController>(Instigator->Controller) : nullptr;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}

FHitResult AWeaponActor::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, Instigator);
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
}

void AWeaponActor::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseClipAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}

bool AWeaponActor::ServerHandleFiring_Validate()
{
	return true;
}