// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item.generated.h"

class UMaterialInstanceDynamic;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemModified);

UENUM(BlueprintType)
enum class EItemRarity : uint8 {

	EIR_Common		UMETA(DisplayName = "Common"),
	EIR_Uncommon	UMETA(DisplayName = "Uncommon"),
	EIR_Rare		UMETA(DisplayName = "Rare"),
	EIR_VeryRare	UMETA(DisplayName = "Very Rare"),
	EIR_Legendary	UMETA(DisplayName = "Legendary"),
	EIR_Default		UMETA(DisplayName = "Default")
};

/**
 * Basic Item.
 Item is stored in Inventory.
*/
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SURVIVALGAME_API UItem : public UObject
{
	GENERATED_BODY()

	friend class UInventoryComponent;
	friend class APickup;

public:

	UItem();

	UFUNCTION(BlueprintImplementableEvent)
	void OnUse(class ASurvivalCharacter* Character);

	/* Returns total weight of this item (multiplied by current Quantity if stackable) in inventory.*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetStackWeight() const { return Quantity * Weight; };

	/* Returns total Quantity of this item in inventory.*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE int32 GetQuantity() const { return Quantity; };

	/* Returns item thumbnail.*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE class UTexture2D* GetThumbnail() const { return ItemThumbnail; };

	/* Returns item Display Name.*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE FText GetItemName() const { return ItemDisplayName; };

	/* Returns item Display Description.*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE FText GetItemDescription() const { return ItemDescription; };

	/* Sets new Quantity.*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 NewQuantity);

	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool ShouldShowInInventory() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE UWorld* GetItemWorld() const { return World; };

	virtual void Use(class ASurvivalCharacter* Character);
	virtual void AddedToInventory(class UInventoryComponent* Inventory);

protected:

#if  WITH_EDITOR
	/// EDITOR ONLY
	//  In here we are clamping Quantity to be always at maximum of MaxStackSize
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;

	/* Must call this function after performing changes to Item.*/
	void MarkDirtyForReplication();

	UFUNCTION()
	void OnRep_Quantity();

public:

	/* Used to efficiently replicate inventory items.*/
	UPROPERTY()
	int32 RepKey;

	/* Item thumbnail.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UTexture2D* ItemThumbnail = nullptr;

	/* Static Mesh that manifests the item.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UStaticMesh* ItemPickupMesh = nullptr;

	/* Inventory which owns this Item.*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	class UInventoryComponent* OwningInventory = nullptr;

	/* The tooltip in the inventory for this item.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	TSubclassOf<class UItemTooltip> ItemTooltip = nullptr;

	/* Display Name of this item.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText ItemDisplayName;

	/* Short Item description.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
	FText ItemDescription;

	/* Name of action to use item, eg. "use"*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText ItemActionText;

	/* Defines the value of the item.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	EItemRarity ItemRariry;

	/* Defines weight of the single item in Kg.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (UIMin = 0, ClampMin = 0))
	float Weight;

	/* Determines whether item can be stacked or not, eg Ammo can be stacked.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	bool bStackable;

	/* Max stack size of an item, eg. max 30 Ammo.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (UIMin = 2, ClampMin = 2, EditCondition = bStackable))
	int32 MaxStackSize;

	/* Current amount of owned item pieces.*/
	UPROPERTY(ReplicatedUsing = OnRep_Quantity, EditAnywhere, Category = "Item", meta = (UIMin = 1, ClampMin = 1, EditCondition = bStackable))
	int32 Quantity;

	/* Called when item is modified.*/
	UPROPERTY(BlueprintAssignable)
	FOnItemModified OnItemModified;

	UPROPERTY(Transient)
	UWorld* World = nullptr;
};