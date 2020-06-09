// All rights reserved Dominik Pavlicek

#include "InteractionComponent.h"
#include "SurvivalCharacter.h"
#include "InteractionWidget.h"

#include "Components/PrimitiveComponent.h"

UInteractionComponent::UInteractionComponent() {

	SetComponentTickEnabled(false);

	InteractionTime = 0.f;
	InteractionDistance = 200.f;
	InteractionNameText = FText::FromString("Interactive Object");
	InteractionActionText = FText::FromString("Interact");
	bAllowMultipleInteeractors = true;

	Space = EWidgetSpace::Screen;
	DrawSize = FIntPoint(500, 100);
	bDrawAtDesiredSize = true;

	SetActive(true);
	SetHiddenInGame(true);
}

void UInteractionComponent::BeginInteract(ASurvivalCharacter* Character) {

	if (GetCanInteract(Character)) {

		Interactors.Add(Character);
		OnBeginInteract.Broadcast(Character);
	}
}

void UInteractionComponent::EndInteract(ASurvivalCharacter* Character) {

	Interactors.RemoveSingle(Character);
	OnEndInteract.Broadcast(Character);
}

void UInteractionComponent::BeginFocus(ASurvivalCharacter* Character) {

	if (!IsActive() || !GetOwner() || !Character) return;

	OnBeginFocus.Broadcast(Character);

	// locally find all primitive comps of this comp and set Custom depth to true
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (Character->GetController()->IsLocalController()) {

			SetHiddenInGame(false);

			for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass()))
			{
				if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
				{
					Prim->SetRenderCustomDepth(true);
				}
			}
		}
	}

	RefreshWidget();
}

void UInteractionComponent::EndFocus(ASurvivalCharacter* Character) {

	OnEndFocus.Broadcast(Character);

	if (GetNetMode() != NM_DedicatedServer)
	{
		SetHiddenInGame(true);

		for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass()))
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
			{
				Prim->SetRenderCustomDepth(false);
			}
		}
	}
}

void UInteractionComponent::Interact(ASurvivalCharacter* Character) {

	if (GetCanInteract(Character))
	{
		OnInteract.Broadcast(Character);
	}
}

float UInteractionComponent::GetInteractPercentage() const {

	// Get first interactor (on Client always Client, on Server first Interactor)
	if (Interactors.IsValidIndex(0)) {

		if (ASurvivalCharacter* Interactor = Interactors[0]) {

			// Interactor is ASurvivalCharacter and is interacting
			if (Interactor && Interactor->GetIsInteracting()) {

				// 1 - absolute value of remaining time / full time
				return 1.f - FMath::Abs(Interactor->GetRemainingInteractionTime() / InteractionTime);
			}
		}
	}

	return 0.f;
}

void UInteractionComponent::SetInteractionNameText(const FText& NewText) {

	InteractionNameText = NewText;
	RefreshWidget();
}

void UInteractionComponent::SetInteractionActionText(const FText& NewText) {

	InteractionActionText = NewText;
	RefreshWidget();
}

void UInteractionComponent::RefreshWidget() {

	if ( UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject())) {

		InteractionWidget->UpdateInteractionWidget(this);
	}

// 	if (!bHiddenInGame && GetOwner()->GetNetMode() != NM_DedicatedServer) {
// 
// 		if (UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject())) {
// 
// 			InteractionWidget->UpdateInteractionWidget(this);
// 		}
// 	}
}

void UInteractionComponent::Deactivate() {

	Super::Deactivate();

	for (auto const Interactor : Interactors) {

		EndFocus(Interactor);
		EndInteract(Interactor);
	}

// 	for (int32 i = Interactors.Num() - 1; i > 0; --i) {
// 
// 		if (ASurvivalCharacter* Interactor = Interactors[i]) {
// 
// 			EndFocus(Interactor);
// 			EndInteract(Interactor);
// 		}
// 	}

	Interactors.Empty();
}

bool UInteractionComponent::GetCanInteract(class ASurvivalCharacter* Character) const {

	const bool bPlayerAlreadyInteracting = !bAllowMultipleInteeractors && Interactors.Num() >= 1;
	return !bPlayerAlreadyInteracting && IsActive() && GetOwner() != nullptr && Character != nullptr;
}
