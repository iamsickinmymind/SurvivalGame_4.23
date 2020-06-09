// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "EquippableItem.generated.h"

UENUM(BlueprintType)
enum class EEquippableSlot : uint8 {

	EES_Head			UMETA(DisplayName = "Head"),
	EES_Helmet			UMETA(DisplayName = "Helmet"),
	EES_Torso			UMETA(DisplayName = "Torso"),
	EES_Vest			UMETA(DisplayName = "Vest"),
	EES_Legs			UMETA(DisplayName = "Legs"),
	EES_Feet			UMETA(DisplayName = "Feet"),
	EES_Hands			UMETA(DisplayName = "Hands"),
	EES_Backpack		UMETA(DisplayName = "Backpack"),
	EES_PrimaryWeapon	UMETA(DisplayName = "Primary Weapon"),
	EES_Throwable		UMETA(DisplayName = "Throwable"),
	ESS_DEFAULT			UMETA(DisplayName = "Default")
};

/**
 * 
 */
UCLASS(Abstract, NotBlueprintable)
class SURVIVALGAME_API UEquippableItem : public UItem
{
	GENERATED_BODY()
	

public:

	UEquippableItem();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;

	virtual void Use(class ASurvivalCharacter* Character) override;

	virtual bool ShouldShowInInventory() const override;

	virtual void AddedToInventory(class UInventoryComponent* Inventory) override;

	// Tries to equip selected Equippable Item to selected Character
	UFUNCTION(BlueprintCallable, Category = "Equippment")
	virtual bool Equip(class ASurvivalCharacter* Character);

	// Tries to un-equip selected Equippable Item to selected Character
	UFUNCTION(BlueprintCallable, Category = "Equippment")
	virtual bool UnEquip(class ASurvivalCharacter* Character);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsEquipped() { return bIsEquiped; };

	// Call this on server side to set equip this item
	void SetEquipped(bool NewEquipped);

protected:

	UFUNCTION()
	void OnRep_EquipStatusChanged();

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
	EEquippableSlot Slot;

protected:

	UPROPERTY(ReplicatedUsing=OnRep_EquipStatusChanged)
	bool bIsEquiped;
};
