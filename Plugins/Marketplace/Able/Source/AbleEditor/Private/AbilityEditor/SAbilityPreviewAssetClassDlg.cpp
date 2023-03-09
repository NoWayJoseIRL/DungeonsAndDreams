// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityPreviewAssetClassDlg.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbleStyle.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Pawn.h"
#include "IContentBrowserSingleton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblAbilityPreviewAssetClassDlg::Construct(const FArguments& InArgs)
{
	m_bOkClicked = false;
	m_ModalWindow = nullptr;
	m_Class = nullptr;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;

	AssetPickerConfig.Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.bRecursiveClasses = true;
	AssetPickerConfig.SelectionMode = ESelectionMode::SingleToggle;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAblAbilityPreviewAssetClassDlg::OnAssetSelected);
	AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SAblAbilityPreviewAssetClassDlg::ShouldFilterAsset);
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
					.OnClicked(this, &SAblAbilityPreviewAssetClassDlg::OkClicked)
					.Text(LOCTEXT("AblAbilityAnimationSelectorOK", "OK"))
				]
			+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
					.OnClicked(this, &SAblAbilityPreviewAssetClassDlg::CancelClicked)
					.Text(LOCTEXT("AblAbilityAnimationSelectorCancel", "Cancel"))
				]
			]
		]
		]
		];

	//MakePicker();
}

bool SAblAbilityPreviewAssetClassDlg::DoModal()
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

void SAblAbilityPreviewAssetClassDlg::MakePicker()
{
	//FClassViewerModule& ClassViewerModule = FModuleManager::Get().LoadModuleChecked<FClassViewerModule>(TEXT("ClassViewer"));

	//// Configure filter for asset picker
	//FClassViewerInitializationOptions Config;
	//Config.DisplayMode = EClassViewerDisplayMode::TreeView;

	//m_ClassPickerContainer->AddSlot()
	//	.AutoHeight()
	//	[
	//		SNew(STextBlock)
	//		.Text(LOCTEXT("SelectAnimationAsset", "Select Asset:"))
	//	.ShadowOffset(FVector2D(1.0f, 1.0f))
	//	];
	//		m_ClassPickerContainer->AddSlot()
	//	[
	//		ClassViewerModule.CreateClassViewer(Config, FOnClassPicked::CreateSP(this, &SAblAbilityPreviewAssetClassDlg::OnClassSelected))
	//	];
}

void SAblAbilityPreviewAssetClassDlg::OnClassSelected(UClass* InClass)
{
	m_Class = InClass;
}

void SAblAbilityPreviewAssetClassDlg::OnAssetSelected(const FAssetData& Asset)
{
	m_AssetSelected = Asset;
}

bool SAblAbilityPreviewAssetClassDlg::ShouldFilterAsset(const FAssetData& Asset) const
{
	if (Asset.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
	{
		if (UBlueprint* LoadedBlueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			return !(*(LoadedBlueprint->GeneratedClass) && LoadedBlueprint->GeneratedClass->IsChildOf(APawn::StaticClass()));
		}
	}
	
	return true;
}

FReply SAblAbilityPreviewAssetClassDlg::OkClicked()
{
	m_bOkClicked = true;

	CloseDialog();

	return FReply::Handled();
}

void SAblAbilityPreviewAssetClassDlg::CloseDialog()
{
	if (m_ModalWindow.IsValid())
	{
		m_ModalWindow.Pin()->RequestDestroyWindow();
	}
}

FReply SAblAbilityPreviewAssetClassDlg::CancelClicked()
{
	m_bOkClicked = false;

	CloseDialog();

	return FReply::Handled();
}

FReply SAblAbilityPreviewAssetClassDlg::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
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