// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/PropertyEditor/Public/IDetailPropertyExtensionHandler.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Misc/NotifyHook.h"
#include "AbilityEditor/ablAbilityEditor.h"

class IDetailsView;
class SBox;
class SEditableTextBox;

class SAblAbilityDetailsView : public SCompoundWidget, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityDetailsView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InBlueprintEditor);
	virtual ~SAblAbilityDetailsView();

	/** Gets the property view for this details panel */
	TSharedPtr<class IDetailsView> GetPropertyView() const { return PropertyView; }
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// FNotifyHook interface
	virtual void NotifyPreChange(FEditPropertyChain* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged) override;
	// End of FNotifyHook interface

protected:
	// Should be implemented by derived classes to provide the object being observed
	virtual UObject* GetObjectToObserve() const { return nullptr; }

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) { return PropertyEditorWidget; }
private:
	/** Registers the designer specific customizations */
	void RegisterCustomizations();

	/** Handles the widget selection changing event in the editor, updates the details panel accordingly. */
	void OnEditorSelectionChanging();

	/** Handles the widget selection changed event in the editor, updates the details panel accordingly. */
	void OnEditorSelectionChanged();

	/** Handles the callback from the property detail view confirming the list of objects being edited has changed */
	void OnPropertyViewObjectArrayChanged(const FString& InTitle, const TArray<TWeakObjectPtr<UObject>>& InObjects);

	void ClearFocusIfOwned();

protected:
	/** The editor that owns this details view */
	TWeakPtr<class FAblAbilityEditor> BlueprintEditor;

	/** Property viewing widget */
	TSharedPtr<class IDetailsView> PropertyView;

	// Cached object view
	TWeakObjectPtr<UObject> LastObservedObject;
};


class IPropertyHandle;
class FAblDetailWidgetExtensionHandler : public IDetailPropertyExtensionHandler
{
public:
	FAblDetailWidgetExtensionHandler(TSharedPtr<class FAblAbilityEditor> InBlueprintEditor);

	virtual bool IsPropertyExtendable(const UClass* InObjectClass, const class IPropertyHandle& PropertyHandle) const override;

	virtual TSharedRef<SWidget> GenerateExtensionWidget(const IDetailLayoutBuilder& InDetailBuilder, const UClass* InObjectClass, TSharedPtr<IPropertyHandle> PropertyHandle) override;

private:
	TWeakPtr<class FAblAbilityEditor> BlueprintEditor;
};

