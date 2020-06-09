// All rights reserved Dominik Pavlicek

#include "InventoryComponent.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Item.h"

#define LOCTEXT_NAMESPACE "Inventory"

UInventoryComponent::UInventoryComponent()
{
	OnItemAdded.AddDynamic(this, &UInventoryComponent::ItemAdded);
	OnItemRemoved.AddDynamic(this, &UInventoryComponent::ItemRemoved);

	SetIsReplicated(true);

	Capacity = 10;
	WeightCapacity = 30.f;
}

FItemAddResult UInventoryComponent::TryAddItem(class UItem* Item) {

	return TryAddItem_Internal(Item);
}

FItemAddResult UInventoryComponent::TryAddItemFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity) {

	if (Quantity == 0) {

		return FItemAddResult::AddedNone(Quantity, LOCTEXT("Error", "Trying to return 0."));
	}

	UItem* Item = NewObject<UItem>(GetOwner(), ItemClass);

	Item->SetQuantity(Quantity);
	return TryAddItem_Internal(Item);
}

FItemAddResult UInventoryComponent::TryAddItem_Internal(class UItem* Item) {

	if (GetOwner() && GetOwner()->HasAuthority()) {

		const int32 AddAmount = Item->GetQuantity();
		
		// In case we are trying to add this amount of items but it would fill inventory above its limit
		if (Items.Num() + 1 > GetCapacity()) {

			return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryCapacityFullText", "Cannot add item to Inventory. Inventory is full."));
		}
		
		// In case the Item weight exceeds maximum Weight Capacity
		if (!(FMath::IsNearlyZero(Item->Weight))) {

			if (GetCurrentWeight() + Item->Weight > GetWeightCapacity()) {

				return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryTooMuchWeightText", "Cannot add item to Inventory. Carrying too much weight."));
			}
		}

		// In case Item is stackable add if we do not already have one
		if (Item->bStackable) {

			// Safety check that controls if Quantity did not exceeded MaxStackSize
			ensure(Item->GetQuantity() <= Item->MaxStackSize); 
			
			// Check for existing Item in Inventory
			if(UItem* ExistingItem = FindItem(Item)) {

				if (ExistingItem->GetQuantity() < ExistingItem->MaxStackSize) {

					// Calculate how much of the Item can be added
					/*const*/ int32 CapacityMaxAddAmount = ExistingItem->MaxStackSize - ExistingItem->GetQuantity();
					int32 ActualAddAmount = FMath::Min(CapacityMaxAddAmount, AddAmount);
					CapacityMaxAddAmount = ActualAddAmount;

					FText ErrorText = LOCTEXT("InventoryErrorText", "Could not add all of the items to you inventory.");

					if (!(FMath::IsNearlyZero(Item->Weight))) {

						// Calculate the maximum amount of the item we could add due to the weight
						const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);
						ActualAddAmount = FMath::Min(CapacityMaxAddAmount, WeightMaxAddAmount);

						// In case we can add at least one item
						if (ActualAddAmount < AddAmount) {

							ErrorText = FText::Format(LOCTEXT("InventoryTooMuchWeightText", "Could not add entire stock of {ItemName} to Inventory."), Item->ItemDisplayName);
						}
					}
					else if (ActualAddAmount < AddAmount) {

						// If Weight of item is NealyZero but we cannot add it it is because of Capacity
						ErrorText = FText::Format(LOCTEXT("InventoryCapacityFullText", "Could not add entire stock of {ItemName} to Inventory. Inventory is full."), Item->ItemDisplayName);
					}

					// This should not happen.
					if (AddAmount <= 0) {

						return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryErrorText", "Could not add item to inventory."));
					}

					ExistingItem->SetQuantity(ExistingItem->GetQuantity() + ActualAddAmount);

					// Safety check if we are not trying to add more than allowed
					ensure(ExistingItem->GetQuantity() <= ExistingItem->MaxStackSize);

					if (ActualAddAmount < AddAmount) {

						return FItemAddResult::AddedSome(AddAmount, ActualAddAmount, ErrorText);
					}
					else {

						return FItemAddResult::AddedAll(AddAmount);
					}
				}
				else {

					return FItemAddResult::AddedNone(AddAmount, FText::Format(LOCTEXT("InventoryStackFullText", "Could not add {ItemName} to Inventory. Inventory stack of this item is already full."), Item->ItemDisplayName));
				}
			}
			else {

				// We don't have any of this item we can add full stack

				AddItem(Item);

				return FItemAddResult::AddedAll(AddAmount);
			}
		}
		else {
			// Safety check we are not trying to add more than one non-stackable item.
			ensure(Item->GetQuantity() == 1);

			AddItem(Item);
			return FItemAddResult::AddedAll(AddAmount);
		}
	}
	else {

		// AddItem should never be called from Client side!
		check(false);
		return FItemAddResult::AddedNone(-1, LOCTEXT("ErrorMessage", ""));
	}
}

int32 UInventoryComponent::ConsumeItem(class UItem* Item) {

	if (Item != nullptr) {

		ConsumeItem(Item, Item->GetQuantity());
	}

	return 0;
}

int32 UInventoryComponent::ConsumeItem(class UItem* Item, const int32 Quantity) {

	if (GetOwner() && GetOwner()->HasAuthority()) {

		if (Item != nullptr) {

			// Logical check to consume at max the only all remaining if fewer than requested consumption amount
			const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

			// For sure perform ensure operation to avoid negative quantity index
			ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

			Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

			if (Item->GetQuantity() <= 0) {

				RemoveItem(Item);
			}
			else {

				// If still any items left, refresh clients Inventory
				ClientRefreshInventory();
			}

			return RemoveQuantity;
		}

		return 0;
	}

	return 0;
}

bool UInventoryComponent::RemoveItem(class UItem* Item) {

	if(GetOwner() && GetOwner()->HasAuthority()) {

		if (Item != nullptr) {

			Items.RemoveSingle(Item);
			OnItemRemoved.Broadcast(Item);

			OnRep_Items();

			ReplicatedItemsKey++;

			return true;
		}
	}

	return false;
}

bool UInventoryComponent::HasItem(TSubclassOf<class UItem> ItemClass, const int32 Quantity /*= 1*/) const {

	UItem* ItemToFind = FindItemByClass(ItemClass);

	if (ItemToFind != nullptr && ItemToFind->GetQuantity() >= Quantity) {

		return true;
	}

	return false;
}

UItem* UInventoryComponent::FindItem(class UItem* Item) const {

	if (Item != nullptr) {

		for (auto& ItrItem : Items) {

			if (ItrItem != nullptr && ItrItem->GetClass() == Item->GetClass()) {

				return ItrItem;
			}
		}

		return nullptr; 
	}

	return nullptr;
}

UItem* UInventoryComponent::FindItemByClass(TSubclassOf<UItem> ItemClass) const {

	for (auto& ItrItem : Items) {

		if (ItrItem != nullptr && ItrItem->GetClass() == ItemClass) {

			return ItrItem;
		}
	}

	return nullptr;
}

TArray<UItem*> UInventoryComponent::FindItemsByClass(TSubclassOf<UItem> ItemClass) const {

	TArray<UItem*> ItrItems;

	for (auto& ItrItem : Items) {

		if (ItrItem != nullptr && ItrItem->GetClass() == ItemClass) {

			ItrItems.Add(ItrItem);
		}
	}

	return ItrItems;
}

float UInventoryComponent::GetCurrentWeight() const {

	float ItemWeight = 0.f;

	for (auto& Item : Items) {

		if (Item != nullptr) {

			ItemWeight += Item->GetStackWeight();
		}
	}

	return ItemWeight;
}

void UInventoryComponent::SetCapacity(const int32 NewCapacity) {

	Capacity = NewCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity) {
	
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast();
}

UItem* UInventoryComponent::AddItem(class UItem* Item) {

	// Recreate item and parent it to this intenvtory
	if (GetOwner() != nullptr && GetOwner()->HasAuthority()) {

		UItem* NewItem = NewObject<UItem>(GetOwner(), Item->GetClass());
			NewItem->Quantity = Item->Quantity;
			NewItem->OwningInventory = this;
			NewItem->AddedToInventory(this);
			NewItem->World = GetWorld();
		Items.Add(NewItem);
		NewItem->MarkDirtyForReplication();
		OnItemAdded.Broadcast(NewItem);

		OnRep_Items();

		return NewItem;
	}

	return nullptr;
}

void UInventoryComponent::OnRep_Items() {

	OnInventoryUpdated.Broadcast();

	if (Items.Num()) {

		for (auto& Item : Items) {

			Item->World = GetWorld();
		}
	}
}

void UInventoryComponent::ClientRefreshInventory_Implementation() {

	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Items);
}

bool UInventoryComponent::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) {

	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// Check if the array need to replicate
	if(Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey)) {

		for (auto Item : Items) {

			if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey)) {

				bWroteSomething = Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
			}
		}
	}

	return bWroteSomething;
}

void UInventoryComponent::ItemAdded(class UItem* Item)
{
	FString RoleString = GetOwner()->HasAuthority() ? "server" : "client";
	UE_LOG(LogTemp, Warning, TEXT("Item added: %s on %s"), *GetNameSafe(Item), *RoleString);
}

void UInventoryComponent::ItemRemoved(class UItem* Item)
{
	FString RoleString = GetOwner()->HasAuthority() ? "server" : "client";
	UE_LOG(LogTemp, Warning, TEXT("Item Removed: %s on %s"), *GetNameSafe(Item), *RoleString);
}

#undef LOCTEXT_NAMESPACE