#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "InventorySlot.generated.h"

// Forward declaration: the slot only points at the definition, so its full header isn't needed here.
class UItemDefinition;

// One slot in a container: which kind of item, how many, and which specific copy.
// A slot whose ItemDefinition is null is empty. Always held by value (no pointer).
// Per-copy data added later (durability, etc.) belongs here, so it travels with
// drops, pickups and moves automatically.
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()


public:

	// What kind of item this is (the shared data asset). Null means the slot is empty.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UItemDefinition> ItemDefinition = nullptr;

	// Identity of this specific stack or copy. Invalid until AddItem assigns one.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	struct FGuid ItemInstanceId;

	// How many are in this slot. Never above the definition's MaxStackSize.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	// True when nothing is in this slot.
	bool IsEmpty() const
	{
		return ItemDefinition == nullptr;
	}

};