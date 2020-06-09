// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThrowableWeapon.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class SURVIVALGAME_API AThrowableWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AThrowableWeapon();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable Item")
	UStaticMeshComponent* ThrowableMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable Item")
	UProjectileMovementComponent* ThrowableMovement = nullptr;

};