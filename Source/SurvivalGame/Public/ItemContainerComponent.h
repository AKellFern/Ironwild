// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventorySlot.h"
#include "ContainerType.h"
#include "ItemContainerComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContainerChangedDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SURVIVALGAME_API UItemContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UItemContainerComponent();

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Inventory")
		TArray<FInventorySlot> Slots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
		int32 Capacity = 20;

	int32 FindEmptySlot() const;

	int32 FindStackableSlot(const FInventorySlot& Slot) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")	
		EContainerType ContainerType = EContainerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
		int32 MaxCapacity = 40;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
		FOnContainerChangedDelegate OnContainerChanged;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
		bool SetCapacity(int32 NewCapacity);

	// Returns the quantity that could NOT be added (0 = everything fit; == Slot.Quantity = nothing fit).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
		int32 AddItem(const FInventorySlot& Slot);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
		bool RemoveItem(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
		int32 FindSlotByGuid(const FGuid& ItemInstanceId) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
	
};
