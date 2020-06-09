// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "ThrowableWeapon.h"
#include "ThrowableItem.generated.h"

class UAnimMontage;
class AThrowableWeapon;

/**
 * Throwable item framework.
 */
UCLASS(Blueprintable)
class SURVIVALGAME_API UThrowableItem : public UEquippableItem
{
	GENERATED_BODY()

	friend class ASurvivalCharacter;

public:

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	UAnimMontage* ThrowableTossAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AThrowableWeapon> ThrowableWeaponClass;
};
