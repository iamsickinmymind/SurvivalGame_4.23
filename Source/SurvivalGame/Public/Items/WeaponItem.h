// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "WeaponItem.generated.h"

/**
 * Default Weapon item Class.
 */
UCLASS(Blueprintable)
class SURVIVALGAME_API UWeaponItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UWeaponItem();

	virtual bool Equip(class ASurvivalCharacter* Character) override;
	virtual bool UnEquip(class ASurvivalCharacter* Character) override;

public:

	// The weapon class to give to the player when equipping this item
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
	TSubclassOf<class AWeaponActor> WeaponClass;

};
