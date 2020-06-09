// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "FoodItem.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API UFoodItem : public UItem
{
	GENERATED_BODY()
	
public:

	UFoodItem();

	virtual void Use(class ASurvivalCharacter* Character) override;

protected:

	/* Amount of heal that will be restored after use.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Food")
	float HealAmount;
};
