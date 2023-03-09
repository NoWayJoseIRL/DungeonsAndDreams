#include "AbilityEditor/SAbilityGenericAssetShortcut.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Pawn.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateTypes.h"
#include "Styling/ToolBarStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

namespace GenericAssetShortcutConstants
{
	const int32 ThumbnailSize = 40;
	const int32 ThumbnailSizeSmall = 16;
}

class SAblAssetShortcut : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAssetShortcut)
	{}
	
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<class FWorkflowCentricApplication>& InHostingApp, const TSharedPtr<FAblAssetShortcutParams>& InParams, const TSharedRef<FAssetThumbnailPool>& InThumbnailPool)
	{
		ShortcutParams = InParams;
		HostingApp = InHostingApp;

		AssetDirtyBrush = FAppStyle::Get().GetBrush("Icons.DirtyBadge");

		const FToolBarStyle& ToolBarStyle = FAppStyle::Get().GetWidgetStyle<FToolBarStyle>("ToolBar");

		ChildSlot
			[
				SNew(SHorizontalBox)

		// This is the left half of the button / combo pair for when there are multiple options
		+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SAssignNew(CheckBox, SCheckBox)
				.Style(FAppStyle::Get(), "SegmentedCombo.Left")
				.OnCheckStateChanged(this, &SAblAssetShortcut::HandleAssetButtonClicked)
				.IsChecked(this, &SAblAssetShortcut::GetCheckState)
				.Visibility(this, &SAblAssetShortcut::GetComboButtonVisibility)
				.ToolTipText(this, &SAblAssetShortcut::GetButtonTooltip)
				.Padding(0.0)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(FMargin(16.f, 4.f))
					[
						SNew(SImage)
						.ColorAndOpacity(this, &SAblAssetShortcut::GetAssetColor)
						.Image(this, &SAblAssetShortcut::GetAssetIcon)
					]
				]
			]
		+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SSeparator)
				.Visibility(this, &SAblAssetShortcut::GetComboVisibility)
			.Thickness(1.0)
			.Orientation(EOrientation::Orient_Vertical)
			]
		+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SComboButton)
				.Visibility(this, &SAblAssetShortcut::GetComboVisibility)
			.ContentPadding(FMargin(7.f, 0.f))
			.ForegroundColor(FSlateColor::UseForeground())
			.ComboButtonStyle(&FAppStyle::Get(), "SegmentedCombo.Right")
			.OnGetMenuContent(this, &SAblAssetShortcut::HandleGetMenuContent)
			.ToolTipText(LOCTEXT("AssetComboTooltip", "Find other assets of this type and perform asset operations.\nShift-Click to open in new window."))
			]
			];

		EnableToolTipForceField(true);
	}

	~SAblAssetShortcut()
	{

	}

	const FText& GetAssetName() const
	{
		return ShortcutParams->ShortcutDisplayName;
	}

	const FSlateBrush* GetAssetIcon() const
	{
		return ShortcutParams->ShortcutIconBrush;
	}

	FSlateColor GetAssetColor() const
	{
		return ShortcutParams->ShortcutIconColor;
	}

	ECheckBoxState GetCheckState() const
	{
		return ECheckBoxState::Unchecked;
	}

	FSlateColor GetAssetTextColor() const
	{
		static const FName InvertedForeground("InvertedForeground");
		return GetCheckState() == ECheckBoxState::Checked || CheckBox->IsHovered() ? FAppStyle::GetSlateColor(InvertedForeground) : FSlateColor::UseForeground();
	}

	void HandleAssetButtonClicked(ECheckBoxState InState)
	{
		ShortcutParams->AssetClickCallback.ExecuteIfBound();
	}

	TSharedRef<SWidget> HandleGetMenuContent()
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		const bool bInShouldCloseWindowAfterMenuSelection = true;
		FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);

		//MenuBuilder.BeginSection("AssetActions", LOCTEXT("AssetActionsSection", "Asset Actions"));
		//{
		//	MenuBuilder.AddMenuEntry(
		//		LOCTEXT("ShowInContentBrowser", "Show In Content Browser"),
		//		LOCTEXT("ShowInContentBrowser_ToolTip", "Show this asset in the content browser."),
		//		FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Search"),
		//		FUIAction(FExecuteAction::CreateSP(this, &SAblAssetShortcut::HandleShowInContentBrowser)));
		//}
		//MenuBuilder.EndSection();

		FAssetData InitialData = ShortcutParams->InitialAssetCallback.IsBound() ? ShortcutParams->InitialAssetCallback.Execute() : FAssetData();


		MenuBuilder.BeginSection("AssetSelection", LOCTEXT("AssetSelectionSection", "Select Asset"));
		{
			FAssetPickerConfig AssetPickerConfig;

			AssetPickerConfig.Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
			AssetPickerConfig.Filter.bRecursiveClasses = true;
			AssetPickerConfig.SelectionMode = ESelectionMode::SingleToggle;
			AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAblAssetShortcut::HandleAssetSelectedFromPicker);
			AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SAblAssetShortcut::ShouldFilterAsset);
			AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAblAssetShortcut::HandleAssetSelectedFromPicker);
			AssetPickerConfig.bAllowNullSelection = false;
			AssetPickerConfig.ThumbnailLabel = EThumbnailLabel::ClassName;
			AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
			AssetPickerConfig.InitialAssetSelection = InitialData;

			MenuBuilder.AddWidget(
				SNew(SBox)
				.WidthOverride(300)
				.HeightOverride(600)
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				],
				FText(), true);
		}
		MenuBuilder.EndSection();


		return MenuBuilder.MakeWidget();
	}

	void HandleAssetSelectedFromPicker(const FAssetData& InAssetData)
	{
		ShortcutParams->AssetCallback.ExecuteIfBound(InAssetData);

		FSlateApplication::Get().DismissAllMenus();
	}

	bool ShouldFilterAsset(const FAssetData& InAssetData)
	{
		return !ShortcutParams->IsAssetCompatible(InAssetData);
	}

	EVisibility GetComboButtonVisibility() const
	{
		return EVisibility::Visible;
	}

	EVisibility GetComboVisibility() const
	{
		return EVisibility::Visible;
	}

	FText GetButtonTooltip() const
	{
		return ShortcutParams->ShortcutDisplayName;
	}

private:
	/** The asset family we are working with */
	TSharedPtr<FAblAssetShortcutParams> ShortcutParams;

	/** The asset editor we are embedded in */
	TWeakPtr<class FWorkflowCentricApplication> HostingApp;

	/** Check box */
	TSharedPtr<SCheckBox> CheckBox;

	/** Cached dirty brush */
	const FSlateBrush* AssetDirtyBrush;
};

void SAblAssetFamilyShortcutBar::Construct(const FArguments& InArgs, const TSharedRef<class FWorkflowCentricApplication>& InHostingApp, const TArray<TSharedPtr<FAblAssetShortcutParams>>& InParams)
{
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(16, false));

	TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

	for(const TSharedPtr<FAblAssetShortcutParams>& Param : InParams)
	{
		HorizontalBox->AddSlot()
			.AutoWidth()
			.Padding(0.0f, 4.0f, 16.0f, 4.0f)
			[
				SNew(SAblAssetShortcut, InHostingApp, Param, ThumbnailPool.ToSharedRef())
			];
	}

	ChildSlot
		[
			HorizontalBox
		];
}

FAblAssetShortcutParams::FAblAssetShortcutParams()
: ShortcutDisplayName(LOCTEXT("AblShortcutInvalidName", "INVALID NAME")),
ShortcutIconBrush(),
ShortcutIconColor(FSlateColor::UseForeground()),
RequiredParentClass(nullptr)
{

}

FAblAssetShortcutParams::FAblAssetShortcutParams(const UClass* InRequiredParentClass, const FText& InDisplayName, const FSlateBrush* InBrush, const FSlateColor& InColor)
: ShortcutDisplayName(InDisplayName),
ShortcutIconBrush(InBrush),
ShortcutIconColor(InColor),
RequiredParentClass(InRequiredParentClass)
{

}

FAblAssetShortcutParams::~FAblAssetShortcutParams()
{

}

bool FAblAssetShortcutParams::IsAssetCompatible(const FAssetData& InAssetData) const
{
	if (!RequiredParentClass)
	{
		return false;
	}

	if (InAssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
	{
		if (UBlueprint* LoadedBlueprint = Cast<UBlueprint>(InAssetData.GetAsset()))
		{
			return *(LoadedBlueprint->GeneratedClass) && LoadedBlueprint->GeneratedClass->IsChildOf(RequiredParentClass);
		}
	}
	return false;
}
