// Fill out your copyright notice in the Description page of Project Settings.


#include "ContainerWidgetBase.h"

void UContainerWidgetBase::InitializeContainer(UItemContainerComponent* NewContainment)
{
	if (NewContainment == nullptr)
	{
		return;
	}

	Container = NewContainment;

	Container->OnContainerChanged.AddDynamic(this, &UContainerWidgetBase::RefreshContainer);

	RefreshContainer();


}


void UContainerWidgetBase::NativeDestruct()
{
	Super::NativeDestruct();
	
	if (Container == nullptr)
	{
		return;
	}

	Container->OnContainerChanged.RemoveDynamic(this, &UContainerWidgetBase::RefreshContainer);
}