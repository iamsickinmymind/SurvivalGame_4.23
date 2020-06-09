// All rights reserved Dominik Pavlicek


#include "WeaponItem.h"

#include "Character/SurvivalCharacter.h"
#include "Character/SurvivalPlayerController.h"

#define LOCTEXT_NAMESPACE "WeaponItem"

UWeaponItem::UWeaponItem() {

	ItemActionText = LOCTEXT("WeaponActionText", "Take");
	ItemDisplayName = LOCTEXT("WeaponNameText", "Weapon");
	ItemDescription = LOCTEXT("WeaponDescription", "It can hurt people.");
	ItemRariry = EItemRarity::EIR_Common;
	Weight = 5.f;
	MaxStackSize = 1;
	Quantity = 1;
	bStackable = false;
}

bool UWeaponItem::Equip(class ASurvivalCharacter* Character) {

	bool bSuccess = Super::Equip(Character);

	if (bSuccess && Character != nullptr) {

		bSuccess = Character->EquipWeapon(this);
	}

	return bSuccess;
}

bool UWeaponItem::UnEquip(class ASurvivalCharacter* Character) {

	bool bSuccess = Super::UnEquip(Character);

	if (bSuccess && Character != nullptr) {

		bSuccess = Character->UnEquipWeapon();
	}

	return bSuccess;
}

#undef LOCTEXT_NAMESPACE