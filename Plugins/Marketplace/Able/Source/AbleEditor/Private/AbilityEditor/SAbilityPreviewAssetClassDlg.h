// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

/* Creates a modal window that allows the user to select what class our Preview Actor will be. */
class SAblAbilityPreviewAssetClassDlg : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAblAbilityPreviewAssetClassDlg)
	{

	}

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/* Creates and spawns the modal window. Returns true if an Asset was selected, false otherwise. */
	bool DoModal();

	/* Returns the Asset selected, if there was one. */
	const UClass* GetSelectedClass() const { return m_Class; }

	const FAssetData& GetSelectedAsset() const { return m_AssetSelected; }
private:
	/* Creates the Asset picker. */
	void MakePicker();

	/* Callback for when a Class is selected by the user. */
	void OnClassSelected(UClass* InClass);

	void OnAssetSelected(const FAssetData& Asset);

	bool ShouldFilterAsset(const FAssetData& Asset) const;

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

	/* The container for the Class picker */
	TSharedPtr<SVerticalBox> m_ClassPickerContainer;

	/* The selected asset */
	UClass* m_Class;

	/* True if Ok was clicked */
	bool m_bOkClicked;

	FAssetData m_AssetSelected;

};