// All rights reserved Dominik Pavlicek

#include "SurvivalCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/DamageType.h"
#include "Materials/MaterialInstance.h"
#include "Kismet/GameplayStatics.h"

#include "Net/UnrealNetwork.h"

#include "SurvivalGame.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "World/Pickup.h"
#include "Items/GearItem.h"
#include "Items/WeaponItem.h"
#include "Items/ThrowableItem.h"
#include "Character/SurvivalPlayerController.h"
#include "Weapons/MeleeDamage.h"
#include "Weapons/WeaponActor.h"
#include "Animation/AnimMontage.h"

#define LOCTEXT_NAMESPACE "Player"
#define NAME_ADS_Socket FName("ADSSocket")

ASurvivalCharacter::ASurvivalCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(FName("Inventory Component"));
	InventoryComponent->SetCapacity(20);
	InventoryComponent->SetWeightCapacity(50.f);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(FName("SpringArmComp"));
	SpringArm->SetupAttachment(GetMesh(), FName("CameraSocket"));
	SpringArm->TargetArmLength = 0.f;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(FName("CameraComp"));
	CameraComp->SetupAttachment(SpringArm);
	CameraComp->bUsePawnControlRotation = true;

	/** Must be created manually.
	* The following case created errors that lead to rewriting the code:
		HelmetMesh = PlayerMeshes.Add(EEquippableSlot::EES_Helmet, CreateDefaultSubobject<USkeletalMesh>(FName("HelmetMesh")));
	* In situation all components were created this way, Editor ignored the UPROPERTY macro and variables were unaccessible anymore.
	* Not even deleting .vs, Intermediate, Binary and Saved folder helped.
	*/
	HelmetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("HelmetMeshComp"));
	HelmetMesh->SetupAttachment(GetMesh());
	HelmetMesh->SetMasterPoseComponent(GetMesh());

	TorsoMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("TorsoMeshComp"));
	TorsoMesh->SetupAttachment(GetMesh());
	TorsoMesh->SetMasterPoseComponent(GetMesh(), true);

	LegsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("LegsMeshComp"));
	LegsMesh->SetupAttachment(GetMesh());
	LegsMesh->SetMasterPoseComponent(GetMesh());

	FeetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("FeetMeshComp"));
	FeetMesh->SetupAttachment(GetMesh());
	FeetMesh->SetMasterPoseComponent(GetMesh());

	HandsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("HandsMeshComp"));
	HandsMesh->SetupAttachment(GetMesh());
	HandsMesh->SetMasterPoseComponent(GetMesh());

	VestMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("VestMeshComp"));
	VestMesh->SetupAttachment(GetMesh());
	VestMesh->SetMasterPoseComponent(GetMesh());

	BackpackMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("BackpackComp"));
	BackpackMesh->SetupAttachment(GetMesh());
	BackpackMesh->SetMasterPoseComponent(GetMesh());

	PlayerMeshes.Add(EEquippableSlot::EES_Head, GetMesh());
	PlayerMeshes.Add(EEquippableSlot::EES_Helmet, HelmetMesh);
	PlayerMeshes.Add(EEquippableSlot::EES_Torso, TorsoMesh);
	PlayerMeshes.Add(EEquippableSlot::EES_Legs, LegsMesh);
	PlayerMeshes.Add(EEquippableSlot::EES_Feet, FeetMesh);
	PlayerMeshes.Add(EEquippableSlot::EES_Hands, HandsMesh);
	PlayerMeshes.Add(EEquippableSlot::EES_Vest, VestMesh);
	PlayerMeshes.Add(EEquippableSlot::EES_Backpack, BackpackMesh);

	for (auto& PlayerMesh : PlayerMeshes) {

		if (USkeletalMeshComponent* ThisMesh = Cast<USkeletalMeshComponent>(PlayerMesh.Value)) {

			ThisMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			ThisMesh->SetCollisionObjectType(ECC_Pawn);
			ThisMesh->SetCollisionProfileName("Custom");
			ThisMesh->SetCollisionResponseToAllChannels(ECR_Block);
			ThisMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}
	}

	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->bCastHiddenShadow = true;
	HelmetMesh->SetOwnerNoSee(true);
	HelmetMesh->bCastHiddenShadow = true;

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	DeadBodyInteractionComponent = CreateDefaultSubobject<UInteractionComponent>(FName("PlayerInteraction"));
	DeadBodyInteractionComponent->SetInteractionActionText(LOCTEXT("LootPlayerText", "Loot"));
	DeadBodyInteractionComponent->SetInteractionNameText(LOCTEXT("LootPlayerName", "Player"));
	DeadBodyInteractionComponent->SetupAttachment(GetMesh());
	DeadBodyInteractionComponent->SetActive(false, true);
	DeadBodyInteractionComponent->bAutoActivate = false;

	InteractionCheckFrequency = 0.f;
	InteractionCheckDistance = 1000.f;

	MaxHealth = 100.f;
	Health = MaxHealth;
	DeadBodyLifespan = 120.f;

	LastMeleeAttackTime = 0.f;
	MeleeAttackDistance = 150.f;
	MeleeAttackDamage = 33.f;
	bIsAiming = false;

	AimingFOV = 70.f;
	DefaultFOV = CameraComp->FieldOfView;

	WalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	MaxSprintSpeed = WalkSpeed * 1.3f;
}

void ASurvivalCharacter::BeginPlay()
{
	Super::BeginPlay();

	DeadBodyInteractionComponent->OnInteract.AddDynamic(this, &ASurvivalCharacter::BeginLootingPlayer);

	if (APlayerState* PS = GetPlayerState()) {

		DeadBodyInteractionComponent->SetInteractionNameText(FText::FromString(PS->GetPlayerName()));
	}

	for (auto& PlayerMesh : PlayerMeshes) {

		NakedMeshes.Add(PlayerMesh.Key, PlayerMesh.Value->SkeletalMesh);
	}
}

void ASurvivalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const bool bIsInteractingOnServer = (HasAuthority() && GetIsInteracting());

	// Perform frequented check on Client only and only in InteractionCheckFrequency
	if ((!HasAuthority() || bIsInteractingOnServer || IsLocallyControlled()) && GetWorld()->TimeSince(InteractData.LastInteractionTimeCheck > InteractionCheckFrequency)) {

		PerformInteractionCheck();
	}

	if (IsLocallyControlled()) {

		const float DesiredFOV = IsAiming() ? AimingFOV : DefaultFOV;
		CameraComp->SetFieldOfView(FMath::FInterpTo(CameraComp->FieldOfView, DesiredFOV, DeltaTime, 10.f));

		if (EquippedWeapon != nullptr) {

			const FVector ADSLocation = EquippedWeapon->GetWeaponMesh()->GetSocketLocation(NAME_ADS_Socket);
			const FVector DefaultCameraLocation = GetMesh()->GetSocketLocation(FName("CameraSocket"));

			FVector CameraLocation = bIsAiming ? ADSLocation : DefaultCameraLocation;

			const float InterpSpeed = FVector::Dist(ADSLocation, DefaultCameraLocation) / EquippedWeapon->ADSTime;
			CameraComp->SetWorldLocation(FMath::VInterpTo(CameraComp->GetComponentLocation(), CameraLocation, DeltaTime, InterpSpeed));
		}
	}
}

void ASurvivalCharacter::Restart() {

	Super::Restart();

	if (ASurvivalPlayerController* PlayerController = Cast<ASurvivalPlayerController>(GetController())) {

		PlayerController->ShowIngameUI();
	}
}

void ASurvivalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASurvivalCharacter, LootSource);
	DOREPLIFETIME(ASurvivalCharacter, Killer);
	DOREPLIFETIME(ASurvivalCharacter, EquippedWeapon);
	DOREPLIFETIME(ASurvivalCharacter, bIsSprinting);

	DOREPLIFETIME_CONDITION(ASurvivalCharacter, bIsAiming, COND_SkipOwner);

	// Health is replicated only from Server to AutonomousProxy
	DOREPLIFETIME_CONDITION(ASurvivalCharacter, Health, COND_OwnerOnly);
}

void ASurvivalCharacter::SetActorHiddenInGame(bool bNewHidden) {

	Super::SetActorHiddenInGame(bNewHidden);

	if (EquippedWeapon) {

		EquippedWeapon->SetActorHiddenInGame(bNewHidden);
	}
}

#pragma region INTERACTION

void ASurvivalCharacter::PerformInteractionCheck() {

	if (GetController() == nullptr) return;

	InteractData.LastInteractionTimeCheck = GetWorld()->GetTimeSeconds();

	FVector EyesLocation;
	FRotator EyesRotation;

	GetController()->GetPlayerViewPoint(EyesLocation, EyesRotation);

	FVector TraceStart = EyesLocation;
	FVector TraceEnd = (EyesRotation.Vector() * InteractionCheckDistance) + TraceStart;

	FHitResult HitResult;

	FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, CollisionQueryParams)) {

		if (HitResult.GetActor() != nullptr) {

			if (UInteractionComponent* InteractionComp = Cast<UInteractionComponent>(HitResult.GetActor()->GetComponentByClass(UInteractionComponent::StaticClass()))) {

				float Distance = (TraceStart - HitResult.ImpactPoint).Size();

				if (InteractionComp != GetInteractable() && Distance <= InteractionComp->GetInteractionDistance()) {

					FoundNewInteractable(InteractionComp);
				}
				// found none or too far away
				else if (GetInteractable() && Distance > InteractionComp->GetInteractionDistance()) {

					CouldntFindInteractable();
				}

				return;
			}
		}
	}

	CouldntFindInteractable();
}

void ASurvivalCharacter::FoundNewInteractable(UInteractionComponent* FoundInteractable) {

	StopInteract();

	if(auto OldInteractible = GetInteractable()) {

		OldInteractible->EndFocus(this);
	}

	if (FoundInteractable != nullptr) {

		InteractData.ViewedInteractionComp = FoundInteractable;
		FoundInteractable->BeginFocus(this);
	}
}

void ASurvivalCharacter::CouldntFindInteractable() {

	// Lost focus on Interactable. Clear interaction timer.
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interaction)) {

		GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);
	}

	// Tell the interactable we no longer focusing on it. If any stored clear current.
	if (auto Interactable = GetInteractable()) {

		Interactable->EndFocus(this);

		if (InteractData.bInteractHeld) {

			Interactable->EndInteract(this);
		}
	}

	InteractData.ViewedInteractionComp = nullptr;
}

void ASurvivalCharacter::Interact() {

	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (UInteractionComponent* Interactable = GetInteractable()) {

		Interactable->Interact(this);
	}
}

void ASurvivalCharacter::BeginInteract() {

	/* If HasAuthority Perform directly InteractionCheck.
	* If !HasAuthority call Server to check.
	*/
	if (!HasAuthority()) {

		Server_StartInteract();
	}

	if (HasAuthority()) {

		PerformInteractionCheck();
	}

	InteractData.bInteractHeld = true;

	if (UInteractionComponent* Interactable = GetInteractable()) {

		Interactable->BeginInteract(this);

		// check interaction duration
		if (FMath::IsNearlyZero(Interactable->GetInteractionTime())) {

			Interact();
		}
		else {

			GetWorldTimerManager().SetTimer(TimerHandle_Interaction, this, &ASurvivalCharacter::Interact, Interactable->GetInteractionTime(), false);
		}
	}
}

void ASurvivalCharacter::StopInteract() {

	if (!HasAuthority()) {

		Server_StopInteract();
	}

	InteractData.bInteractHeld = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (UInteractionComponent* Interactable = GetInteractable()) {

		Interactable->EndInteract(this);
	}
}

bool ASurvivalCharacter::GetIsInteracting() const {

	// If timer is active we are in interaction
	return GetWorldTimerManager().IsTimerActive(TimerHandle_Interaction);
}

float ASurvivalCharacter::GetRemainingInteractionTime() const {

	return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interaction);
}

void ASurvivalCharacter::Server_StartInteract_Implementation() {
	BeginInteract();
}

void ASurvivalCharacter::Server_StopInteract_Implementation() {
	StopInteract();
}

bool ASurvivalCharacter::Server_StartInteract_Validate() {
	return true;
}

bool ASurvivalCharacter::Server_StopInteract_Validate() {
	return true;
}

#pragma endregion INTERACTION

#pragma region INVENTORY

USkeletalMeshComponent* ASurvivalCharacter::GetSlotSkeletalMeshComponent(const EEquippableSlot Slot) {

	if(PlayerMeshes.Contains(Slot)) {

		return *PlayerMeshes.Find(Slot);
	}

	return nullptr;
}

bool ASurvivalCharacter::EquipItem(class UEquippableItem* Item) {

	EquippedItems.Add(Item->Slot, Item);

	OnEquippmentChanged.Broadcast(Item->Slot, Item);

	return true;
}

bool ASurvivalCharacter::UnEquipItem(class UEquippableItem* Item) {

	if( Item != nullptr) {

		if (EquippedItems.Contains(Item->Slot)) {

			if (Item == *EquippedItems.Find(Item->Slot)) {

				EquippedItems.Remove(Item->Slot);

				OnEquippmentChanged.Broadcast(Item->Slot, nullptr);

				return true;
			}
		}
	}

	return false;
}

bool ASurvivalCharacter::EquipGear(class UGearItem* Gear) {

	if (Gear != nullptr) {

		if (USkeletalMeshComponent* GearMesh = *PlayerMeshes.Find(Gear->Slot)) {

			GearMesh->SetSkeletalMesh(Gear->GetGearMesh());
			// update the last one
			GearMesh->SetMaterial(GearMesh->GetMaterials().Num() - 1, Gear->MaterialInstance);
		}
	}

	return true;
}

bool ASurvivalCharacter::UnEquipGear(const EEquippableSlot Slot) {

	if (USkeletalMeshComponent* GearMesh = *PlayerMeshes.Find(Slot)) {

		if (USkeletalMesh* BodyMesh = *NakedMeshes.Find(Slot)) {

			GearMesh->SetSkeletalMesh(BodyMesh);
			// Set default material
			GearMesh->SetMaterial(0, BodyMesh->Materials[0].MaterialInterface);

			return true;
		}
		else {
			// Some slots do not have naked version (backpack for instance), then set Skeletel mesh to nullptr
			GearMesh->SetSkeletalMesh(nullptr);
			return true;
		}
	}

	return false;
}

bool ASurvivalCharacter::EquipWeapon(class UWeaponItem* WeaponItem) {

	if (HasAuthority()) {

		if (WeaponItem == nullptr) return false;

		if (WeaponItem->WeaponClass == nullptr) return false;
		 
		if (EquippedWeapon != nullptr) UnEquipWeapon();

		FActorSpawnParameters SpawnParams;
			SpawnParams.bNoFail = true;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Owner = SpawnParams.Instigator = this;

		if (AWeaponActor* NewWeapon = GetWorld()->SpawnActor<AWeaponActor>(WeaponItem->WeaponClass, SpawnParams)) {

			NewWeapon->Item = WeaponItem;

			EquippedWeapon = NewWeapon;
			OnRep_Weapon();

			NewWeapon->OnEquip();

			return true;
		}
	}

	return false;
}

bool ASurvivalCharacter::UnEquipWeapon() {

	if (HasAuthority()) {

		if (EquippedWeapon == nullptr) return false;

		EquippedWeapon->OnUnEquip();
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;

		OnRep_Weapon();
		return true;
	}

	return false;
}

void ASurvivalCharacter::UseItem(class UItem* Item) {

	if (Role < ROLE_Authority) {

		if (Item != nullptr) {

			if (ASurvivalPlayerController* PlayerCon = Cast<ASurvivalPlayerController>(GetController())) {

				FString MessageText = "Using Item: ";
				MessageText = MessageText.Append(Item->GetItemName().ToString());
				FText Message = FText::FromString(MessageText);

				//PlayerCon->ClientShowNotification(Message);
				PlayerCon->ShowNotificationMessage(Message);
			}

			ServerUseItem(Item);
		}
	}
	if (HasAuthority()) {

		if (InventoryComponent != nullptr && !InventoryComponent->FindItem(Item)) {
			// Player is tryuing to use item he does not possess on server
			return;
		}
	}

	if (Item != nullptr) {

		Item->OnUse(this);
		Item->Use(this);
	}
}

void ASurvivalCharacter::ServerUseItem_Implementation(class UItem* Item){
	UseItem(Item);
}

bool ASurvivalCharacter::ServerUseItem_Validate(class UItem* Item){
	return true;
}

void ASurvivalCharacter::DropItem(class UItem* Item, const int32 Quantity) {

	if (InventoryComponent != nullptr && Item != nullptr && InventoryComponent->FindItem(Item)) {

		if (Role < ROLE_Authority) {

			ServerDropItem(Item, Quantity);
			return;
		}

		if (HasAuthority()) {

			const int32 ItemQuantity = Item->GetQuantity();
			const int32 DroppedQuantity = InventoryComponent->ConsumeItem(Item, Quantity);

			FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.bNoFail = true;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			// Just visual thing no need to adjust Z
			FVector SpawnLocation = GetActorLocation();
				SpawnLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

			ensure(PickupClass);

			if (APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams)) {

				Pickup->InitializePickup(Item->GetClass(), DroppedQuantity);
			}
		}
	}
}

void ASurvivalCharacter::ServerDropItem_Implementation(class UItem* Item, const int32 Quantity){

	DropItem(Item, Quantity);
}

bool ASurvivalCharacter::ServerDropItem_Validate(class UItem* Item, const int32 Quantity){
	return true;
}

#pragma endregion INVENTORY

#pragma region LOOTING

bool ASurvivalCharacter::GetIsLooting() const {

	return LootSource != nullptr;
}

void ASurvivalCharacter::LootItem(class UItem* ItemToGive) {

	if (HasAuthority()) {

		// Check if Inventory & LootSource are valid and if Loot source has the Item we want to take
		if (InventoryComponent != nullptr && LootSource != nullptr && LootSource->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity())) {

			const FItemAddResult AddResult = InventoryComponent->TryAddItem(ItemToGive);

			// If we added any item, Consume the item from looting source
			if (AddResult.AmountGiven > 0) {

				LootSource->ConsumeItem(ItemToGive, AddResult.AmountGiven);
			}
			else {

				// In case we could not add item, show the resason
				if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController())) {

					//PC->ClientShowNotification(AddResult.ErrorText);
					PC->ShowNotificationMessage(AddResult.ErrorText);
				}
			}
		}
	}
	else {

		ServerLootItem(ItemToGive);
	}
}

void ASurvivalCharacter::ServerLootItem_Implementation(class UItem* ItemTogive) {

	LootItem(ItemTogive);
}

bool ASurvivalCharacter::ServerLootItem_Validate(class UItem* ItemTogive) {

	return true;
}

void ASurvivalCharacter::BeginLootingPlayer(class ASurvivalCharacter* Character) {

	if (Character != nullptr) {

		Character->SetLootingSource(InventoryComponent);
	}
}

void ASurvivalCharacter::OnLootSourceOwnerDestroyed(AActor* DestroyedActor) {

	// Remove all looting source
	if (HasAuthority() && LootSource != nullptr && DestroyedActor == LootSource->GetOwner()) {

		ServerSetLootingSource(nullptr);
	}
}

void ASurvivalCharacter::OnRep_LootSource() {

	if (ASurvivalPlayerController * PC = Cast<ASurvivalPlayerController>(GetController())) {

		if (PC->IsLocalController()) {

			if (LootSource != nullptr) {

				PC->ShowLootMenu(LootSource);
			}
			else {

				PC->HideLootMenu();
			}
		}
	}
}

void ASurvivalCharacter::SetLootingSource(class UInventoryComponent* NewLootingSource) {

	/** If the item we are looting gets destroyed, we need to tell the client to remove their loot from screen.*/
	if (NewLootingSource != nullptr && NewLootingSource->GetOwner() != nullptr) {

		NewLootingSource->GetOwner()->OnDestroyed.AddUniqueDynamic(this, &ASurvivalCharacter::OnLootSourceOwnerDestroyed);
	}

	if (HasAuthority()) {

		if (NewLootingSource != nullptr) {

			// Looting player keeps their body alive for an extra 2 minuts to provide enought time to loot their items
			if (ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(NewLootingSource->GetOwner())) {

				Character->SetLifeSpan(120.f);
			}
		}

		LootSource = NewLootingSource;
		OnRep_LootSource();
	}
	else {

		ServerSetLootingSource(NewLootingSource);
	}
}

void ASurvivalCharacter::ServerSetLootingSource_Implementation(class UInventoryComponent* NewLootSource) {

	SetLootingSource(NewLootSource);
}

bool ASurvivalCharacter::ServerSetLootingSource_Validate(class UInventoryComponent* NewLootSource) {

	return true;
}

#pragma endregion LOOTING

#pragma region HEALTH

float ASurvivalCharacter::ModifyHealth(const float DeltaHealth) {

	const float OldHealth = Health;

	Health = FMath::Clamp<float>(Health + DeltaHealth, 0.f, MaxHealth);

	OnRep_Health(OldHealth);

	return Health - OldHealth;
}

void ASurvivalCharacter::OnRep_Health(float OldHealth) {

	OnHealthModified(Health - OldHealth);
}

void ASurvivalCharacter::OnRep_Killer() {

	SetLifeSpan(DeadBodyLifespan);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMovementComponent()->SetIsReplicated(false);

	DeadBodyInteractionComponent->Activate();

	// Unequip all equipment to make items visible in inventory again
	if (HasAuthority()) {

		TArray<UEquippableItem*> Equipment;
		EquippedItems.GenerateValueArray(Equipment);

		for (auto& EquippedItem : Equipment) {

			EquippedItem->SetEquipped(false);
		}
	}

	if (IsLocallyControlled()) {

		SpringArm->TargetArmLength = 500.f;
			bUseControllerRotationPitch = true;
			bUseControllerRotationRoll = true;
			bUseControllerRotationYaw = true;

		FVector NewLocation = GetActorLocation();
			NewLocation.Z += 250.f;
			SpringArm->SetWorldLocation(NewLocation);
/*			SpringArm->DetachFromParent(true);*/

		if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController())) {

			PC->ShowDeathScreen(Killer);
		}
	}

	OnDeath();
}

void ASurvivalCharacter::KilledSelf(struct FDamageEvent const& DamageEvent, const AActor* DamageCauser) {

	Killer = this;
	OnRep_Killer();
}

void ASurvivalCharacter::KilledBy(struct FDamageEvent const& DamageEvent, const class AController* EventInstigator, const AActor* DamageCauser) {

	if (EventInstigator != nullptr) {

		if (auto TempKiller = Cast<ASurvivalCharacter>(EventInstigator->GetPawn())) {

			Killer = TempKiller;
			OnRep_Killer();
		}
	}
}

float  ASurvivalCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) {

	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	const float DeltaDamage = ModifyHealth(-Damage);

	if (Health <= 0) {

		if (EventInstigator != GetController()) {

			KilledBy(DamageEvent,EventInstigator, DamageCauser);
		}
		else {

			KilledSelf(DamageEvent, this);
		}
	}

	return DeltaDamage;
}

#pragma endregion HEALTH

#pragma region COMBAT_MELEE

void ASurvivalCharacter::BeginMeleeAttack() {

	if (MeleeAttackMontage && MeleeDamageTypeClass) {

		// If passed the time of at lest montage duration
		if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength()) {

			FVector StartTrace = CameraComp->GetComponentLocation();
			FVector EndTrace = (CameraComp->GetComponentRotation().Vector() * MeleeAttackDistance) + StartTrace;

			FHitResult HitResult;
			FCollisionShape Shape = FCollisionShape::MakeSphere(15.f);

			FCollisionQueryParams QueryParams;
				QueryParams.bTraceComplex = true;
				QueryParams.AddIgnoredActor(this);

			PlayAnimMontage(MeleeAttackMontage);

			if (GetWorld()->SweepSingleByChannel(HitResult, StartTrace, EndTrace, FQuat(), COLLISION_WEAPON, Shape, QueryParams)) {

				if (ASurvivalCharacter* HitPlayer = Cast<ASurvivalCharacter>(HitResult.GetActor())) {

					if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController())) {

						PC->OnHitPlayer();
					}
				}
			}

			Server_MeleeAttack(HitResult);

			LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
		}
	}
}

void ASurvivalCharacter::MulticastPlayMeleeFX_Implementation() {

	if (!IsLocallyControlled()) {

		if (MeleeAttackMontage != nullptr) {

			PlayAnimMontage(MeleeAttackMontage);
		}
	}
}

void ASurvivalCharacter::Server_MeleeAttack_Implementation(const FHitResult& MeleeHit) {


	if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength() && (GetActorLocation() - MeleeHit.ImpactPoint).Size() <= MeleeAttackDistance) {

		MulticastPlayMeleeFX();

		if (MeleeHit.GetActor() != nullptr) {

			UGameplayStatics::ApplyPointDamage(MeleeHit.GetActor(), MeleeAttackDamage, (MeleeHit.TraceStart - MeleeHit.TraceEnd).GetSafeNormal(), MeleeHit, GetController(), this, MeleeDamageTypeClass->StaticClass());
		}
	}

	LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
}

bool ASurvivalCharacter::Server_MeleeAttack_Validate(const FHitResult& MeleeHit) {
	return true;
}

#pragma endregion COMBAT_MELEE

#pragma region WEAPON

void ASurvivalCharacter::OnRep_Weapon() {

	if (EquippedWeapon != nullptr) {

		EquippedWeapon->OnEquip();
	}
}

bool ASurvivalCharacter::CanAim() const {

	return EquippedWeapon != nullptr;
}

void ASurvivalCharacter::StartAiming() {

	if(CanAim()) {

		SetAiming(true);
	}
}

void ASurvivalCharacter::StopAiming() {

	SetAiming(false);
}

void ASurvivalCharacter::SetAiming(const bool NewIsAiming) {

	if (!CanAim() || bIsAiming == NewIsAiming) return;

	if (Role < ROLE_Authority) {

		ServerSetAiming(NewIsAiming);
	}

	bIsAiming = NewIsAiming;
}

void ASurvivalCharacter::ServerSetAiming_Implementation(const bool NewIsAiming) {

	SetAiming(NewIsAiming);
}

bool ASurvivalCharacter::ServerSetAiming_Validate(const bool NewIsAiming) {

	return true;
}

void ASurvivalCharacter::StartReload() {

	if (EquippedWeapon) {

		EquippedWeapon->StartReload();
	}
}

#pragma endregion WEAPON

#pragma region THROWABLE_ITEM

UThrowableItem* ASurvivalCharacter::GetThrowableItem() const {

	UThrowableItem* EquippedThrowable = nullptr;

	if (EquippedItems.Contains(EEquippableSlot::EES_Throwable)) {

		EquippedThrowable = Cast< UThrowableItem>(*EquippedItems.Find(EEquippableSlot::EES_Throwable));
	}

	return EquippedThrowable;
}

void ASurvivalCharacter::MulticastPlayThrowableTossFX_Implementation(UAnimMontage* MontageToPlay) {

	if (GetNetMode() != NM_DedicatedServer && !IsLocallyControlled() && MontageToPlay != nullptr) {

		PlayAnimMontage(MontageToPlay);
	}
}

void ASurvivalCharacter::ServerUseThrowable_Implementation() {

	UseThrowable();
}

bool ASurvivalCharacter::ServerUseThrowable_Validate() {

	return true;
}

void ASurvivalCharacter::UseThrowable() {

	if (CanUseThrowable()) {

		if (UThrowableItem* ThrowableItem = GetThrowableItem()) {

			if (Role == ROLE_Authority) {

				SpawnThrowable();

				if (InventoryComponent) {

					InventoryComponent->ConsumeItem(ThrowableItem, 1);
				}
			}
			else {

				// To make sure UI on client side updades correctly
				if (ThrowableItem->GetQuantity() <= 1) {

					EquippedItems.Remove(EEquippableSlot::EES_Throwable);
					OnEquippmentChanged.Broadcast(EEquippableSlot::EES_Throwable, nullptr);
				}

				PlayAnimMontage(ThrowableItem->ThrowableTossAnim);
				ServerUseThrowable();
			}
		}
	}
	else {

		if (ASurvivalPlayerController* PlayerCon = Cast<ASurvivalPlayerController>(GetController())) {

			PlayerCon->ShowNotificationMessage(FText::FromString("No throwable items in inventory."));
		}
	}
}

void ASurvivalCharacter::SpawnThrowable() {

	if (HasAuthority()) {

		if (UThrowableItem* ThrowableItem = GetThrowableItem()) {

			if (ThrowableItem->ThrowableWeaponClass) {

				FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = SpawnParams.Instigator = this;
					SpawnParams.bNoFail = true;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				FVector CamLoc;
				FRotator CamRot;

				GetController()->GetPlayerViewPoint(CamLoc, CamRot);

				// Spawn a bit in front of the player
				CamLoc = (CamRot.Vector() * 20.f) + CamLoc;

				if (AThrowableWeapon* ThrowableWeapon = GetWorld()->SpawnActor<AThrowableWeapon>(ThrowableItem->ThrowableWeaponClass, FTransform(CamRot, CamLoc), SpawnParams)) {

					MulticastPlayThrowableTossFX(ThrowableItem->ThrowableTossAnim);
				}
			}
		}
	}
}

bool ASurvivalCharacter::CanUseThrowable() const {

	return GetThrowableItem() != nullptr && GetThrowableItem()->ThrowableWeaponClass != nullptr;
}

#pragma endregion THROWABLE_ITEM

#pragma region INPUTSETUP

void ASurvivalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASurvivalCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASurvivalCharacter::MoveRight);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASurvivalCharacter::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ASurvivalCharacter::StopSprint);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASurvivalCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASurvivalCharacter::StopCrouching);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ASurvivalCharacter::BeginInteract);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &ASurvivalCharacter::StopInteract);

	PlayerInputComponent->BindAction("Shoot", IE_Pressed, this, &ASurvivalCharacter::StartFire);
	PlayerInputComponent->BindAction("Shoot", IE_Released, this, &ASurvivalCharacter::StopFire);

	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ASurvivalCharacter::StartAiming);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ASurvivalCharacter::StopAiming);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASurvivalCharacter::StartReload);

	PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &ASurvivalCharacter::UseThrowable);
}

void ASurvivalCharacter::MoveForward(float Value) {
	AddMovementInput(GetActorForwardVector(), Value);
}

void ASurvivalCharacter::MoveRight(float Value) {
	AddMovementInput(GetActorRightVector(), Value);
}

void ASurvivalCharacter::LookUp(float Value) {
	AddControllerPitchInput(Value);
}

void ASurvivalCharacter::Turn(float Value ) {
	AddControllerYawInput(Value);
}

void ASurvivalCharacter::StartCrouching() {
	Crouch();
}

void ASurvivalCharacter::StopCrouching() {
	UnCrouch();
}

void ASurvivalCharacter::StartFire() {

	if (EquippedWeapon != nullptr) {

		EquippedWeapon->StartFire();
	}
	else BeginMeleeAttack();
}

void ASurvivalCharacter::StopFire() {

	if (EquippedWeapon != nullptr) EquippedWeapon->StopFire();
}

#pragma endregion INPUTSETUP

bool ASurvivalCharacter::CanSprint() const
{
	return !IsAiming();
}

void ASurvivalCharacter::StartSprint()
{
	SetSprinting(true);
}

void ASurvivalCharacter::StopSprint()
{
	SetSprinting(false);
}

void ASurvivalCharacter::SetSprinting(bool NewSprinting)
{
	if (NewSprinting == bIsSprinting) return;

	if (Role < ROLE_Authority) {

		ServerSetSprinting(NewSprinting);
	}
	bIsSprinting = NewSprinting;

	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? MaxSprintSpeed : WalkSpeed;
}

void ASurvivalCharacter::ServerSetSprinting_Implementation(bool NewSprinting)
{
	SetSprinting(NewSprinting);
}

bool ASurvivalCharacter::ServerSetSprinting_Validate(bool NewSprinting)
{
	return true;
}

#undef LOCTEXT_NAMESPACE