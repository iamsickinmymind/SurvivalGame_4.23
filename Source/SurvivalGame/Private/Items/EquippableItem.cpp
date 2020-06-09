// All rights reserved Dominik Pavlicek


#include "EquippableItem.h"
#include "Net/UnrealNetwork.h"

#include "Character/SurvivalCharacter.h"
#include "Components/InventoryComponent.h"

#define LOCTEXT_NAMESPACE "EquippableItem"

UEquippableItem::UEquippableItem() {

	bStackable = false;
	bIsEquiped = false;
	ItemActionText = LOCTEXT("ItemUseActionText", "Equip");
}

void UEquippableItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquippableItem, bIsEquiped);
}

// overrides Use function of Item
// Where Items is consumed, Equippable Item is equipped
void UEquippableItem::Use(class ASurvivalCharacter* Character) {

	if (Character != nullptr && Character->HasAuthority()) {

		if (Character->GetEquippment().Contains(Slot) && !bIsEquiped) {

			UEquippableItem* AlreadyEquipped = *Character->GetEquippment().Find(Slot);

			if (AlreadyEquipped != nullptr) {

				AlreadyEquipped->SetEquipped(false);
			}
		}

		SetEquipped(!IsEquipped());
	}
}

bool UEquippableItem::ShouldShowInInventory() const {

	// If not equipped, show
	// Vice versa
	return !bIsEquiped;
}

void UEquippableItem::AddedToInventory(class UInventoryComponent* Inventory) {

	if (ASurvivalCharacter* Player = Cast <ASurvivalCharacter>(Inventory->GetOwner())) {

		// Do not auto equip if looting from pasiv, eg from chest
		if (Player != nullptr && !Player->GetIsLooting()) {

			// If Player does not have this slot occupied, then Equip
			if (!(Player->GetEquippment().Contains(Slot))) {

				SetEquipped(true);
			}
		}
	}
}

bool UEquippableItem::Equip(class ASurvivalCharacter* Character) {

	if(Character != nullptr) {

		return Character->EquipItem(this);
	}

	return false;
}

bool UEquippableItem::UnEquip(class ASurvivalCharacter* Character) {

	if (Character != nullptr) {

		return Character->UnEquipItem(this);
	}

	return false;
}

void UEquippableItem::SetEquipped(bool NewEquipped) {

	bIsEquiped = NewEquipped;

	OnRep_EquipStatusChanged();

	MarkDirtyForReplication();
}

void UEquippableItem::OnRep_EquipStatusChanged() {

	if(ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(GetOuter())) {

		if (bIsEquiped) {

			Equip(Character);
		}
		else {

			UnEquip(Character);
		}
	}

	OnItemModified.Broadcast();
}

#undef LOCTEXT_NAMESPACE