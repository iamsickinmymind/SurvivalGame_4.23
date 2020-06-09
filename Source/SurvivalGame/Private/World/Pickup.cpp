// All rights reserved Dominik Pavlicek

#include "Pickup.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#include "Items/Item.h"
#include "Character/SurvivalCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/InteractionComponent.h"

APickup::APickup() {

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComp"));
		PickupMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	SetRootComponent(PickupMesh);

	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(FName("PickupInteractionComp"));
		InteractionComponent->SetInteractionTime(0.5);
		InteractionComponent->SetInteractionDistance(200.f);
		InteractionComponent->SetInteractionNameText(FText::FromString("Pickup"));
		InteractionComponent->SetInteractionActionText(FText::FromString("Take"));
		InteractionComponent->OnInteract.AddDynamic(this, &APickup::OnTakePickup);
		InteractionComponent->SetupAttachment(GetRootComponent());
	
	SetReplicates(true);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() && ItemTemplate != nullptr && bNetStartup) {

		InitializePickup(ItemTemplate->GetClass(), ItemTemplate->GetQuantity());
	}

	/* If pickup was dropped from inventory at runtime, ensure it is aligned with nearest surface normals.
	* in case pickup is dropped to roof with 20 degrees angle, set the pickups rotation to match.*/
	if (!bNetStartup) {

		AlignWithGround();
	}

	if (Item != nullptr) {

		Item->MarkDirtyForReplication();
	}
}

void APickup::InitializePickup(const TSubclassOf<class UItem> ItemClass, const int32 Quantity) {

	if (HasAuthority() && ItemClass && Quantity > 0) {

		Item = NewObject<UItem>(this, ItemClass);
		Item->SetQuantity(Quantity);

		if(Item->ItemPickupMesh) {
			PickupMesh->SetStaticMesh(Item->ItemPickupMesh);
		}

		OnRep_Item();
		
		Item->MarkDirtyForReplication();
	}	
}

void APickup::OnRep_Item() {

	if (Item != nullptr) {

		PickupMesh->SetStaticMesh(Item->ItemPickupMesh);
		InteractionComponent->SetInteractionNameText(Item->ItemDisplayName);

		// Bind to delegate to force widget refresh
		Item->OnItemModified.AddDynamic(this, &APickup::OnItemModified);
	}

	if (InteractionComponent != nullptr) {
		
		InteractionComponent->RefreshWidget();
	}
}

void APickup::OnItemModified() {

	if (InteractionComponent != nullptr) {

		InteractionComponent->RefreshWidget();
	}
}

void APickup::OnTakePickup(class ASurvivalCharacter* Taker) {

	if (Taker == nullptr) {
		UE_LOG(LogTemp, Error, TEXT("Pickup taker is not valid."))
		return;
	}

	// Check if has authority and is not in process of destruction
	if (HasAuthority() &&!IsPendingKillPending() && Item != nullptr) {

		if (UInventoryComponent* TakerInventory = Taker->GetPlayerInventory()) {

			const FItemAddResult AddResult = TakerInventory->TryAddItem(Item);

			// In case we added none or some items to Inventory
			if (AddResult.AmountGiven < Item->GetQuantity()) {

				// Decrement the Items quantity and leave it be
				Item->SetQuantity(Item->GetQuantity() - AddResult.AmountGiven);
			}
			else {

				Destroy();
			}
		}
	}
}

#if WITH_EDITOR
// We are doing logical check to ensure even if the Itemtemplate is modified that we will display the correct Staticmesh
void APickup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {

	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(APickup, ItemTemplate)) {

		if (ItemTemplate != nullptr) {

			PickupMesh->SetStaticMesh(ItemTemplate->ItemPickupMesh);
		}
	}
}

#endif

void APickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APickup, Item);
}

bool APickup::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) {

	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (Item != nullptr && Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey)) {

		bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

