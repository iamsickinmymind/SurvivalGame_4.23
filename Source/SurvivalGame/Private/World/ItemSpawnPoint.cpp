// All rights reserved Dominik Pavlicek


#include "ItemSpawnPoint.h"

#include "World/Pickup.h"
#include "Items/Item.h"
#include "TimerManager.h"

AItemSpawnPoint::AItemSpawnPoint() {

	PrimaryActorTick.bCanEverTick = false;
	bNetLoadOnClient = false;

	RespawnRange = FIntPoint(10, 30);
	RespawnRatio = 10.f;
	SpawnRadius = 30.f;

	SetReplicates(true);
}

void AItemSpawnPoint::BeginPlay() {

	Super::BeginPlay();

	if (HasAuthority()) {

		SpawnItem();
	}
}

void AItemSpawnPoint::SpawnItem() {

	if (HasAuthority() && LootTable != nullptr) {

		TArray<FLootTableRow*> LootRows;
		LootTable->GetAllRows("", LootRows);

		const FLootTableRow* LootRow = LootRows[FMath::RandRange(0, LootRows.Num() - 1)];

		ensure(LootRow);

		float ProbabilityRoll = FMath::RandRange(0.f, 1.0);

		while (ProbabilityRoll > LootRow->Probability) {

			LootRow = LootRows[FMath::RandRange(0, LootRows.Num() - 1)];
			ProbabilityRoll = FMath::RandRange(0.f, 1.0);
		}

		// Arrange SpawnItems close to each other
		if (LootRow != nullptr && PickupClass && LootRow->Items.Num()) {

			float Angle = 10.f;

			for (auto& Itr : LootRow->Items) {

				// cos && sin make a circle
				const FVector LocationOffset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * SpawnRadius;

				FActorSpawnParameters SpawnParams;
					SpawnParams.bNoFail = true;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				const int32 ItemQuantity = Itr->GetDefaultObject<UItem>()->GetQuantity();

				FTransform SpawnTransform = GetActorTransform();
					SpawnTransform.AddToTranslation(LocationOffset);

				APickup* NewPickupActor = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams);
					NewPickupActor->InitializePickup(Itr, ItemQuantity);
					NewPickupActor->OnDestroyed.AddDynamic(this, &AItemSpawnPoint::OnItemTaken);

				SpawnedPickups.Add(NewPickupActor);

				// 2PI * R basically
				// First istem is S (center of circle)
				Angle += (PI * 2.f) / LootRow->Items.Num();
			}
		}
	}
}

void AItemSpawnPoint::OnItemTaken(AActor* DestroyedActor) {

	// Maybe I could change AActor to APickup?
	if (HasAuthority()) {

		SpawnedPickups.Remove(DestroyedActor);

		if (SpawnedPickups.Num() <= 0 && bRespawns) {

			GetWorldTimerManager().SetTimer(TimerHandle_RespawnItem, this, &AItemSpawnPoint::SpawnItem, RespawnRatio * 60, false);
		}
	}
}
