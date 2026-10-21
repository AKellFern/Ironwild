// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventorySlot.h"
#include "ContainerType.h"
#include "ItemContainerComponent.generated.h"


// Fired after any change to a container's slots. Carries no data on purpose: listeners (the UI)
// simply re-read Slots.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContainerChangedDelegate);

/**
 * A list of item slots that can be added to any actor: player backpack, chest, hotbar.
 * This component owns the inventory DATA. Widgets only display it and listen to OnContainerChanged.
 * All changes go through the functions below so the change event always fires.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SURVIVALGAME_API UItemContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UItemContainerComponent();

	// The slots themselves, sized from Capacity in BeginPlay. Transient so stale editor data is never
	// saved. Read-only to Blueprints: change it only through the functions below.
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Inventory")
		TArray<FInventorySlot> Slots;

	// Starting number of slots, set per container in Details. Applied in BeginPlay via SetCapacity.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
		int32 Capacity = 20;

	// Index of the first empty slot, or INDEX_NONE if the container is full.
	int32 FindEmptySlot() const;

	// Index of the first slot holding the same item (by ItemId) that still has room, or INDEX_NONE.
	int32 FindStackableSlot(const FInventorySlot& Slot) const;

	// What this container is for. Must not be None, or AddItem refuses everything.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")	
		EContainerType ContainerType = EContainerType::None;

	// Largest size SetCapacity will accept (room for future backpack upgrades).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
		int32 MaxCapacity = 40;

	// Broadcast after any change to Slots. UContainerWidgetBase subscribes to it to redraw.
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
		FOnContainerChangedDelegate OnContainerChanged;

	// The only way to resize the container. Refuses <= 0, > MaxCapacity, or shrinking below the
	// current slot count (so items are never deleted). Returns true if the new size was applied.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
		bool SetCapacity(int32 NewCapacity);

	// Returns the quantity that could NOT be added (0 = everything fit; == Slot.Quantity = nothing fit).
	// Existing stacks of the same item are topped up first, then new stacks are created.
	// bPreserveInstanceId = false (default): every new stack gets a fresh ItemInstanceId; the item is
	//   treated as new to this container (pickups, crafting, loot).
	// bPreserveInstanceId = true: the FIRST new stack keeps Slot's ItemInstanceId, if it is valid and not
	//   already used in this container (moves between your own containers). Any extra stacks still get
	//   fresh IDs, and quantity merged into an existing stack takes that stack's ID.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
		int32 AddItem(const FInventorySlot& Slot, bool bPreserveInstanceId = false);

	// Empties the slot at SlotIndex completely. Returns false for an invalid index or an already-empty slot.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
		bool RemoveItem(int32 SlotIndex);

	// Index of the slot holding the item with this ItemInstanceId, or INDEX_NONE.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
		int32 FindSlotByGuid(const FGuid& ItemInstanceId) const;

protected:
	// Called when the game starts. Sizes Slots from Capacity.
	virtual void BeginPlay() override;
};
