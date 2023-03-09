// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityValidatorResultsDlg.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityValidator.h"
#include "AbilityEditor/AblAbilityTaskValidator.h"

#include "Editor.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblAbilityValidatorResultsDlg::Construct(const FArguments& InArgs)
{
	m_Validator = InArgs._Validator;
	m_ShowFlags = (1 << (uint8)EAblAbilityValidatorLogType::TotalTypes) - 1;
	m_CachedTotalOutputLines = 0;

	ChildSlot
		[
			SNew(SBorder)
			.Visibility(EVisibility::Visible)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
			[
				SNew(SBox)
				.Visibility(EVisibility::Visible)
				.WidthOverride(500.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Content()
						[
							SAssignNew(m_LogOutput, SScrollBox)
						]
					]
					+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Bottom)
						.Padding(4)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.AutoWidth()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SAblAbilityValidatorResultsDlg::OnShowInfoCheckboxChanged)
								.IsChecked(this, &SAblAbilityValidatorResultsDlg::ShouldShowInfo)
								.ToolTipText(LOCTEXT("ShowInfoCheckBoxToolTip", "Toggles whether to display Info logs."))
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("ShowInfoCheckBox", "Show Info"))
								]
							]
							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoWidth()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SAblAbilityValidatorResultsDlg::OnShowWarningsCheckboxChanged)
								.IsChecked(this, &SAblAbilityValidatorResultsDlg::ShouldShowWarnings)
								.ToolTipText(LOCTEXT("ShowWarningsCheckBoxToolTip", "Toggles whether to display Warning logs."))
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("ShowWarningsCheckBox", "Show Warnings"))
								]
							]
							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoWidth()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SAblAbilityValidatorResultsDlg::OnShowErrorsCheckboxChanged)
								.IsChecked(this, &SAblAbilityValidatorResultsDlg::ShouldShowErrors)
								.ToolTipText(LOCTEXT("ShowErrorsCheckBoxToolTip", "Toggles whether to display Error logs."))
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("ShowErrorsCheckBox", "Show Errors"))
								]
							]

						]

						// Ok button
						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Bottom)
							.Padding(8)
							[
								SNew(SUniformGridPanel)
								.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
								.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
								.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
								+ SUniformGridPanel::Slot(0, 0)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
									.OnClicked(this, &SAblAbilityValidatorResultsDlg::OkClicked)
									.Text(LOCTEXT("AblAbilityValidatorResultsOK", "OK"))
								]
							]
				]
			]
		];

	PopulateOutput();
}

void SAblAbilityValidatorResultsDlg::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	PopulateOutput();

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

bool SAblAbilityValidatorResultsDlg::DoModal()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("AblAbilityValidatorResult", "Ability Validator Results"))
		.ClientSize(FVector2D(800, 1000))
		.SupportsMinimize(false)
		.SupportsMaximize(true)
		[
			AsShared()
		];

	m_ModalWindow = Window;

	GEditor->EditorAddModalWindow(Window);

	return true;
}

void SAblAbilityValidatorResultsDlg::PopulateOutput(bool ForceRepopulate)
{
	const UAblAbilityTaskValidatorContext& Context = m_Validator->GetContext();
	const int32 TotalLines = Context.GetTotalOutputLines();

	if (!ForceRepopulate && m_CachedTotalOutputLines == TotalLines)
	{
		return;
	}

	m_LogOutput->ClearChildren();

	const TArray<FAblAbilityValidatorLogLine>& OutputLog = Context.GetOutput();

	for (const FAblAbilityValidatorLogLine& LogLine : OutputLog)
	{
		if (LogLine.LogFlags & m_ShowFlags)
		{
			m_LogOutput->AddSlot()
				[
					SNew(STextBlock)
					.Text(LogLine.Text)
					.ColorAndOpacity(GetColorForFlags(LogLine.LogFlags))
				];
		}
	}

	m_CachedTotalOutputLines = TotalLines;
}

FSlateColor SAblAbilityValidatorResultsDlg::GetColorForFlags(int8 Flags) const
{
	if (Flags & 1 << (uint8)EAblAbilityValidatorLogType::Log)
	{
		return FSlateColor(FLinearColor::Gray);
	}

	if (Flags & 1 << (uint8)EAblAbilityValidatorLogType::Warning)
	{
		return FSlateColor(FLinearColor::Yellow);
	}

	if (Flags & 1 << (uint8)EAblAbilityValidatorLogType::Error)
	{
		return FSlateColor(FLinearColor::Red);
	}

	return FSlateColor(FLinearColor::White);
}

FReply SAblAbilityValidatorResultsDlg::OkClicked()
{
	CloseDialog();
	return FReply::Handled();
}

void SAblAbilityValidatorResultsDlg::CloseDialog()
{
	if (m_ModalWindow.IsValid())
	{
		m_ModalWindow.Pin()->RequestDestroyWindow();
	}
}

ECheckBoxState SAblAbilityValidatorResultsDlg::ShouldShowInfo() const
{
	return (m_ShowFlags & 1 << (int8)EAblAbilityValidatorLogType::Log) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAblAbilityValidatorResultsDlg::OnShowInfoCheckboxChanged(ECheckBoxState NewCheckedState)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		m_ShowFlags ^= (1 << (int8)EAblAbilityValidatorLogType::Log);
	}
	else
	{
		m_ShowFlags &= ~(1 << (int8)EAblAbilityValidatorLogType::Log);
	}

	PopulateOutput(true);
}

ECheckBoxState SAblAbilityValidatorResultsDlg::ShouldShowWarnings() const
{
	return (m_ShowFlags & 1 << (int8)EAblAbilityValidatorLogType::Warning) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAblAbilityValidatorResultsDlg::OnShowWarningsCheckboxChanged(ECheckBoxState NewCheckedState)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		m_ShowFlags ^= (1 << (int8)EAblAbilityValidatorLogType::Warning);
	}
	else
	{
		m_ShowFlags &= ~(1 << (int8)EAblAbilityValidatorLogType::Warning);
	}

	PopulateOutput(true);
}

ECheckBoxState SAblAbilityValidatorResultsDlg::ShouldShowErrors() const
{
	return (m_ShowFlags & 1 << (int8)EAblAbilityValidatorLogType::Error) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAblAbilityValidatorResultsDlg::OnShowErrorsCheckboxChanged(ECheckBoxState NewCheckedState)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		m_ShowFlags ^= (1 << (int8)EAblAbilityValidatorLogType::Error);
	}
	else
	{
		m_ShowFlags &= ~(1 << (int8)EAblAbilityValidatorLogType::Error);
	}

	PopulateOutput(true);
}

#undef LOCTEXT_NAMESPACE