// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AdvancedPreviewScene.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Textures/SlateIcon.h"

struct FAssetData;
class FAblAbilityEditor;
class FAssetThumbnailPool;
class UAblAbilityEditorSettings;

/* Ability Editor Toolbar */
class FAblAbilityEditorToolbar : public TSharedFromThis<FAblAbilityEditorToolbar>
{
public:
	FAblAbilityEditorToolbar(TSharedPtr<FAblAbilityEditor> InAbilityEditor);

	/* Populates the Toolbar with basic asset commands. */
	void SetupToolbar(TSharedPtr<FExtender> Extender );

	/* Populates the Toolbar with Timeline specific commands. */
	void AddTimelineToolbar(TSharedPtr<FExtender> Extender);

	void OnPreviewAssetSelected(const FAssetData& Asset);
	void OnTargetAssetSelected(const FAssetData& Asset);
	FAssetData OnGetPreviewAsset() const;
	FAssetData OnGetTargetAsset() const;

	TSharedRef<SWidget> OnAddPreviewAssetWidgets();
	TSharedRef<SWidget> OnAddTargetAssetWidgets();
	TSharedRef<SWidget> GenerateForceTargetComboBox();
	void SetForceTarget(int index);

	void OnResetPreviewAsset();
	void OnResetTargetAsset();
	FReply OnAddTargetAsset();
private:
	bool IsAssetValid(const FAssetData& Asset) const;

	/* Helper methods for the Toolbar. */
	void FillAbilityEditorModeToolbar(FToolBarBuilder& ToolbarBuilder);
	void FillTimelineModeToolbar(FToolBarBuilder& ToolbarBuilder);

	/* Returns the Icon based on our Ability status (Play/Stop). */
	FSlateIcon GetPlayIcon() const;

	/* Returns the proper Text based on our Ability status. */
	FText GetPlayText() const;

	/* Returns the proper Tooltip based on our AAbility status. */
	FText GetPlayToolTip() const;

	/* Pointer back to the Ability Editor that owns us. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	/* Weakptr to our Editor Settings. */
	TWeakObjectPtr<UAblAbilityEditorSettings> m_AbilityEditorSettings;

	/* Play Icon */
	FSlateIcon m_PlayIcon;

	/* Pause Icon */
	FSlateIcon m_PauseIcon;

	/* Thumbnail Cache */
	TSharedPtr<FAssetThumbnailPool> m_PreviewThumbnailPool;
	TSharedPtr<FAssetThumbnailPool> m_TargetThumbnailPool;
};


