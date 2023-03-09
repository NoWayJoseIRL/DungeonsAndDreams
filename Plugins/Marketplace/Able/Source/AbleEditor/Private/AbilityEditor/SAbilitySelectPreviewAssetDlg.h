// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AssetRegistry/AssetData.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

/* Creates a modal window that allows the user to select what our Preview Actor should look like. */
class SAblAbilitySelectPreviewAssetDlg : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAblAbilitySelectPreviewAssetDlg)
	{

	}

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/* Creates and spawns the modal window. Returns true if an Asset was selected, false otherwise. */
	bool DoModal();

	/* Returns the Asset selected, if there was one. */
	const FAssetData& GetSelectedAsset() const { return m_AssetSelected; }
private:
	/* Callback for when an Asset is selected by the user. */
	void OnAssetSelected(const FAssetData& AssetData);

	/* Callback for when an Asset is double clicked by the user. */
	void OnAssetDoubleClicked(const FAssetData& AssetData);

	/* Callback for if an Asset should be filtered or not. */
	bool OnShouldFilterAsset(const FAssetData& AssetData) const;

	/* Callback for when the OK button is clicked. */
	FReply OkClicked();

	/* Closes the window. */
	void CloseDialog();

	/* Callback for when the Cancel button is clicked. */
	FReply CancelClicked();

	// Input Override. 
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);

	/* A pointer to the window that is asking the user to select an asset.*/
	TWeakPtr<SWindow> m_ModalWindow;

	/* The selected asset */
	FAssetData m_AssetSelected;

	/* True if Ok was clicked */
	bool m_bOkClicked;

};