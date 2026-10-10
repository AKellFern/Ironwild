// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemContainerComponent.h"


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
	/* TODO: Broadcast OnContainerChanged */
	return true;
}

bool UItemContainerComponent::AddItem(const FInventorySlot& Slot)
{
	if (ContainerType == EContainerType::None)
	{
		return false;
	}

	if (Slot.ItemDefinition == nullptr)
	{
		return false;
	}

	int32 EmptySlot = FindEmptySlot();

	if (EmptySlot == INDEX_NONE)
	{
		return false;
	}

	Slots[EmptySlot] = Slot;
	return true;
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
	return true;
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

