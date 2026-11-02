#pragma once

#include "CoreMinimal.h"
#include "ItemType.generated.h"

// Defines the primary category of an item.
// Used by inventory, equipment, crafting, and other gameplay systems.
UENUM(BlueprintType)
enum class EItemType : uint8
{
    None UMETA(DisplayName = "None"),           // Default or unassigned item type.

    Resource UMETA(DisplayName = "Resource"),       // Raw materials gathered from the environment, such as wood, stone, plant fiber, and animal hide.

    Consumable UMETA(DisplayName = "Consumable"),     // Items used up when consumed, such as food, healing potions, or stamina-restoring items.

    Tool UMETA(DisplayName = "Tool"),           // Equipment used for gathering or performing tasks, such as hatchets, pickaxes, and hammers.

    Weapon UMETA(DisplayName = "Weapon"),         // Items primarily intended for combat, such as swords, bows, and spears.

    Armor UMETA(DisplayName = "Armor"),          // Wearable equipment that provides protection, such as helmets, chest armor, and boots.

    Material UMETA(DisplayName = "Material"),       // Processed or refined crafting ingredients, such as copper ingots, leather, and wooden planks.

    Miscellaneous UMETA(DisplayName = "Miscellaneous"),  // General-purpose items that do not fit into another category.

    Quest UMETA(DisplayName = "Quest Item"),          // Items associated with objectives, discoveries, or progression.

    Magic UMETA(DisplayName = "Magic")           // Magical artifacts, enchanted objects, or items primarily associated with supernatural abilities.
};