// All rights reserved Dominik Pavlicek


#include "ThrowableWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AThrowableWeapon::AThrowableWeapon()
{
	ThrowableMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("ThrowableComp"));
	SetRootComponent(ThrowableMesh);

	ThrowableMovement = CreateDefaultSubobject<UProjectileMovementComponent>(FName("ThrowableMovementComp"));
	ThrowableMovement->InitialSpeed = 1000.f;

	SetReplicates(true);
	SetReplicateMovement(true);
}