// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once
#include "Animation/AnimationAsset.h"
#include "AssetRegistry/AssetData.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

class UAblAbility;
class UAblAbilityTask;
class SAblAbilityAnimationSelector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityAnimationSelector)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/* Construct and launch the modal dialogue. Returns true if the user selected an Animation asset, false otherwise. */
	bool DoModal();

	/* Returns the selected Animation asset, if one was picked. */
	const TWeakObjectPtr<UAnimationAsset>& GetAnimationAsset() const { return m_AnimationAsset; }

	/* Returns whether the user would like to adjust the Ability length to match the Animation asset length. */
	bool GetResizeAbilityLength() const { return m_bResizeAbilityLength; }
private:
	/* Create Asset Picker Window. */
	void MakeAssetPicker();

	/* Called when an Asset is selected by the user. */
	void OnAssetSelected(const FAssetData& AssetData);

	/* Called when an Asset is doubled-clicked on by the user. */
	void OnAssetDoubleClicked(const FAssetData& AssetData);

	/* Handle OK button being clicked. */
	FReply OkClicked();

	/* Close the Modal window. */
	void CloseDialog();

	/* Handle Cancel button being clicked. */
	FReply CancelClicked();

	// Input Override
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);

	/* Check State for the Resize Ability checkbox. */
	ECheckBoxState ShouldResizeAbilityTimeline() const;

	/* Callback when the Resize Ability checkbox is checked. */
	void OnResizeAbilityTimelineCheckboxChanged(ECheckBoxState NewCheckedState);

	/* A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> m_PickerWindow;

	/* The container for the asset picker */
	TSharedPtr<SVerticalBox> m_AssetPickerContainer;

	/* The selected asset */
	TWeakObjectPtr<UAnimationAsset> m_AnimationAsset;

	/* True if Ok was clicked */
	bool m_bOkClicked;

	/** True if we should resize the Ability length or not. */
	bool m_bResizeAbilityLength;

};