// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemContainerComponent.h"
#include "ItemDefinition.h"	




// Sets default values for this component's properties
UItemContainerComponent::UItemContainerComponent()
{
	// Containers only change when something calls into them, so they never tick.
	PrimaryComponentTick.bCanEverTick = false;

	

	// ...
}

// Linear scan. Containers hold tens of slots, so this is cheap.
int32 UItemContainerComponent::FindEmptySlot() const
{

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].IsEmpty())
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// Conditions are checked left to right: the slot is occupied (which makes the -> calls after it
// safe), it holds the same ItemId, and it is still below its stack limit.
int32 UItemContainerComponent::FindStackableSlot(const FInventorySlot& Slot) const
{
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (!Slots[i].IsEmpty() && Slots[i].ItemDefinition->ItemId == Slot.ItemDefinition->ItemId && Slots[i].Quantity < Slots[i].ItemDefinition->MaxStackSize)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// Guards first, then apply. The shrink check compares against Slots.Num() rather than Capacity,
// so the BeginPlay call (where Capacity already equals the requested value) is not refused.
bool UItemContainerComponent::SetCapacity(int32 NewCapacity)
{
	if (NewCapacity <= 0)
	{
		return false;
	}

	if (NewCapacity > MaxCapacity)
	{
		return false;
	}

	if (NewCapacity < Slots.Num())
	{
		return false;
	}

	// SetNum adds empty slots at the end; existing items keep their indices.
	Capacity = NewCapacity;
	Slots.SetNum(Capacity);
	OnContainerChanged.Broadcast();
	return true;
}

// Full contract is in the header. Order: validate, top up existing stacks, open new stacks,
// broadcast once, return what didn't fit.
int32 UItemContainerComponent::AddItem(const FInventorySlot& Slot, bool bPreserveInstanceId)
{
	// Validate the container. A container with no type was never set up.
	if (ContainerType == EContainerType::None)
	{
		return Slot.Quantity;
	}

	// Validate the incoming item.
	if (Slot.ItemDefinition == nullptr)
	{
		return Slot.Quantity;
	}

	// An unnumbered definition (ItemId 0) would stack with every other unnumbered item.
	if (Slot.ItemDefinition->ItemId == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: AddItem refused %s because its ItemId is 0 (not assigned)."),
			*GetNameSafe(GetOwner()), *GetNameSafe(Slot.ItemDefinition));
		return Slot.Quantity;
	}

	// Nothing (or a nonsense negative amount) to add, so nothing is left over.
	if (Slot.Quantity <= 0)
	{
		return 0;
	}

	// A stack limit of 0 or less is a broken asset: no stack could ever hold anything.
	if (Slot.ItemDefinition->MaxStackSize <= 0)
	{
		return Slot.Quantity;
	}

	// Track how many items still need to be added.
	int32 RemainingQuantity = Slot.Quantity;

	// Track whether the inventory was modified.
	bool bInventoryChanged = false;

	// STEP 1: Fill existing compatible stacks.
	while (RemainingQuantity > 0)
	{
		int32 StackableSlot = FindStackableSlot(Slot);

		if (StackableSlot == INDEX_NONE)
		{
			break;
		}

		FInventorySlot& ExistingSlot = Slots[StackableSlot];

		// Room left in this stack, clamped so a stack already over its limit can't go negative.
		int32 MaxStack = ExistingSlot.ItemDefinition->MaxStackSize;
		int32 AvailableSpace = FMath::Max(0, MaxStack - ExistingSlot.Quantity);

		int32 AmountToAdd = FMath::Min(
			RemainingQuantity,
			AvailableSpace
		);

		ExistingSlot.Quantity += AmountToAdd;
		RemainingQuantity -= AmountToAdd;

		bInventoryChanged = true;
	}

	// Only one stack can carry the incoming ID, and only if it is real and not already in this
	// container (two slots sharing an ID would break FindSlotByGuid).
	bool bCanPreserveId = bPreserveInstanceId
		&& Slot.ItemInstanceId.IsValid()
		&& FindSlotByGuid(Slot.ItemInstanceId) == INDEX_NONE;

	// STEP 2: Create new stacks for leftover items.
	while (RemainingQuantity > 0)
	{
		int32 EmptySlot = FindEmptySlot();

		if (EmptySlot == INDEX_NONE)
		{
			break;
		}

		int32 MaxStack = Slot.ItemDefinition->MaxStackSize;

		int32 AmountToAdd = FMath::Min(
			RemainingQuantity,
			MaxStack
		);

		// Start from a copy of the incoming slot so any other per-item data comes along,
		// then give this stack its own quantity and identity.
		FInventorySlot NewSlot = Slot;

		NewSlot.Quantity = AmountToAdd;
		if (bCanPreserveId)
		{
			NewSlot.ItemInstanceId = Slot.ItemInstanceId;
			bCanPreserveId = false; // used up: any further stacks get fresh IDs
		}
		else
		{
			NewSlot.ItemInstanceId = FGuid::NewGuid();
		}

		Slots[EmptySlot] = NewSlot;

		RemainingQuantity -= AmountToAdd;

		bInventoryChanged = true;
	}

	// STEP 3: Notify listeners if anything changed.
	if (bInventoryChanged)
	{
		OnContainerChanged.Broadcast();
	}

	// Return how many units could not be added (0 = everything fit).
	return RemainingQuantity;
}

// Removes the whole stack. Removing only part of a stack is the TODO below.
bool UItemContainerComponent::RemoveItem(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	if (Slots[SlotIndex].IsEmpty())
	{
		return false;
	}	

	/* TODO: Add RemoveQuantity(SlotIndex, Amount) for partial stacks (eating, crafting). */

	Slots[SlotIndex] = FInventorySlot();
	OnContainerChanged.Broadcast();
	return true;
}

int32 UItemContainerComponent::FindSlotByGuid(const FGuid& ItemInstanceId) const
{
	// Every empty slot has an invalid ID, so searching for one would just return the first empty slot.
	if (!ItemInstanceId.IsValid())
	{
		return INDEX_NONE;
	}
	
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].ItemInstanceId == ItemInstanceId)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// Called when the game starts
void UItemContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	// SetCapacity refuses bad values (below 1, above MaxCapacity, or shrinking).
	// If the starting Capacity set in Details is refused, say so loudly instead of
	// silently leaving the container with zero slots.
	if (!SetCapacity(Capacity))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: starting Capacity %d was refused (MaxCapacity is %d). Container has %d slots."),
			*GetNameSafe(GetOwner()), Capacity, MaxCapacity, Slots.Num());
	}
	
}


