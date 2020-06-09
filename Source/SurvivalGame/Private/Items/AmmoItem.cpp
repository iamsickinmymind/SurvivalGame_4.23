// All rights reserved Dominik Pavlicek


#include "AmmoItem.h"

#define LOCTEXT_NAMESPACE "AmmoItem"

UAmmoItem::UAmmoItem() {

	ItemActionText = LOCTEXT("AmmoActionText", "Take");
	ItemDisplayName = LOCTEXT("AmmoNameText", "Ammonution");
	ItemDescription = LOCTEXT("AmmoDescription", "Stack of ammonution.");
	ItemRariry = EItemRarity::EIR_Common;
	Weight = 0.25;
	MaxStackSize = 90;
	Quantity = 15;
	bStackable = true;
}

#undef LOCTEXT_NAMESPACE