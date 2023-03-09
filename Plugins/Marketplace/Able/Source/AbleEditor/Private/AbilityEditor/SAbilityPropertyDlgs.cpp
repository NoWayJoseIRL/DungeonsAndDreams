
#include "AbilityEditor/SAbilityPropertyDlgs.h"

#include "AbleEditorPrivate.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "IContentBrowserSingleton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblAbilityPropertyDlg::Construct(const FArguments& InArgs)
{
	m_bOkClicked = false;
	m_ModalWindow = nullptr;

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
					.FillHeight(1.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Content()
						[
							GetContent()
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
							.OnClicked(this, &SAblAbilityPropertyDlg::OkClicked)
							.Text(LOCTEXT("AblAbilityPropertyDlgOK", "OK"))
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
							.OnClicked(this, &SAblAbilityPropertyDlg::CancelClicked)
							.Text(LOCTEXT("AblAbilityPropertyDlgCancel", "Cancel"))
						]
					]
				]
			]
		];
}

bool SAblAbilityPropertyDlg::DoModal()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(GetWindowTitle())
		.ClientSize(GetWindowSize())
		.SupportsMinimize(false).SupportsMaximize(false)
		[
			AsShared()
		];

	m_ModalWindow = Window;

	GEditor->EditorAddModalWindow(Window);

	return m_bOkClicked;
}

FReply SAblAbilityPropertyDlg::OkClicked()
{
	m_bOkClicked = true;

	CloseDialog();

	return FReply::Handled();
}

void SAblAbilityPropertyDlg::CloseDialog()
{
	if (m_ModalWindow.IsValid())
	{
		m_ModalWindow.Pin()->RequestDestroyWindow();
	}
}

FReply SAblAbilityPropertyDlg::CancelClicked()
{
	m_bOkClicked = false;

	CloseDialog();

	return FReply::Handled();
}

FReply SAblAbilityPropertyDlg::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		m_bOkClicked = false;

		CloseDialog();

		return FReply::Handled();
	}

	return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SAblAbilityAssetPropertyDlg::Construct(const FArguments& InArgs)
{
	m_DefaultValue = InArgs._DefaultValue;
	m_AssetFilter = InArgs._AssetFilter;
	m_PropertyLabel = InArgs._PropertyLabel;

	SAblAbilityPropertyDlg::Construct(SAblAbilityPropertyDlg::FArguments());
}

TSharedRef<SWidget> SAblAbilityAssetPropertyDlg::GetContent()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// Configure filter for asset picker
	FAssetPickerConfig Config;
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAblAbilityAssetPropertyDlg::OnAssetSelected);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAblAbilityAssetPropertyDlg::OnAssetDoubleClicked);
	Config.InitialAssetSelection = m_DefaultValue;
	Config.bCanShowFolders = true;
	Config.Filter.ClassPaths.Append(m_AssetFilter);
	Config.Filter.bRecursiveClasses = true;

	return ContentBrowserModule.Get().CreateAssetPicker(Config);
}

void SAblAbilityAssetPropertyDlg::OnAssetSelected(const FAssetData& AssetData)
{
	m_Value = AssetData;

	OkClicked();
}

void SAblAbilityAssetPropertyDlg::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	m_Value = AssetData;

	OkClicked();
}

#undef LOCTEXT_NAMESPACE

void SAblAbilityEnumPropertyDlg::Construct(const FArguments& InArgs)
{
	m_EnumClass = InArgs._EnumClass;
	m_PropertyLabel = InArgs._PropertyLabel;
	m_InitialIndex = InArgs._InitialSelectionIndex;

	for (int i = 0; i < m_EnumClass->NumEnums(); ++i)
	{
		m_EnumNames.Add(MakeShared<FString>(m_EnumClass->GetNameStringByIndex(i)));
	}

	SAblAbilityPropertyDlg::Construct(SAblAbilityPropertyDlg::FArguments());
}

TSharedRef<SWidget> SAblAbilityEnumPropertyDlg::GetContent()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNumericEntryBox<int>::BuildLabel(m_PropertyLabel, FLinearColor::White, FLinearColor(0.2f, 0.2f, 0.2f))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.InitiallySelectedItem(MakeShared<FString>(m_EnumClass->GetNameByValue(m_InitialIndex).ToString()))
			.OptionsSource(&m_EnumNames)
			.OnSelectionChanged(this, &SAblAbilityEnumPropertyDlg::OnSelectionChanged)
		];

}

void SAblAbilityEnumPropertyDlg::OnSelectionChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
{
	if (Value.IsValid())
	{
		m_Value = m_EnumClass->GetValueByNameString(*Value);
	}
}
