// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "World/ItemSpawnPoint.h"
#include "Items/Item.h"

#include "LootableActor.generated.h"

/*
* 
*/

UCLASS()
class SURVIVALGAME_API ALootableActor : public AActor
{
	GENERATED_BODY()
	
public:	

	ALootableActor();

	UFUNCTION(BlueprintCallable, Category = "Loot")
	FORCEINLINE class UStaticMeshComponent* GetLootContainerMesh() const { return LootContainerMesh; };

	UFUNCTION(BlueprintCallable, Category = "Loot")
	FORCEINLINE class UInteractionComponent* GetInteractionComponent () const { return InteractionComp; };

	UFUNCTION(BlueprintCallable, Category = "Loot")
	FORCEINLINE class UInventoryComponent* GetInventoryComponent() const { return InventoryComp; };

	UFUNCTION(BlueprintCallable, Category = "Loot")
	FORCEINLINE class UDataTable* GetLootTable() const { return LootTable; };

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteract(class ASurvivalCharacter* Character);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* LootContainerMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInteractionComponent* InteractionComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComp = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lootable Settings")
	class UDataTable* LootTable = nullptr;

	// How many times we try to spawn an item.
	// Higher numbers => higher chances for a loot.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lootable Settings")
	FIntPoint LootRolls;
};
