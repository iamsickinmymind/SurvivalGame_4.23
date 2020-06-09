// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateInteractionWidget(class UInteractionComponent* InteractionComponenet);

	UFUNCTION(BlueprintImplementableEvent)
	void OnUpdateInteractionWidget();

public:

	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (ExposeOnSpawnS))
	class UInteractionComponent* OwningInteractionComp;
};
