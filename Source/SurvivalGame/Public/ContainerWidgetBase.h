// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemContainerComponent.h"
#include "ContainerWidgetBase.generated.h"

/**
 * Base class for any widget that shows one item container (backpack grid, chest, hotbar).
 * It handles the plumbing: holding the container, listening for its changes, and cleaning up.
 * Blueprint children only implement RefreshContainer to draw the slots.
 */
UCLASS()
class SURVIVALGAME_API UContainerWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	// Redraw from Container->Slots. Implemented in Blueprint (no C++ body). Called once by
	// InitializeContainer, then every time the container broadcasts OnContainerChanged.
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
		void RefreshContainer();

	// Points this widget at a container: stores it, subscribes to its changes, and draws once.
	// This is the only way to set Container (read-only to Blueprints), so the subscription can't be skipped.
	// Safe to call again: switching to another container unsubscribes from the old one first, and
	// passing the same container again never subscribes twice.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
		void InitializeContainer(UItemContainerComponent* NewContainment);
	
	// The container being shown. Not owned: the component owns the data, this widget only reads it.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
		TObjectPtr<UItemContainerComponent> Container;

protected:

	// Unsubscribes from the container when the widget is removed (if it still has a live one).
	virtual void NativeDestruct() override;
	
	
};
