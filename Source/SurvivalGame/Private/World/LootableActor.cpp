// All rights reserved Dominik Pavlicek


#include "LootableActor.h"

#include "Character/SurvivalCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/InteractionComponent.h"

#define LOCTEXT_NAMESPACE "Lootableactor"

ALootableActor::ALootableActor()
{
	LootContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComp"));
	SetRootComponent(LootContainerMesh);

	InteractionComp = CreateDefaultSubobject<UInteractionComponent>(FName("InteractionComp"));
		InteractionComp->SetInteractionActionText(LOCTEXT("LootActorText", "Loot"));
		InteractionComp->SetInteractionNameText(LOCTEXT("LootActorName", "Chest"));
		InteractionComp->SetupAttachment(GetRootComponent());

	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(FName("InventoryComp"));
		InventoryComp->SetCapacity(25);
		InventoryComp->SetWeightCapacity(150.f);

	LootRolls = FIntPoint(2, 8);

	SetReplicates(true);
}

void ALootableActor::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionComp->OnInteract.AddDynamic(this, &ALootableActor::OnInteract);

	if (HasAuthority() && LootTable != nullptr) {

		TArray<FLootTableRow*> SpawnItems;
		LootTable->GetAllRows("", SpawnItems);

		// Defines how many times we will iterate.
		// Higher number means higher change to spawn a loot.
		int32 Rolls = FMath::RandRange(LootRolls.GetMin(), LootRolls.GetMax());

		for (int32 i = 0; i < Rolls; ++i) {

			// Get random TableRow
			const FLootTableRow* TableRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];

			ensure(TableRow);

			float ProbabilityRoll = FMath::FRandRange(0.f, 1.0);

			// Check if this Row's probability is fulfilled
			while (ProbabilityRoll > TableRow->Probability) {

				TableRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];
				ProbabilityRoll = FMath::FRandRange(0.f, 1.0);
			}

			if (TableRow != nullptr && TableRow->Items.Num()) {

				for (auto& ItemClass : TableRow->Items) {

					if (ItemClass != nullptr) {

						/*UItem* SpawnableItem = Cast<UItem>(ItemClass->GetDefaultObject());*/
						const int32 Quantity = Cast<UItem>(ItemClass->GetDefaultObject())->GetQuantity();
						InventoryComp->TryAddItemFromClass(ItemClass, Quantity);
					}
				}
			}
		}
	}
}

void ALootableActor::OnInteract(class ASurvivalCharacter* Character) {

	if (Character != nullptr) {

		Character->SetLootingSource(InventoryComp);
	}
}

#undef LOCTEXT_NAMESPACE