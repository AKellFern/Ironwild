// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemContainerComponent.h"


// Sets default values for this component's properties
UItemContainerComponent::UItemContainerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	Slots.SetNum(Capacity);

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

	/* TODO: Implement the actual removal logic */
	Slots[SlotIndex] = FInventorySlot();
	return true;
}


// Called when the game starts
void UItemContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UItemContainerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

