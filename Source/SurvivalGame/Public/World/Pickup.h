// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class SURVIVALGAME_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	

	APickup();

	// Takes the item class and creates a Pickup from it.
	// Called in  BeginPlay and when item is dropped from Inventory
	UFUNCTION()
	void InitializePickup(const TSubclassOf<class UItem> ItemClass, const int32 Quantity);

	// Aligns Pickup world rotation to nearest normal rotation
	UFUNCTION(BlueprintImplementableEvent)
	void AlignWithGround();

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Item();

	// When item is modified (picked up, partially picked up...) we bind this function to OnItemModifie and refresh the UI
	UFUNCTION()
	void OnItemModified();

	// When item is picked up.
	UFUNCTION()
	void OnTakePickup(class ASurvivalCharacter* Taker);

#if WITH_EDITOR
	// IN EDITOR ONLY
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

public:

	// Template item used to initialize the pickup
	// Instanced because of Item class is EditInlineNew
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	class UItem* ItemTemplate;

protected:

	// The item that will be added to Inventory when the pickup is taken
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, ReplicatedUsing=OnRep_Item)
	class UItem* Item;

	// Static Mesh which manifests the item.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pickup")
	class UStaticMeshComponent* PickupMesh = nullptr;

	// UI component used to display information about Item
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pickup")
	class UInteractionComponent* InteractionComponent = nullptr;
};
