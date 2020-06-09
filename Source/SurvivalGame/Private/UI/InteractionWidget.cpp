// All rights reserved Dominik Pavlicek

#include "InteractionWidget.h"
#include "Components/InteractionComponent.h"

void UInteractionWidget::UpdateInteractionWidget(class UInteractionComponent* InteractionComponenet) {

	OwningInteractionComp = InteractionComponenet;
	OnUpdateInteractionWidget();
}
