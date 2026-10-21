// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ShapeComponent.h"
#include "ItemContainerComponent.h"
#include "ItemDefinition.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"

// Sets default values
AItemPickup::AItemPickup()
{
	// Pickups have no per-frame work.
	PrimaryActorTick.bCanEverTick = false;


	// The mesh is the root (see the header for why) and uses the engine's PhysicsActor collision profile.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	MeshComponent->SetSimulatePhysics(true);

	// FInventorySlot::Quantity defaults to 0 (right for an empty container slot), but a pickup of 0
	// destroys itself in BeginPlay. Start pickups at 1 instead; each Blueprint can still change it.
	Item.Quantity = 1;
}

bool AItemPickup::TryPickup(UItemContainerComponent* TargetContainer)
{
	if (TargetContainer == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: TryPickup called with null TargetContainer."), *GetNameSafe(this));
		return false;
	}

	// Destroy() only marks the actor; it lingers until the end of the frame. Without this check, a
	// second call in that same frame (e.g. capsule and mesh overlapping at once) would add Item again.
	if (IsActorBeingDestroyed())
	{
		return false;
	}

	// AddItem returns how many did NOT fit.
	int32 RemainingQuantity = TargetContainer->AddItem(Item);

	if (RemainingQuantity == 0)
	{
		// Everything fit: this pickup is used up.
		Destroy();
		return true;
	}

	else if	(RemainingQuantity < Item.Quantity)
	{
		// Some fit: keep only what's left on the ground.
		Item.Quantity = RemainingQuantity;
		return true;
	}
	else
	{
		// Nothing fit (container full, or it refused the item).
		return false;
	}

}

// Called when the game starts or when spawned
void AItemPickup::BeginPlay()
{
	Super::BeginPlay();

	// A pickup with no item or nothing in it can never be picked up. Remove it when the game
	// starts (never in the editor, see OnConstruction) and say why.
	if (!IsValid(Item.ItemDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ItemPickup has no ItemDefinition. Destroying."), *GetNameSafe(this));
		Destroy();
		return;
	}

	if (Item.Quantity <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ItemPickup has non-positive quantity (%d). Destroying."), *GetNameSafe(this), Item.Quantity);
		Destroy();
		return;
	}

	// Every shape component the Blueprint added (Sphere, Box or Capsule) becomes a pickup range. The
	// Blueprint only decides the shape and its size; the collision setup is forced here so it's always
	// right. The mesh is a static mesh component, not a shape, so it's never in this list.
	TArray<UShapeComponent*> PickupShapes;
	GetComponents<UShapeComponent>(PickupShapes);

	if (PickupShapes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ItemPickup has no shape component, so it can't be picked up. Add a Sphere, Box or Capsule under MeshComponent."), *GetNameSafe(this));
		return;
	}

	for (UShapeComponent* Shape : PickupShapes)
	{
		// Overlap-only: a blocking shape would hold the simulating mesh up off the ground.
		// Pawn only: the player walking in is the only thing that should trigger a pickup.
		Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
		Shape->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Shape->SetGenerateOverlapEvents(true);

		// AddUnique, so binding twice (e.g. BeginPlay running again) can't make one overlap fire twice.
		Shape->OnComponentBeginOverlap.AddUniqueDynamic(this, &AItemPickup::OnPickupRangeBeginOverlap);
	}
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Visuals only. This also runs in the editor, so it must never destroy the actor: a freshly
	// placed pickup is empty until its Details are filled in. Validity is checked in BeginPlay.
	UStaticMesh* WorldMesh = nullptr;
	if (IsValid(Item.ItemDefinition))
	{
		WorldMesh = Item.ItemDefinition->WorldMesh;
		if (!IsValid(WorldMesh))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: ItemDefinition %s has no WorldMesh assigned."), *GetNameSafe(this), *GetNameSafe(Item.ItemDefinition));
		}
	}
	MeshComponent->SetStaticMesh(WorldMesh);
}

void AItemPickup::OnPickupRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return;
	}

	// An actor can carry several containers (backpack, hotbar, equipment later on). Pickups go into
	// the one marked Inventory. A pawn without one (e.g. an animal) simply doesn't pick things up.
	TArray<UItemContainerComponent*> Containers;
	OtherActor->GetComponents<UItemContainerComponent>(Containers);

	for (UItemContainerComponent* Container : Containers)
	{
		if (Container->ContainerType == EContainerType::Inventory)
		{
			TryPickup(Container);
			return;
		}
	}
}
