// All rights reserved Dominik Pavlicek

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionComponent.generated.h"

class ASurvivalCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginInteract, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStopInteract, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginFocus, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndFocus, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, class ASurvivalCharacter*, Character);

/**
 * Interactable component.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SURVIVALGAME_API UInteractionComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UInteractionComponent();

	FORCEINLINE float GetInteractionDistance() const { return InteractionDistance; };
	FORCEINLINE float GetInteractionTime() const { return InteractionTime; };
	FORCEINLINE void SetInteractionTime(const float NewTime) { InteractionTime = NewTime; };
	FORCEINLINE void SetInteractionDistance(const float NewDistance) { InteractionDistance = NewDistance; };

	void BeginInteract(ASurvivalCharacter* Character);
	void EndInteract(ASurvivalCharacter* Character);
	void BeginFocus(ASurvivalCharacter* Character);
	void EndFocus(ASurvivalCharacter* Character);

	void Interact(ASurvivalCharacter* Character);

	// Returns value in range 0-1 which determines how far in interaction duration we are.
	// For instance, if interactable requires 2 seconds to interact and we held the key for 1s, returns 0.5
	// On server returns first interactor's percentage, on client local interactors percentage
	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetInteractPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionNameText(const FText& NewText);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionActionText(const FText& NewText);

	// Refresh the interaction widget and its sub-widgets.
 	//For instance, when I take 30 ammo from stack of 100, I need to update remaining ammo in storage.
	void RefreshWidget();

protected:

	virtual void Deactivate() override;

	bool GetCanInteract(class ASurvivalCharacter* Character) const;

public:

	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnInteract OnInteract;

protected:

	// The time player must hold the interaction key to interact with this object
	// For instance, 2 seconds to Open door
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionTime;

	// The max distance interaction with this object is allowed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionDistance;

	// The name that will come up whem player look at this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionNameText;

	// The name of the action that will be done, eg. "Open" chest, "Turn On/Off" lights etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionActionText;

	// Whether we allow multiple players interact at the same time with this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bAllowMultipleInteeractors;

	// On server hold information of all active interactors.
	// On client only client.
	UPROPERTY()
	TArray<class ASurvivalCharacter*> Interactors;

private:

	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnBeginInteract OnBeginInteract;

	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnStopInteract OnEndInteract;

	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnBeginFocus OnBeginFocus;

	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnEndFocus OnEndFocus;
};
