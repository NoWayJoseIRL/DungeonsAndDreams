// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AssetRegistry/AssetData.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

class UAblAbilityValidator;

/* Validator Results Dialog is a modal window that shows the results from an Ability Validator. */
class SAblAbilityValidatorResultsDlg : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAblAbilityValidatorResultsDlg)
	{
	}
	SLATE_ARGUMENT(TWeakObjectPtr<UAblAbilityValidator>, Validator)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	// Tick Override
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
	/* Creates and displays the window. Returns true if the OK button was selected, false otherwise. */
	bool DoModal();
private:
	/* Populates the output of the dialog. */
	void PopulateOutput(bool ForceRepopulate = false);

	/* Returns the appropriate color of a line of text depending on its flags. */
	FSlateColor GetColorForFlags(int8 Flags) const;

	/* Callback for when the OK button is clicked. */
	FReply OkClicked();

	/* Closes the dialog. */
	void CloseDialog();

	/* Checked state of the Info checkbox. */
	ECheckBoxState ShouldShowInfo() const;

	/* Callback for when the Info checkbox is changed. */
	void OnShowInfoCheckboxChanged(ECheckBoxState NewCheckedState);

	/* Checked state of the Warnings checkbox. */
	ECheckBoxState ShouldShowWarnings() const;

	/* Callback for when the Warnings checkbox is changed. */
	void OnShowWarningsCheckboxChanged(ECheckBoxState NewCheckedState);

	/* Checked state of the Errors checkbox. */
	ECheckBoxState ShouldShowErrors() const;

	/* Callback for when the Errors checkbox is changed. */
	void OnShowErrorsCheckboxChanged(ECheckBoxState NewCheckedState);

	/* Bit flags for which flags we should display on the window. */
	int8 m_ShowFlags;

	/* Total number of lines currently output. */
	int32 m_CachedTotalOutputLines;

	/* A pointer to our window.*/
	TWeakPtr<SWindow> m_ModalWindow;

	/* Container for our text output. */
	TSharedPtr<SScrollBox> m_LogOutput;

	/* Pointer to the Ability Validator we represent. */
	TWeakObjectPtr<UAblAbilityValidator> m_Validator;

};
