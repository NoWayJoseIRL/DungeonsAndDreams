// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilitySelectPreviewAssetDlg.h"

#include "AbleEditorPrivate.h"
#include "AbleStyle.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbilityEditor/SAbilityPreviewAssetClassDlg.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimBlueprint.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Pawn.h"
#include "IContentBrowserSingleton.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "IContentBrowserSingleton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"


void SAblAbilitySelectPreviewAssetDlg::Construct(const FArguments& InArgs)
{
	m_bOkClicked = false;
	m_ModalWindow = nullptr;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;

	AssetPickerConfig.Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.bRecursiveClasses = true;
	AssetPickerConfig.SelectionMode = ESelectionMode::SingleToggle;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAblAbilitySelectPreviewAssetDlg::OnAssetSelected);
	AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SAblAbilitySelectPreviewAssetDlg::OnShouldFilterAsset);
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAblAbilitySelectPreviewAssetDlg::OnAssetDoubleClicked);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.ThumbnailLabel = EThumbnailLabel::ClassName;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.InitialAssetSelection = FAssetData();

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
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					.Padding(4.0f, 4.0f)
					[
						SNew(SImage).Image(FAbleStyle::GetBrush("AblAbilityEditor.PreviewAssetImage"))
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f, 4.0f)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(LOCTEXT("AblAbilitySelectPreviewAsset", "The Preview Asset is used as the actor to execute your Ability while in the editor.\n\nChoose a Blueprint Asset to use, the asset must at least inherit from APawn."))
					]

				+ SVerticalBox::Slot()
					.FillHeight(1)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Content()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("SelectPreviewAsset", "Select Asset:"))
								.ShadowOffset(FVector2D(1.0f, 1.0f))
							]
						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
							]
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
								.OnClicked(this, &SAblAbilitySelectPreviewAssetDlg::OkClicked)
								.Text(LOCTEXT("AblAbilityAnimationSelectorOK", "OK"))
							]
						+ SUniformGridPanel::Slot(1, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
								.OnClicked(this, &SAblAbilitySelectPreviewAssetDlg::CancelClicked)
								.Text(LOCTEXT("AblAbilityAnimationSelectorCancel", "Cancel"))
							]
						]
				]
			]
		];
}

bool SAblAbilitySelectPreviewAssetDlg::DoModal()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("AblAbilityPreviewAssetPicker", "Select Asset"))
		.ClientSize(FVector2D(400, 700))
		.SupportsMinimize(false).SupportsMaximize(false)
		[
			AsShared()
		];

	m_ModalWindow = Window;

	GEditor->EditorAddModalWindow(Window);

	return m_bOkClicked;
}

void SAblAbilitySelectPreviewAssetDlg::OnAssetSelected(const FAssetData& AssetData)
{
	m_AssetSelected = AssetData.GetAsset();
}

void SAblAbilitySelectPreviewAssetDlg::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	m_AssetSelected = AssetData.GetAsset();
	m_bOkClicked = true;
	CloseDialog();
}

bool SAblAbilitySelectPreviewAssetDlg::OnShouldFilterAsset(const FAssetData& AssetData) const
{
	if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
	{
		if (UBlueprint* LoadedBlueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			return !(*(LoadedBlueprint->GeneratedClass) && LoadedBlueprint->GeneratedClass->IsChildOf(APawn::StaticClass()));
		}
	}

	return true;
}

FReply SAblAbilitySelectPreviewAssetDlg::OkClicked()
{
	m_bOkClicked = true;

	CloseDialog();

	return FReply::Handled();
}

void SAblAbilitySelectPreviewAssetDlg::CloseDialog()
{
	if (m_ModalWindow.IsValid())
	{
		m_ModalWindow.Pin()->RequestDestroyWindow();
	}
}

FReply SAblAbilitySelectPreviewAssetDlg::CancelClicked()
{
	m_bOkClicked = false;

	CloseDialog();

	return FReply::Handled();
}

FReply SAblAbilitySelectPreviewAssetDlg::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		m_bOkClicked = false;

		CloseDialog();

		return FReply::Handled();
	}

	return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE