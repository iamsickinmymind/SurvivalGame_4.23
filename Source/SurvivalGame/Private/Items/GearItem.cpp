// All rights reserved Dominik Pavlicek


#include "GearItem.h"

#include "Character/SurvivalCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "GearItem"
UGearItem::UGearItem() {

	DamageDefenseMultiplier = 0.1;
	ItemActionText = LOCTEXT("GearActionText", "Pick up");
}

bool UGearItem::Equip(class ASurvivalCharacter* Character) {

	bool bEquipSuccessful = Super::Equip(Character);

	if (bEquipSuccessful && Character != nullptr) {

		bEquipSuccessful = Character->EquipGear(this);
	}

	return bEquipSuccessful;
}

bool UGearItem::UnEquip(class ASurvivalCharacter* Character) {

	bool bEquipSuccessful = Super::UnEquip(Character);

	if (bEquipSuccessful && Character != nullptr) {

		bEquipSuccessful = Character->UnEquipGear(Slot);
	}

	return bEquipSuccessful;
}

#undef LOCTEXT_NAMESPACE