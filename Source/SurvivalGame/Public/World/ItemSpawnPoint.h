// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"

#include "Engine/DataTable.h"

#include "ItemSpawnPoint.generated.h"

USTRUCT(BlueprintType)
struct FLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	// Item definitions to spawn.
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<TSubclassOf<class UItem>> Items;

	// Percentage chance of spawning this item
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (UIMin = 0.1, ClampMin = 0.1, UIMax = 1.0, ClampMax = 1.0))
	float Probability = 1.f;
};

/**
 * Item Spawn Point is an Actor responsible for generic item spawning.
 * If placed on map, randomly generates item(s) on its position, if possible.
 */
UCLASS()
class SURVIVALGAME_API AItemSpawnPoint : public ATargetPoint
{
	GENERATED_BODY()

public:

	AItemSpawnPoint();

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void SpawnItem();

	UFUNCTION()
	void OnItemTaken(AActor* DestroyedActor);

protected:

	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	class UDataTable* LootTable = nullptr;

	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	TSubclassOf<class APickup> PickupClass;

	/** Circle radius. Defines how far away from each other should be items spawned.*/
	UPROPERTY(EditAnywhere, Category = "SpawnSettings", meta = (UIMin = 0, ClampMin = 0))
	float SpawnRadius;

	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	bool bRespawns;

	/** How often the does Spawner respawn items.
	* Time is in minutes.
	*/
	UPROPERTY(EditAnywhere, Category = "SpawnSettings", meta = (EditCondition = bRespawns))
	float RespawnRatio;

	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	FIntPoint RespawnRange;

	UPROPERTY()
	TArray<AActor*> SpawnedPickups;

private:

	FTimerHandle TimerHandle_RespawnItem;
};
