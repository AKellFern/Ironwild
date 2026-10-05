#pragma once

#include "CoreMinimal.h"
#include "InventorySlot.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()


public:

	UPROPERTY(BlueprintReadWrite)
	UItemDefinition* ItemDefinition = nullptr;

	UPROPERTY(BlueprintReadWrite)
	int32 Quantity = 0;

	bool IsEmpty() const
	{
		return ItemDefinition == nullptr;
	}

};