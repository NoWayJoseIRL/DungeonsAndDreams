// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AbilityEditor/ablAbilityEditor.h"

#include "IDocumentation.h"

#include "SEditorViewport.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

class FEditorViewportClient;

/* The Viewport for the Ability Editor. */
class SAbilityEditorViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SAbilityEditorViewport) {}
	SLATE_END_ARGS()

	virtual ~SAbilityEditorViewport();

	void Construct(const FArguments& InArgs, TSharedPtr<class FAblAbilityEditor> InAbilityEditor, TSharedPtr<class SAbilityEditorViewportTabBody> InTabBody);
	
	/* Returns the Viewport client. */
	TSharedPtr<class FAbilityEditorViewportClient> GetAbilityViewportClient() const { return m_LevelViewportClient; }
protected:
	// SEditorViewport interface
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	// End of SEditorViewport interface

protected:
	/* Viewport client */
	TSharedPtr<class FAbilityEditorViewportClient> m_LevelViewportClient;

	/* Pointer to the compound widget that owns this viewport widget */
	TWeakPtr<class SAbilityEditorViewportTabBody> m_TabBodyPtr;

	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;
};

/* Widget that contains our Viewport. */
class SAbilityEditorViewportTabBody : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAbilityEditorViewportTabBody) {}

	SLATE_ARGUMENT(TSharedPtr<FAblAbilityEditor>, AbilityEditor)
	SLATE_END_ARGS()
public:

	SAbilityEditorViewportTabBody();
	virtual ~SAbilityEditorViewportTabBody();

	void Construct(const FArguments& InArgs);

	/* Refreshes the Viewport. */
	void RefreshViewport();

	/* Captures the current Viewport image as a thumbnail. */
	void CaptureThumbnail() const;

	/* Returns the Viewport client. */
	TSharedPtr<FEditorViewportClient> GetLevelViewportClient() const { return m_LevelViewportClient; }

	/* Returns the Viewport widget. */
	TSharedPtr<SAbilityEditorViewport> GetViewportWidget() const { return m_ViewportWidget; }

	/* Returns the Viewport Commands. */
	TSharedPtr<FUICommandList> GetCommandList() { return m_Commands; }

private:

	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	/* Level viewport client.  */
	TSharedPtr<FEditorViewportClient> m_LevelViewportClient;

	/* Viewport widget. */
	TSharedPtr<SAbilityEditorViewport> m_ViewportWidget;

	/* Viewport Commands. */
	TSharedPtr<FUICommandList> m_Commands;
};

/* Simple Tab Summoner for the Viewport. */
struct FAblAbilityEditorViewportSummoner : public FWorkflowTabFactory
{
	FAblAbilityEditorViewportSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	// FWorkflowTabFactory interface
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	// Create a tooltip widget for the tab
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override
	{
		return  IDocumentation::Get()->CreateToolTip(LOCTEXT("AblAbilityViewportTooltip", "Viewport allows you to see how this Ability would play in-game."), NULL, TEXT("Shared/Editors/Able"), TEXT("AbilityAssetDetail_Window"));
	}
	// FWorkflowTabFactory interface

	static const FName ID;
};

#undef LOCTEXT_NAMESPACE