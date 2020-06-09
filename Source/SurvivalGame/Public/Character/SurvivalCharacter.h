// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Items/EquippableItem.h"
#include "SurvivalCharacter.generated.h"

//@TODO:
// Implement New Component - HealthComponent
// Replace actual Health system with HealthComponent
// Replicate component

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquippmentChanged, const EEquippableSlot, Slot, const UEquippableItem*, Item);

class USkeletalMeshComponent;
class UInteractionComponent;
class UAnimMontage;
class AThrowableItem;

USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()

		FInteractionData() {

		ViewedInteractionComp = nullptr;
		LastInteractionTimeCheck = 0.f;
		bInteractHeld = false;
	}

	// Last seen Int. comp.
	UPROPERTY()
	class UInteractionComponent* ViewedInteractionComp;

	// Last time we checked for an interactable
	UPROPERTY()
	float LastInteractionTimeCheck;

	// Whether the local player holds the interactable
	UPROPERTY()
	bool bInteractHeld;
};

UCLASS()
class SURVIVALGAME_API ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASurvivalCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void SetActorHiddenInGame(bool bNewHidden) override;

#pragma region INTERACTION_public

	// Returns whether we are interacting with interactable item (eg. holding E key for unlocking door which takes 3 seconds)
	bool GetIsInteracting() const;

	// Returns how much time of Interaction time needed to interact is left to consume (eg. holding E key for unlocking door which takes 3 seconds but already 2 seconds passed, then return 1s)
	float GetRemainingInteractionTime() const;

#pragma endregion INTERACTION_public

#pragma region INVENTORY_public

	FORCEINLINE class UInventoryComponent* GetPlayerInventory() const { return InventoryComponent; };

	// Gets all equipped items from players
	UFUNCTION(BlueprintPure)
	FORCEINLINE TMap<EEquippableSlot, UEquippableItem*> GetEquippment() const { return  EquippedItems; };

	// Returns Skeletal Mesh Component that occupies given slot.
	// If none found returns NULL value
	UFUNCTION(BlueprintPure, Category = "Player|Inventory")
	class USkeletalMeshComponent* GetSlotSkeletalMeshComponent(const EEquippableSlot Slot);

	bool EquipItem(class UEquippableItem* Item);
	bool UnEquipItem(class UEquippableItem* Item);

	bool EquipGear(class UGearItem* Gear);
	bool UnEquipGear(const EEquippableSlot Slot);

	bool EquipWeapon(class UWeaponItem* WeaponItem);
	bool UnEquipWeapon();

#pragma endregion INVENTORY_public

#pragma region LOOTING_public

	UFUNCTION(BlueprintCallable, Category = "Player|Looting")
	void LootItem(class UItem* ItemToGive);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLootItem(class UItem* ItemTogive);

	UFUNCTION(BlueprintCallable, Category = "Player|Looting")
	void SetLootingSource(class UInventoryComponent* NewLootingSource);

	UFUNCTION(BlueprintPure, Category = "Player|Inventory")
	bool GetIsLooting() const;

#pragma endregion LOOTING_public

#pragma region WEAPON_public

	UFUNCTION()
	void OnRep_Weapon();

	// Whether player aims or not
	UFUNCTION(BlueprintPure, Category = "Player|Firing")
	FORCEINLINE bool IsAiming() const { return bIsAiming; };

	// Returns currently equipped Weapon
	UFUNCTION(BlueprintPure, Category = "Player|Weapon")
	FORCEINLINE	class AWeaponActor* GetEquippedWeapon() const { return EquippedWeapon; };

	UFUNCTION()
	void StartReload();

#pragma endregion WEAPON_public

#pragma region HEALTH_public

	UFUNCTION(BlueprintPure, Category = "Player|Health")
	FORCEINLINE bool IsAlive() const { return ((Health > 0.0f) || (Health > KINDA_SMALL_NUMBER) && (Killer == nullptr)); };

	float ModifyHealth(const float DeltaHealth);

#pragma endregion HEALTH_public

	UFUNCTION(BlueprintPure, Category = "Equippment")
	UThrowableItem* GetThrowableItem() const;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Restart() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void LookUp(float Value);
	void Turn(float Value);

	void StartCrouching();
	void StopCrouching();

	UFUNCTION(BlueprintCallable)
	void StartFire();
	void StopFire();

	bool CanSprint() const;
	void StartSprint();
	void StopSprint();
	void SetSprinting(bool NewSprinting);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetSprinting(bool NewSprinting);

#pragma region INTERACTION_protected
	void BeginInteract();
	void StopInteract();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartInteract();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StopInteract();

	void Interact();

	void PerformInteractionCheck();

	void CouldntFindInteractable();
	void FoundNewInteractable(UInteractionComponent* FoundInteractable);

	FORCEINLINE UInteractionComponent* GetInteractable() const { return InteractData.ViewedInteractionComp; };
#pragma endregion INTERACTION_protected

#pragma region INVENTORY_protected

	// Based on authority calls Inventory to handle Using or asks server to do so
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItem(class UItem* Item);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseItem(class UItem* Item);

	// Based on authority call Inventory to handle dropping or asks server to do so
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItem(class UItem* Item, const int32 Quantity);	
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropItem(class UItem* Item, const int32 Quantity);

#pragma endregion INVENTORY_protected

#pragma region LOOTING_protected

	UFUNCTION()
	void BeginLootingPlayer(class ASurvivalCharacter* Character);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerSetLootingSource(class UInventoryComponent* NewLootSource);

	UFUNCTION()
	void OnLootSourceOwnerDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnRep_LootSource();

#pragma endregion LOOTING_protected

#pragma region HEALTH_protected

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthModified(const float DeltaHealth);

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION(BlueprintImplementableEvent, category = "Player|Health")
	void OnDeath();

	UFUNCTION()
	void OnRep_Killer();

	/** This function should be called when player dies without any interference of other players, falling from cliff, for instance.*/ 
	UFUNCTION()
	void KilledSelf(struct FDamageEvent const& DamageEvent, const AActor* DamageCauser);

	/** This function should be called when the player dies based on damage dealt by other player.*/
	UFUNCTION()
	void KilledBy(struct FDamageEvent const& DamageEvent, const class AController* EventInstigator, const AActor* DamageCauser);

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

#pragma endregion HEALTH_protected

#pragma region COMBAT_protected

	void BeginMeleeAttack();

	/** Process the melee hit.*/
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MeleeAttack(const FHitResult& MeleeHit);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeFX();

#pragma endregion COMBAT_protected

#pragma region FIREWEAPON_protected

	bool CanAim() const;

	void StartAiming();
	void StopAiming();

	void SetAiming(const bool NewIsAiming);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetAiming(const bool NewIsAiming);

#pragma endregion FIREWEAPON_protected

#pragma region THROWABLE_protected

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayThrowableTossFX(UAnimMontage* MontageToPlay);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseThrowable();

	void UseThrowable();
	void SpawnThrowable();
	bool CanUseThrowable() const;

#pragma endregion THROWABLE_protected

public:

	UPROPERTY(BlueprintAssignable)
	FOnEquippmentChanged OnEquippmentChanged;

protected:

#pragma region COMPONENTS

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInventoryComponent* InventoryComponent = nullptr;

	/** Interaction component spawned when this player dies.*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Dead Body Interaction Component"))
	UInteractionComponent* DeadBodyInteractionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCameraComponent* CameraComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* HelmetMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* TorsoMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* LegsMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* FeetMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* VestMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* HandsMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* BackpackMesh = nullptr;

#pragma endregion COMPONENTS

#pragma region INTERACTION_protected_variables

	// How ofter we check for interactive object
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency;

	// How far can player check for interactable
	// Basically a sight distance
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckDistance;

	UPROPERTY()
	FInteractionData InteractData;

#pragma endregion INTERACTION_protected_variables

#pragma region INVENTORY_protected_variables

	// We need this variable because BP are using BP base class
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	TSubclassOf<class APickup> PickupClass;

	// The player body meshes map to equipment slots
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TMap<EEquippableSlot, USkeletalMeshComponent*> PlayerMeshes;

	// Default mesh if no equippment available
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TMap<EEquippableSlot, USkeletalMesh*> NakedMeshes;

	// The player equipped items map to equipment slots
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TMap<EEquippableSlot, UEquippableItem*> EquippedItems;

#pragma endregion INVENTORY_protected_variables

#pragma region LOOTING_protected_variables

	UPROPERTY(ReplicatedUsing = OnRep_LootSource, BlueprintReadOnly)
	UInventoryComponent* LootSource = nullptr;

#pragma endregion LOOTING_protected_variables

#pragma region HEALTH_protected_variables

	UPROPERTY(ReplicatedUsing = OnRep_Health, EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (UIMin = 0, ClampMin = 0))
	float MaxHealth;

	/** How many seconds it take to dematerialize the dead body.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (UIMin = 20, ClampMin = 20))
	float DeadBodyLifespan;

	UPROPERTY(ReplicatedUsing = OnRep_Killer, VisibleAnywhere, BlueprintReadOnly, Category = "Player|Health")
	class ASurvivalCharacter* Killer = nullptr;

#pragma endregion HEALTH_protected_variables

#pragma region MELEE_protected_variables

	UPROPERTY()
	float LastMeleeAttackTime;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Melee")
	float MeleeAttackDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Melee")
	float MeleeAttackDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Melee")
	TSubclassOf<class UMeleeDamage> MeleeDamageTypeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Melee")
	class UAnimMontage* MeleeAttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TArray<class UAnimMontage*> HitFX_Animations_Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TArray<class UParticleSystem*> HitFX_Emitters_Melee;

#pragma endregion MELEE_protected_variables

#pragma region WEAPON_protected_variables

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Weapon)
	class AWeaponActor* EquippedWeapon = nullptr;

	UPROPERTY(Transient, Replicated)
	bool bIsAiming;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|POV")
	float AimingFOV;

	float DefaultFOV;

#pragma endregion WEAPON_protected_variables

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MaxSprintSpeed;

	UPROPERTY()
	float WalkSpeed;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

private:

	FTimerHandle TimerHandle_Interaction;
};
