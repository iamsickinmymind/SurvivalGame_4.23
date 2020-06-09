// All rights reserved Dominik Pavlicek

#include "Item.h"
#include "Net/UnrealNetwork.h"
#include "Components/InventoryComponent.h"
#include "ThumbnailRendering/ClassThumbnailRenderer.h"

#define LOCTEXT_NAMESPACE "Item"

UItem::UItem() {
	// LOCTEXT is used for localization of the FText
	ItemDisplayName = LOCTEXT("ItemName", "Item");
	ItemActionText = LOCTEXT("ItemUseActionText", "Use");
	Weight = 0.f;
	Quantity = 1;
	bStackable = true;
	MaxStackSize = 2;
	RepKey = 0;

}

#if WITH_EDITOR
void UItem::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) {

	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// Check if the changed property name is the name of the Quantity variable
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UItem, Quantity)) {

		Quantity = FMath::Clamp(Quantity, 1, bStackable ? MaxStackSize : 1);
	}
}
#endif

void UItem::SetQuantity(int32 NewQuantity) {

	if (NewQuantity == Quantity) return;
	
	Quantity = FMath::Clamp(NewQuantity, 0, bStackable ? MaxStackSize : 1);

	MarkDirtyForReplication();
}

bool UItem::ShouldShowInInventory() const {

	return true;
}

void UItem::Use(class ASurvivalCharacter* Character) {

	//...
}

void UItem::AddedToInventory(class UInventoryComponent* Inventory) {

	//...
}

void UItem::MarkDirtyForReplication() {

	++RepKey;

	if(OwningInventory != nullptr) {

		++OwningInventory->ReplicatedItemsKey;
	}
}

void UItem::OnRep_Quantity() {
	OnItemModified.Broadcast();
}

bool UItem::IsSupportedForNetworking() const {

	return true;
}

void UItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItem, Quantity);
}

#undef LOCTEXT_NAMESPACE