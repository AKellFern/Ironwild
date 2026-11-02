// Fill out your copyright notice in the Description page of Project Settings.


#include "ContainerWidgetBase.h"

void UContainerWidgetBase::InitializeContainer(UItemContainerComponent* NewContainment)
{
	// Nothing to show. Keep whatever container was set before (none, on a first call).
	if (NewContainment == nullptr)
	{
		return;
	}

	// Switching to a different container: stop listening to the old one first, or changes to it
	// would keep redrawing this widget with the new container's contents.
	if (IsValid(Container) && Container != NewContainment)
	{
		Container->OnContainerChanged.RemoveDynamic(this, &UContainerWidgetBase::RefreshContainer);
	}

	Container = NewContainment;

	// From now on, every change to the container redraws this widget. AddUniqueDynamic (rather than
	// AddDynamic) means calling this again with the same container can't bind it twice.
	Container->OnContainerChanged.AddUniqueDynamic(this, &UContainerWidgetBase::RefreshContainer);

	// Draw the current contents now instead of waiting for the first change.
	RefreshContainer();


}


void UContainerWidgetBase::NativeDestruct()
{
	Super::NativeDestruct();
	
	// Nothing to unsubscribe from if the widget was never initialized (call forgotten, or null passed
	// in), or if the container's actor was already destroyed. IsValid covers both cases.
	if (!IsValid(Container))
	{
		return;
	}

	// Stop listening, or a closed widget would keep redrawing and stay alive in memory.
	Container->OnContainerChanged.RemoveDynamic(this, &UContainerWidgetBase::RefreshContainer);
}