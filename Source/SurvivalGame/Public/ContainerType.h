#pragma once

#include "CoreMinimal.h"
#include "ContainerType.generated.h"

UENUM(BlueprintType)
enum class EContainerType : uint8
{
	None,
	Inventory,
	Equipment,
	Storage,
	Hotbar
};