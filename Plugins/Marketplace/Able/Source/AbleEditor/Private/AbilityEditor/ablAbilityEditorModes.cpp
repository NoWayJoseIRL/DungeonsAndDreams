// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/ablAbilityEditorModes.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/SAbilityEditorToolbar.h"
#include "AbilityEditor/SAbilityPropertyViewers.h"
#include "AbilityEditor/SAbilityTimeline.h"
#include "AbilityEditor/SAbilityViewport.h"
#include "AbilityEditor/SAbilityDetailView.h"
#include "Editor/Kismet/Public/BlueprintEditorTabs.h"
#include "Editor/Kismet/Public/SBlueprintEditorToolbar.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

const FName FAblAbilityEditorModes::AblAbilityEditorName("AblAbilityEditorApp");
const FName FAblAbilityEditorModes::AbilityBlueprintMode("Graph");
const FName FAblAbilityEditorModes::AbilityTimelineMode("Timeline");

const FName FAblAbilityTimelineSummoner::ID("AblAbilityTimeline");

FAppAbilityEditorTimelineMode::FAppAbilityEditorTimelineMode(TSharedPtr<class FAblAbilityEditor> AbilityEditor)
	: FApplicationMode(FAblAbilityEditorModes::AbilityTimelineMode, FAblAbilityEditorModes::GetLocalizedMode)
{
	m_AbilityEditor = AbilityEditor;
	
	TabLayout = FTabManager::NewLayout("Ability_AbilityTimelineEditMode_Layout_v10")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			//->Split
			//(
			//	// Top toolbar
			//	FTabManager::NewStack()
			//	->SetSizeCoefficient(0.2f)
			//	->SetHideTabWell(true)
			//	->AddTab(AbilityEditor->GetToolbarTabId(), ETabState::OpenedTab)
			//)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				//->SetSizeCoefficient(0.8f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->SetHideTabWell(false)
					->AddTab(FAblAbilityPropertiesSummoner::ID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.4f)
						->SetHideTabWell(true)
						->AddTab(FAblAbilityEditorViewportSummoner::ID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->SetHideTabWell(true)
						->AddTab(FAblAbilityTimelineSummoner::ID, ETabState::OpenedTab)
					)

				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->SetHideTabWell(false)
					->AddTab(FAblAbilityTaskPropertiesSummoner::ID, ETabState::OpenedTab)
					->AddTab(FAblAbilityEditorSettingsSummoner::ID, ETabState::OpenedTab)
					->SetForegroundTab(FAblAbilityTaskPropertiesSummoner::ID)
				)
			)
		);

	
	m_AbilityEditorTabFactories.RegisterFactory(MakeShareable(new FAblAbilityTimelineSummoner(AbilityEditor)));
	m_AbilityEditorTabFactories.RegisterFactory(MakeShareable(new FAblAbilityPropertiesSummoner(AbilityEditor)));
	m_AbilityEditorTabFactories.RegisterFactory(MakeShareable(new FAblAbilityTaskPropertiesSummoner(AbilityEditor)));
	m_AbilityEditorTabFactories.RegisterFactory(MakeShareable(new FAblAbilityEditorViewportSummoner(AbilityEditor)));
	m_AbilityEditorTabFactories.RegisterFactory(MakeShareable(new FAblAbilityEditorSettingsSummoner(AbilityEditor)));

	ToolbarExtender = MakeShareable(new FExtender);
	m_AbilityEditor.Pin()->GetAbilityToolbar()->AddTimelineToolbar(ToolbarExtender);
}


void FAppAbilityEditorTimelineMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FBlueprintEditor> BP = m_AbilityEditor.Pin();

	BP->RegisterToolbarTab(InTabManager.ToSharedRef());

	BP->PushTabFactories(m_AbilityEditorTabFactories);
}

void FAppAbilityEditorTimelineMode::PostActivateMode()
{
	FApplicationMode::PostActivateMode();
	
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->Reinitialize();
	}
}

FAppAbilityEditorBlueprintMode::FAppAbilityEditorBlueprintMode(TSharedPtr<class FAblAbilityEditor> AbilityEditor)
	: FBlueprintEditorApplicationMode(StaticCastSharedPtr<FBlueprintEditor>(AbilityEditor), FAblAbilityEditorModes::AbilityBlueprintMode, FAblAbilityEditorModes::GetLocalizedMode, false, false)
{
	m_AbilityEditor = AbilityEditor;

	TabLayout = FTabManager::NewLayout("Ability_AbilityBlueprintEditMode_Layout_v4")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					// Left side
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.2f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.5f)
						->AddTab(FAblAbilityPropertiesSummoner::ID, ETabState::OpenedTab)
						->AddTab(FBlueprintEditorTabs::MyBlueprintID, ETabState::OpenedTab)
						->SetForegroundTab(FBlueprintEditorTabs::MyBlueprintID)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.5f)
						->AddTab(FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.80f)
					->AddTab("Document", ETabState::ClosedTab)
				)

			)
		);
	
	m_AbilityEditorTabFactories.RegisterFactory(MakeShareable(new FAblAbilityPropertiesSummoner(AbilityEditor)));

	ToolbarExtender = MakeShareable(new FExtender);
	if (UToolMenu* Toolbar = AbilityEditor->RegisterModeToolbarIfUnregistered(GetModeName()))
	{
		AbilityEditor->GetToolbarBuilder()->AddCompileToolbar(Toolbar);
		AbilityEditor->GetToolbarBuilder()->AddScriptingToolbar(Toolbar);
		AbilityEditor->GetToolbarBuilder()->AddBlueprintGlobalOptionsToolbar(Toolbar);
		AbilityEditor->GetToolbarBuilder()->AddDebuggingToolbar(Toolbar);
	}

}

FAppAbilityEditorBlueprintMode::~FAppAbilityEditorBlueprintMode()
{

}

void FAppAbilityEditorBlueprintMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FBlueprintEditor> BP = m_AbilityEditor.Pin();

	BP->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	BP->PushTabFactories(CoreTabFactories);
	BP->PushTabFactories(BlueprintEditorTabFactories);
	BP->PushTabFactories(m_AbilityEditorTabFactories);

	// Add custom tab factories
}

void FAppAbilityEditorBlueprintMode::PostActivateMode()
{
	FBlueprintEditorApplicationMode::PostActivateMode();

	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->Reinitialize();
	}

}

FAblAbilityTimelineSummoner::FAblAbilityTimelineSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	:FWorkflowTabFactory(ID, InHostingApp)
{
	TabLabel = LOCTEXT("AblAbilityTimeline_TabTitle", "Timeline");
	TabIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), "Able.Tabs.AbilityTimeline");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AblAbilityTimeline_MenuTitle", "Ability Task Properties");
	ViewMenuTooltip = LOCTEXT("AblAbilityTimeline_MenuToolTip", "Shows the ability task asset properties");
}

TSharedRef<SWidget> FAblAbilityTimelineSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FAblAbilityEditor> InAbilityEditor = StaticCastSharedPtr<FAblAbilityEditor>(HostingApp.Pin());

	return SNew(SAblAbilityTimelinePanel).AbilityEditor(InAbilityEditor);
}


#undef LOCTEXT_NAMESPACE
