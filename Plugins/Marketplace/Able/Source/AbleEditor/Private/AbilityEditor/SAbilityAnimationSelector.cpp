// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityAnimationSelector.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "IContentBrowserSingleton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblAbilityAnimationSelector::Construct(const FArguments& InArgs)
{
	m_AnimationAsset = nullptr;
	m_bOkClicked = false;
	m_bResizeAbilityLength = true;

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
							SAssignNew(m_AssetPickerContainer, SVerticalBox)
						]
					]
				+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Bottom)
					.Padding(4)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &SAblAbilityAnimationSelector::OnResizeAbilityTimelineCheckboxChanged)
						.IsChecked(this, &SAblAbilityAnimationSelector::ShouldResizeAbilityTimeline)
						.ToolTipText(LOCTEXT("ResizeAbilityTimelineCheckBoxToolTip", "Toggles whether to resize the Ability timeline to the length of the Animation."))
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ResizeAbilityTimelineCheckBox", "Resize Ability Timeline"))
						]
					]

				// Ok/Cancel buttons
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
						.OnClicked(this, &SAblAbilityAnimationSelector::OkClicked)
						.Text(LOCTEXT("AblAbilityAnimationSelectorOK", "OK"))
						]
					+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
						.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SAblAbilityAnimationSelector::CancelClicked)
						.Text(LOCTEXT("AblAbilityAnimationSelectorCancel", "Cancel"))
						]
					]
				]
			]
		];

	MakeAssetPicker();
}



bool SAblAbilityAnimationSelector::DoModal()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("AblAbilityAnimationAssetSelector", "Animation Asset Selector"))
		.ClientSize(FVector2D(400, 700))
		.SupportsMinimize(false).SupportsMaximize(false)
		[
			AsShared()
		];

	m_PickerWindow = Window;

	GEditor->EditorAddModalWindow(Window);

	return m_bOkClicked;
}

void SAblAbilityAnimationSelector::MakeAssetPicker()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// Configure filter for asset picker
	FAssetPickerConfig Config;
	Config.OnAssetSelected = FOnAssetDoubleClicked::CreateSP(this, &SAblAbilityAnimationSelector::OnAssetSelected);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAblAbilityAnimationSelector::OnAssetDoubleClicked);
	Config.Filter.ClassPaths.Add(UAnimMontage::StaticClass()->GetClassPathName());
	Config.Filter.ClassPaths.Add(UAnimSequenceBase::StaticClass()->GetClassPathName());
	Config.Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
	Config.bCanShowFolders = true;

	m_AssetPickerContainer->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectAnimationAsset", "Select Animation Asset:"))
		.ShadowOffset(FVector2D(1.0f, 1.0f))
		];
	m_AssetPickerContainer->AddSlot()
		[
			ContentBrowserModule.Get().CreateAssetPicker(Config)
		];
	
}

void SAblAbilityAnimationSelector::OnAssetSelected(const FAssetData& AssetData)
{
	m_AnimationAsset = CastChecked<UAnimationAsset>(AssetData.GetAsset());
}

void SAblAbilityAnimationSelector::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	m_AnimationAsset = CastChecked<UAnimationAsset>(AssetData.GetAsset());
	m_bOkClicked = true;
	CloseDialog();
}

FReply SAblAbilityAnimationSelector::OkClicked()
{
	m_bOkClicked = true;

	CloseDialog();

	return FReply::Handled();
}

void SAblAbilityAnimationSelector::CloseDialog()
{
	if (m_PickerWindow.IsValid())
	{
		m_PickerWindow.Pin()->RequestDestroyWindow();
	}
}

FReply SAblAbilityAnimationSelector::CancelClicked()
{
	m_bOkClicked = false;
	CloseDialog();
	return FReply::Handled();
}

FReply SAblAbilityAnimationSelector::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		m_bOkClicked = false;
		CloseDialog();
		return FReply::Handled();
	}
	return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

ECheckBoxState SAblAbilityAnimationSelector::ShouldResizeAbilityTimeline() const
{
	return m_bResizeAbilityLength ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAblAbilityAnimationSelector::OnResizeAbilityTimelineCheckboxChanged(ECheckBoxState NewCheckedState)
{
	m_bResizeAbilityLength = (NewCheckedState == ECheckBoxState::Checked);
}

#undef LOCTEXT_NAMESPACE


