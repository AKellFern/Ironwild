// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemContainerComponent.h"
#include "ItemDefinition.h"	




// Sets default values for this component's properties
UItemContainerComponent::UItemContainerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	

	// ...
}

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

	Capacity = NewCapacity;
	Slots.SetNum(Capacity);
	OnContainerChanged.Broadcast();
	return true;
}

int32 UItemContainerComponent::AddItem(const FInventorySlot& Slot)
{
	// Validate the container.
	if (ContainerType == EContainerType::None)
	{
		return Slot.Quantity;
	}

	// Validate the incoming item.
	if (Slot.ItemDefinition == nullptr)
	{
		return Slot.Quantity;
	}

	// Nothing (or a nonsense negative amount) to add, so nothing is left over.
	if (Slot.Quantity <= 0)
	{
		return 0;
	}

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

		FInventorySlot NewSlot = Slot;

		NewSlot.Quantity = AmountToAdd;
		NewSlot.ItemInstanceId = FGuid::NewGuid();

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


// Called every frame
void UItemContainerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

