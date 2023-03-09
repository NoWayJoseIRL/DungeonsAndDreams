// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "Editor/KismetWidgets/Public/SSingleObjectDetailsPanel.h"
#include "AbilityEditor/SAbilityDetailView.h"
#include "IDocumentation.h"

#include "Widgets/SWidget.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

class FAblAbilityEditor;

/* Various Property summoners/tabs for the Layouts. */

// Ability Properties Tab
class SAblAbilityPropertiesTabBody : public SAblAbilityDetailsView
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityPropertiesTabBody) {}
	SLATE_END_ARGS()

private:

	TWeakPtr<class FAblAbilityEditor> m_AbilityEditor;
public:
	void Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InAbilityEditor);
	
	virtual EVisibility GetAssetDisplayNameVisibility() const;

	virtual FText GetAssetDisplayName() const;

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const override;

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) override;

};

// Summoner for Ability Properties
struct FAblAbilityPropertiesSummoner : public FWorkflowTabFactory
{
public:
	FAblAbilityPropertiesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	// FWorkflowTabFactory interface
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	// Create a tooltip widget for the tab
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override
	{
		return  IDocumentation::Get()->CreateToolTip(LOCTEXT("AblAbilityAssetPropertiesTooltip", "The Ability properties tab allows you to edit generic properties of an ability (length, etc)."), NULL, TEXT("Shared/Editors/Able"), TEXT("AbilityAssetDetail_Window"));
	}
	// FWorkflowTabFactory interface

	static const FName ID;
};

// Task Properties Tab
class SAblAbilityTaskPropertiesTabBody : public SAblAbilityDetailsView
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTaskPropertiesTabBody) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InAbilityEditor);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual EVisibility GetAssetDisplayNameVisibility() const;

	virtual FText GetAssetDisplayName() const;

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const override;

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) override;

private:
	/* Our Ability Editor Instance. */
	TWeakPtr<class FAblAbilityEditor> m_AbilityEditor;

	/* Cached Visibility for our Show End Time value. */
	EVisibility m_CachedVisibility;
};

// Summoner for Task Properties
struct FAblAbilityTaskPropertiesSummoner : public FWorkflowTabFactory
{
public:
	FAblAbilityTaskPropertiesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	// FWorkflowTabFactory interface
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	// Create a tooltip widget for the tab
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override
	{
		return  IDocumentation::Get()->CreateToolTip(LOCTEXT("AblAbilityTaskPropertiesTooltip", "The Ability Task properties tab allows you to edit properties of an ability task."), NULL, TEXT("Shared/Editors/Able"), TEXT("AbilityTaskDetail_Window"));
	}
	// FWorkflowTabFactory interface

	static const FName ID;
};

// Task Properties Tab
class SAblAbilityEditorSettingsTabBody : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityEditorSettingsTabBody) {}
	SLATE_END_ARGS()

private:

	TWeakPtr<class FAblAbilityEditor> m_AbilityEditor;
public:
	void Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InAbilityEditor);

	virtual EVisibility GetAssetDisplayNameVisibility() const;

	virtual FText GetAssetDisplayName() const;

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const override;

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) override;
};

// Summoner for Editor Settings
struct FAblAbilityEditorSettingsSummoner : public FWorkflowTabFactory
{
public:
	FAblAbilityEditorSettingsSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	// FWorkflowTabFactory interface
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	// Create a tooltip widget for the tab
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override
	{
		return  IDocumentation::Get()->CreateToolTip(LOCTEXT("AblAbilityEditorSettingsTooltip", "The Editor Settings tab lets you easily configure your settings to fit your style."), NULL, TEXT("Shared/Editors/Able"), TEXT("AbilityTaskDetail_Window"));
	}
	// FWorkflowTabFactory interface

	static const FName ID;
};

#undef LOCTEXT_NAMESPACE