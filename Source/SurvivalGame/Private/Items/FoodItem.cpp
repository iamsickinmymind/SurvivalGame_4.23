// All rights reserved Dominik Pavlicek

#include "FoodItem.h"
#include "SurvivalCharacter.h"
#include "SurvivalPlayerController.h"
#include "InventoryComponent.h"

#define LOCTEXT_NAMESPACE "FoodItem"

UFoodItem::UFoodItem() {

	HealAmount = 10.f;
	ItemActionText = LOCTEXT("ItemUseActionText", "Consume");
}

void UFoodItem::Use(class ASurvivalCharacter* Character) {

	if (Character != nullptr) {

		const float ActualHealthHealed = Character->ModifyHealth(HealAmount);
		const bool bUseFood = !FMath::IsNearlyZero(ActualHealthHealed);

		if (Character) { /*(!Character->HasAuthority()) {*/

			if (ASurvivalPlayerController* PlayerCon = Cast<ASurvivalPlayerController>(Character->GetController())) {

				if (bUseFood) {

					UE_LOG(LogTemp, Warning, TEXT("Did use offd"))
					PlayerCon->ShowNotificationMessage(FText::Format(LOCTEXT("AteFoodText", "Ate {FoodName}, healed {HealedAmonut} health."), ItemDisplayName, HealAmount));
				}
				if (!bUseFood) {

					UE_LOG(LogTemp, Warning, TEXT("Did not use offd"))
					PlayerCon->ShowNotificationMessage(FText::Format(LOCTEXT("FullHealthText", "No need to eat {FoodName}, health is full. "), ItemDisplayName));
				}
			}
		}

		if (bUseFood) {

			if (UInventoryComponent* InventoryComp = Character->GetPlayerInventory()) {

				InventoryComp->ConsumeItem(this, 1);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE