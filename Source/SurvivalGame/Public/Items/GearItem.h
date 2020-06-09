// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "GearItem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class SURVIVALGAME_API UGearItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UGearItem();

	virtual bool Equip(class ASurvivalCharacter* Character) override;
	virtual bool UnEquip(class ASurvivalCharacter* Character) override;

	FORCEINLINE class USkeletalMesh* GetGearMesh() const { return Mesh; };

public:

	//...
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class USkeletalMesh* Mesh = nullptr;

	//...
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class UMaterialInstance* MaterialInstance = nullptr;

	class UMaterialInstanceDynamic* DynamicMaterialInstance = nullptr;

	//...
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	float DamageDefenseMultiplier;
};
