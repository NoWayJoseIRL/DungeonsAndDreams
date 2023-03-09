// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityPropertyViewers.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbleStyle.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/AppStyle.h"
#include "Tasks/IAblAbilityTask.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

const FName FAblAbilityPropertiesSummoner::ID("AblAbilityProperties");
const FName FAblAbilityTaskPropertiesSummoner::ID("AblAbilityTaskProperties");
const FName FAblAbilityEditorSettingsSummoner::ID("AblAbilityEditorSettings");

void SAblAbilityPropertiesTabBody::Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InAbilityEditor)
{
	m_AbilityEditor = InAbilityEditor;

	SAblAbilityDetailsView::Construct(SAblAbilityDetailsView::FArguments(), InAbilityEditor);
}

EVisibility SAblAbilityPropertiesTabBody::GetAssetDisplayNameVisibility() const
{
	return (GetObjectToObserve() != NULL) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SAblAbilityPropertiesTabBody::GetAssetDisplayName() const
{
	return LOCTEXT("AblAbilityPropertiesLabel", "Ability Properties");
}

UObject* SAblAbilityPropertiesTabBody::GetObjectToObserve() const
{
	return Cast<UObject>(m_AbilityEditor.Pin()->GetAbility());
}

TSharedRef<SWidget> SAblAbilityPropertiesTabBody::PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			// Header, shows name of the ability we are editing
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("Graph.TitleBackground")))
			.HAlign(HAlign_Center)
			.Visibility(this, &SAblAbilityPropertiesTabBody::GetAssetDisplayNameVisibility)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
					.Text(this, &SAblAbilityPropertiesTabBody::GetAssetDisplayName)
				]
			]
		]

	+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			PropertyEditorWidget
		];
}

FAblAbilityPropertiesSummoner::FAblAbilityPropertiesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	:FWorkflowTabFactory(ID, InHostingApp)
{
	TabLabel = LOCTEXT("AblAbilityAssetProperties_TabTitle", "Ability Properties");
	TabIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), "Able.Tabs.AbilityAssetDetails");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AblAbilityAssetProperties_MenuTitle", "Ability Properties");
	ViewMenuTooltip = LOCTEXT("AblAbilityAssetProperties_MenuToolTip", "Shows the ability asset properties");
}

TSharedRef<SWidget> FAblAbilityPropertiesSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FAblAbilityEditor> AbilityEditor = StaticCastSharedPtr<FAblAbilityEditor>(HostingApp.Pin());

	return SNew(SAblAbilityPropertiesTabBody, AbilityEditor);
}

void SAblAbilityTaskPropertiesTabBody::Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InAbilityEditor)
{
	m_AbilityEditor = InAbilityEditor;
	if (InAbilityEditor.IsValid())
	{
		if (const UAblAbilityTask* Task = InAbilityEditor->GetCurrentlySelectedAbilityTask())
		{
			m_CachedVisibility = Task->ShowEndTime();
		}
	}

	SAblAbilityDetailsView::Construct(SAblAbilityDetailsView::FArguments(), InAbilityEditor);
}

void SAblAbilityTaskPropertiesTabBody::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) 
{
	SAblAbilityDetailsView::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	TArray<UObject*> Objects;
	if (const UAblAbilityTask* Task = Cast<const UAblAbilityTask>(GetObjectToObserve()))
	{
		if (m_CachedVisibility != Task->ShowEndTime())
		{
			Objects.Add(GetObjectToObserve());

			check(PropertyView.IsValid());
			PropertyView->SetObjects(Objects, true);
		}

		m_CachedVisibility = Task->ShowEndTime();
	}
	else
	{
		PropertyView->SetObjects(Objects, true);
	}
}

EVisibility SAblAbilityTaskPropertiesTabBody::GetAssetDisplayNameVisibility() const
{
	return (GetObjectToObserve() != NULL) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SAblAbilityTaskPropertiesTabBody::GetAssetDisplayName() const
{
	if (const UAblAbilityTask* Task = m_AbilityEditor.Pin()->GetCurrentlySelectedAbilityTask())
	{
		return Task->GetTaskName();
	}

	return FText::GetEmpty();
}

UObject* SAblAbilityTaskPropertiesTabBody::GetObjectToObserve() const
{
	return Cast<UObject>(m_AbilityEditor.Pin()->GetMutableCurrentlySelectedAbilityTask());
}

TSharedRef<SWidget> SAblAbilityTaskPropertiesTabBody::PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			// Header, shows name of the ability we are editing
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("Graph.TitleBackground")))
			.HAlign(HAlign_Center)
			.Visibility(this, &SAblAbilityTaskPropertiesTabBody::GetAssetDisplayNameVisibility)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14))
				.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
				.Text(this, &SAblAbilityTaskPropertiesTabBody::GetAssetDisplayName)
				]
			]
		]

	+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			PropertyEditorWidget
		];
}

FAblAbilityTaskPropertiesSummoner::FAblAbilityTaskPropertiesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(ID, InHostingApp)
{
	TabLabel = LOCTEXT("AblAbilityTaskAssetProperties_TabTitle", "Task Properties");
	TabIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), "Able.Tabs.AbilityTaskAssetDetails");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AblAbilityTaskAssetProperties_MenuTitle", "Ability Task Properties");
	ViewMenuTooltip = LOCTEXT("AblAbilityTaskAssetProperties_MenuToolTip", "Shows the ability task asset properties");
}

TSharedRef<SWidget> FAblAbilityTaskPropertiesSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FAblAbilityEditor> AbilityEditor = StaticCastSharedPtr<FAblAbilityEditor>(HostingApp.Pin());

	return SNew(SAblAbilityTaskPropertiesTabBody, AbilityEditor);
}


void SAblAbilityEditorSettingsTabBody::Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InAbilityEditor)
{
	m_AbilityEditor = InAbilityEditor;

	SSingleObjectDetailsPanel::Construct(SSingleObjectDetailsPanel::FArguments().HostCommandList(InAbilityEditor->GetToolkitCommands()), true, true);
}

EVisibility SAblAbilityEditorSettingsTabBody::GetAssetDisplayNameVisibility() const
{
	return (GetObjectToObserve() != NULL) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SAblAbilityEditorSettingsTabBody::GetAssetDisplayName() const
{
	return LOCTEXT("AblAbilityEditorSettingsTab", "Editor Settings");
}

UObject* SAblAbilityEditorSettingsTabBody::GetObjectToObserve() const
{
	UAblAbilityEditorSettings& EditorSettings = m_AbilityEditor.Pin()->GetEditorSettings();
	return Cast<UObject>(&EditorSettings);
}

TSharedRef<SWidget> SAblAbilityEditorSettingsTabBody::PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			// Header, shows name of the ability we are editing
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Center)
		.Visibility(this, &SAblAbilityEditorSettingsTabBody::GetAssetDisplayNameVisibility)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14))
		.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
		.Text(this, &SAblAbilityEditorSettingsTabBody::GetAssetDisplayName)
		]
		]
		]

	+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			PropertyEditorWidget
		];
}


FAblAbilityEditorSettingsSummoner::FAblAbilityEditorSettingsSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	:FWorkflowTabFactory(ID, InHostingApp)
{
	TabLabel = LOCTEXT("AblAbilityEditorSettings_TabTitle", "Editor Settings");
	TabIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), "Able.Tabs.AbilityEditorSettings");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AblAbilityEditorSettings_MenuTitle", "Ability Editor Settings");
	ViewMenuTooltip = LOCTEXT("AblAbilityEditorSettings_MenuToolTip", "Shows the options for customizing the Ability editor behavior.");
}

TSharedRef<SWidget> FAblAbilityEditorSettingsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FAblAbilityEditor> AbilityEditor = StaticCastSharedPtr<FAblAbilityEditor>(HostingApp.Pin());

	return SNew(SAblAbilityEditorSettingsTabBody, AbilityEditor);
}

#undef LOCTEXT_NAMESPACE