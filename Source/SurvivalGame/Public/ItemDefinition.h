// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemType.h"
#include "ItemDefinition.generated.h"

// Forward declarations: only pointers to these are stored here, so their full headers aren't needed.
class UTexture2D;
class UStaticMesh;

/**
 * Shared description of one kind of item, one asset per kind (e.g. DA_Wood).
 * Everything here is identical for every copy of the item. Per-copy data (quantity,
 * identity, later durability) lives in FInventorySlot instead.
 * Edited in the asset's Details panel but read-only to Blueprints: the asset is shared, so
 * changing it at runtime would change every copy of the item (and, in the editor, the asset itself).
 */
UCLASS()
class SURVIVALGAME_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// Player-facing name shown in the UI.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	// Unique number for this kind of item; stacking matches on it.
	// 0 means unassigned, and AddItem refuses it. Keep numbers unique across assets.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 ItemId = 0;

	// Icon for a single item, shown in inventory slots.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Icon for a stack or bundle of this item.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> StackIcon = nullptr;

	// Mesh shown when this item lies in the world as an AItemPickup.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visuals")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	// Weight of ONE item. Multiply by Quantity for a stack's total.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float Weight = 0.0f; 

	// Base value of one item.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 BuyPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 SellPrice = 0;

	// Most of this item one slot can hold. 1 = never stacks (tools, weapons).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 MaxStackSize = 1;

	// Category, used by the item details panel and later by equipment and crafting.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::None;

	// Text for the item details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Description;
	
};
