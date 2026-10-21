// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "InventorySlot.h"
#include "ItemPickup.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;
class UItemContainerComponent;

/**
 * An item lying in the world. Holds one FInventorySlot (item, quantity, identity) and moves it
 * into a container when picked up. Simulates physics so dropped items fall and settle.
 *
 * Pickup range: each Blueprint adds its OWN shape component (Sphere, Box or Capsule) in the
 * Components panel, attached under MeshComponent, and sizes it there. At BeginPlay this class finds
 * every shape component on the actor, forces it to overlap-only, and listens for pawns entering it.
 * A Blueprint with no shape component logs a warning and can't be picked up.
 */
UCLASS()
class SURVIVALGAME_API AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AItemPickup();


	// Root and visible body. It is the root because it simulates physics: a simulating child
	// component would detach from the actor and leave everything else behind. Add the pickup-range
	// shape as a child of this, so it moves with the item as it falls and rolls.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// The item this pickup represents. Editable on each placed pickup, and settable when spawned.
	// Quantity defaults to 1 here (set in the constructor), since a pickup of 0 destroys itself.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Pickup")
	FInventorySlot Item;

	// Moves as much of Item as fits into TargetContainer. Returns true if ANY quantity was taken
	// (destroys this pickup when everything was taken; otherwise keeps the remainder). Returns false
	// if nothing fit, TargetContainer is null, or this pickup is already being destroyed, so calling
	// it twice in one frame can never hand out the same items twice.
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	bool TryPickup(UItemContainerComponent* TargetContainer);


protected:
	// Called when the game starts or when spawned. Removes pickups that are empty or have no item,
	// then sets up every shape component on the actor as a pickup range (see the class comment).
	virtual void BeginPlay() override;

	// Sets the mesh from Item's WorldMesh. Runs in the editor as well as when spawned, so the
	// viewport always shows the right mesh.
	virtual void OnConstruction(const FTransform& Transform) override;

	// Bound to each pickup-range shape's OnComponentBeginOverlap in BeginPlay. Finds the overlapping
	// actor's Inventory container and calls TryPickup with it. Must be a UFUNCTION to bind to the
	// (dynamic) overlap delegate.
	// A character has TWO components on the Pawn channel (capsule and mesh), so this usually fires
	// twice for one character. That's safe: TryPickup refuses once the pickup is being destroyed, and
	// a second try on a full container just takes nothing.
	UFUNCTION()
	void OnPickupRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
