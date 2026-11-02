#pragma once

#include "CoreMinimal.h"
#include "ContainerType.generated.h"

// What role a container plays. Every container must be given a real type:
// AddItem refuses all items while a container is still set to None.
UENUM(BlueprintType)
enum class EContainerType : uint8
{
	// Unassigned. Containers left at this refuse every item.
	None,
	// The player's backpack.
	Inventory,
	// Worn gear (planned, Step 5).
	Equipment,
	// Chests and other containers placed in the world (planned, Step 3).
	Storage,
	// Quick-access bar (planned, Step 4).
	Hotbar
};