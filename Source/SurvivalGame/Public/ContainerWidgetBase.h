// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemContainerComponent.h"
#include "ContainerWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API UContainerWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
		void RefreshContainer();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
		void InitializeContainer(UItemContainerComponent* NewContainment);
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
		TObjectPtr<UItemContainerComponent> Container;

protected:

	virtual void NativeDestruct() override;
	
	
};
