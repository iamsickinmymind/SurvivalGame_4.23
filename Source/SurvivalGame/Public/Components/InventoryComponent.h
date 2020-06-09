// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

// Called when the inventory is changed and the UI needs to be refreshed.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

/**Called on server when an item is added to this inventory*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, class UItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, class UItem*, Item);

UENUM(BlueprintType)
enum class EItemAddResult : uint8 {

	EAR_NoItemsAdded	UMETA(DisplayName = "No Items Added"),
	EAR_SomeItemsAdded	UMETA(DisplayName = "Some Items Added"),
	EAR_AllItemsAdded	UMETA(DisplayName = "All Items Added"),
	EAR_Default			UMETA (DisplayName = "Default")
};

//Represents the result of adding an item to the inventory.
USTRUCT(BlueprintType)
struct FItemAddResult
{

	GENERATED_BODY()

public:

	// Constructors
	FItemAddResult() {};
	FItemAddResult(int32 InItemQuantity) : AmountToGive(InItemQuantity), AmountGiven(0) {};
	FItemAddResult(int32 InItemQuantity, int32 InQuantityAdded) : AmountToGive(InItemQuantity), AmountGiven(InQuantityAdded) {};

	//The amount of the item that we tried to add
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item Add Result")
	int32 AmountToGive = 0;

	//The amount of the item that was actually added in the end. Maybe we tried adding 10 items, but only 8 could be added because of capacity/weight
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item Add Result")
	int32 AmountGiven = 0;

	//The result
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item Add Result")
	EItemAddResult Result = EItemAddResult::EAR_NoItemsAdded;

	//If something went wrong, like we did not have enough capacity or carrying too much weight this contains the reason why
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item Add Result")
	FText ErrorText = FText::GetEmpty();

	//Helpers
	static FItemAddResult AddedNone(const int32 InItemQuantity, const FText& ErrorText)
	{
		FItemAddResult AddedNoneResult(InItemQuantity);
		AddedNoneResult.Result = EItemAddResult::EAR_NoItemsAdded;
		AddedNoneResult.ErrorText = ErrorText;

		return AddedNoneResult;
	}

	static FItemAddResult AddedSome(const int32 InItemQuantity, const int32 ActualAmountGiven, const FText& ErrorText)
	{
		FItemAddResult AddedSomeResult(InItemQuantity, ActualAmountGiven);

		AddedSomeResult.Result = EItemAddResult::EAR_SomeItemsAdded;
		AddedSomeResult.ErrorText = ErrorText;

		return AddedSomeResult;
	}

	static FItemAddResult AddedAll(const int32 InItemQuantity)
	{
		FItemAddResult AddAllResult(InItemQuantity, InItemQuantity);

		AddAllResult.Result = EItemAddResult::EAR_AllItemsAdded;

		return AddAllResult;
	}
};

/**
 * INVENTORY COMPONENT
 * Inventory stores Items.
 * Replicated.
 * Can be used as Player inventory as well as lootable chest inventory.
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SURVIVALGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UItem;

public:	

	UInventoryComponent();

	/** Adds an Item to Inventory
	@param ErrorText	- Text to display if fce cannot add Item to Inventory
	@return				- Amount of the Item that was added to the Inventory
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItem(class UItem* Item);

	/** Adds an Item to the Inventory
	* Adds an Item using Item class instead of direct Item reference. Use in case you don't have any Item to add.
	@param ErrorText	- Text to display if fce cannot add Item to Inventory
	@return				- Amount of the Item that was added to the Inventory
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItemFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity);

	/** Takes an Item instance from the Inventory
	* If Item quantity reaches zero, Item will be removed completely.
	* Can be specified how much of an Item should be consumed. By default all instances are being consumed.
	*/
	int32 ConsumeItem(class UItem* Item);
	int32 ConsumeItem(class UItem* Item, const int32 Quantity);

	/** Removes Item from the Inventory
	* Do not call Items.RemoveSingle directly.
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(class UItem* Item);

	/** Returns whether the Inventory contains a given amount of an item*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(TSubclassOf<class UItem> ItemClass, const int32 Quantity = 1) const;

	/** Returns first item with the same class from Inventory
	* Usage: Looking for a particular Weapon, eg. Assault Rifle.
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UItem* FindItem(class UItem* Item) const;

	/** Returns first item with given class from Inventory
	* Usage: Looking for a Weapon, but not a particular one.
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UItem* FindItemByClass(TSubclassOf<UItem> ItemClass) const;

	/** Returns all items of given class from Inventory
	* Usage: Looking for all Food items to check whether we can add some more into the inventory.
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<UItem*> FindItemsByClass(TSubclassOf<UItem> ItemClass) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE TArray<class UItem*> GetItems() const { return Items; };

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE int32 GetCapacity() const { return Capacity; };

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE float GetWeightCapacity() const { return WeightCapacity; };

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCurrentWeight() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetCapacity(const int32 NewCapacity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetWeightCapacity(const float NewWeightCapacity);

	UFUNCTION(Client, Reliable)
	void ClientRefreshInventory();

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

private:

	UFUNCTION()
	void OnRep_Items();

	UFUNCTION()
	void ItemAdded(class UItem* Item);

	UFUNCTION()
	void ItemRemoved(class UItem* Item);

	// Internal, non-BP exposed
	// Do not call directly, use TryAddItem, or TryAddItemFromClass instead.
	FItemAddResult TryAddItem_Internal(class UItem* Item);

	// Internal, non-BP exposed
	// Do not call Items.Add directly, use this fce instead
	UItem* AddItem(class UItem* Item);

public:

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemRemoved OnItemRemoved;

protected:

	/** List of items in Inventory.*/
	UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, Category = "Inventory")
	TArray<class UItem*> Items;

	/** Maximum number of item the inventory can contain.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory", meta = (UIMin = 0, ClampMin = 0))
	int32 Capacity;

	/** Maximum weight of all items combined that can be stored in inventory.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory")
	float WeightCapacity;

private:

	UPROPERTY()
	int32 ReplicatedItemsKey;
};