// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityEditorToolbar.h"

#include "AbleEditorPrivate.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/ablAbilityEditorCommands.h"
#include "AbilityEditor/ablAbilityEditorModes.h"
#include "AbilityEditor/SAbilityGenericAssetShortcut.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "Misc/MessageDialog.h"

#include "AbleStyle.h"

#include "Editor/Kismet/Public/BlueprintEditor.h"
#include "WorkflowOrientedApp/SModeWidget.h"
#include "WorkflowOrientedApp/SContentReference.h"

#include "IDocumentation.h"
#include "ClassIconFinder.h"
#include "ContentBrowserModule.h"
#include "IAssetFamily.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Application/SlateApplication.h"

#include "Engine/AssetManager.h"

#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

#include "Styling/SlateTypes.h"
#include "Styling/ToolBarStyle.h"

#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

class SModeSeparator : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SModeSeparator) {}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArg)
	{
		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(FAbleStyle::GetBrush("AbilityEditor.ModeSeparator"))
			.Padding(0.0f)
		);
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float Height = 24.0f;
		const float Thickness = 16.0f;
		return FVector2D(Thickness, Height);
	}
	// End of SWidget interface
};

FAblAbilityEditorToolbar::FAblAbilityEditorToolbar(TSharedPtr<FAblAbilityEditor> InAbilityEditor)
: m_AbilityEditor(InAbilityEditor.ToSharedRef())
{

}

void FAblAbilityEditorToolbar::SetupToolbar(TSharedPtr<FExtender> Extender)
{
	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		m_AbilityEditor.Pin()->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FAblAbilityEditorToolbar::FillAbilityEditorModeToolbar));

	
	m_PlayIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), FName(TEXT("AblAbilityEditor.m_PlayAbility")));
	m_PauseIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), FName(TEXT("AblAbilityEditor.m_PauseAbility")));

	m_PreviewThumbnailPool = MakeShareable(new FAssetThumbnailPool(16, false));
	m_TargetThumbnailPool = MakeShareable(new FAssetThumbnailPool(16, false));
}

bool FAblAbilityEditorToolbar::IsAssetValid(const FAssetData& Asset) const
{
	if (Asset.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
	{
		if (UBlueprint* LoadedBlueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			return (*(LoadedBlueprint->GeneratedClass) && LoadedBlueprint->GeneratedClass->IsChildOf(APawn::StaticClass()));
		}
	}

	return false;
}

void FAblAbilityEditorToolbar::FillAbilityEditorModeToolbar(FToolBarBuilder& ToolbarBuilder)
{
	UBlueprint* BlueprintObj = m_AbilityEditor.Pin()->GetBlueprintObj();

	const float ContentRefWidth = 80.0f;

	FToolBarBuilder Builder(ToolbarBuilder.GetTopCommandList(), ToolbarBuilder.GetCustomization());

	{
		TSharedPtr<FAblAbilityEditor> AbilityEditor = m_AbilityEditor.Pin();
		TSharedRef<FAblAbilityEditor> AbilityEditorRef = AbilityEditor.ToSharedRef();
		TAttribute<FName> GetActiveMode(AbilityEditorRef, &FAblAbilityEditor::GetCurrentMode);
		FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(AbilityEditorRef, &FAblAbilityEditor::SetCurrentMode);
		// Left side padding
		AbilityEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

		AbilityEditor->AddToolbarWidget(
			SNew(SModeWidget, FAblAbilityEditorModes::GetLocalizedMode(FAblAbilityEditorModes::AbilityTimelineMode), FAblAbilityEditorModes::AbilityTimelineMode)
			.OnGetActiveMode(GetActiveMode)
			.OnSetActiveMode(SetActiveMode)
			.IconImage(FAbleStyle::GetBrush("AblAbilityEditor.TimelineMode"))
			//.SmallIconImage(FAbleStyle::GetBrush("AblAbilityEditor.TimelineMode.Small"))
			.DirtyMarkerBrush(AbilityEditorRef, &FAblAbilityEditor::GetDirtyImageForMode, FAblAbilityEditorModes::AbilityTimelineMode)
			.ToolTip(IDocumentation::Get()->CreateToolTip(
				LOCTEXT("AbilityTimelineButtonTooltip", "Switch to timeline editing mode"),
				NULL,
				TEXT("Shared/Plugin/Able"),
				TEXT("TimelineMode")))
			.AddMetaData<FTagMetaData>(TEXT("Able.Timeline"))
			.ShortContents()
			[
				SNew(SContentReference)
				.AssetReference(AbilityEditorRef, &FAblAbilityEditor::GetAbilityBlueprintAsObject)
			.AllowSelectingNewAsset(false)
			.AllowClearingReference(false)
			.WidthOverride(ContentRefWidth)
			]
		);

		AbilityEditor->AddToolbarWidget(SNew(SModeSeparator));

		AbilityEditor->AddToolbarWidget(
			SNew(SModeWidget, FAblAbilityEditorModes::GetLocalizedMode(FAblAbilityEditorModes::AbilityBlueprintMode), FAblAbilityEditorModes::AbilityBlueprintMode)
			.OnGetActiveMode(GetActiveMode)
			.OnSetActiveMode(SetActiveMode)
			.IconImage(FAbleStyle::GetBrush("AblAbilityEditor.GraphMode"))
			//.SmallIconImage(FAbleStyle::GetBrush("AblAbilityEditor.GraphMode.Small"))
			.DirtyMarkerBrush(AbilityEditorRef, &FAblAbilityEditor::GetDirtyImageForMode, FAblAbilityEditorModes::AbilityBlueprintMode)
			.ToolTip(IDocumentation::Get()->CreateToolTip(
				LOCTEXT("AbilityBlueprintButtonTooltip", "Switch to blueprint editing mode"),
				NULL,
				TEXT("Shared/Plugin/Able"),
				TEXT("BlueprintMode")))
			.AddMetaData<FTagMetaData>(TEXT("Able.Blueprint"))
			.ShortContents()
			[
				SNew(SContentReference)
				.AssetReference(AbilityEditorRef, &FAblAbilityEditor::GetAbilityBlueprintAsObject)
			.AllowSelectingNewAsset(false)
			.AllowClearingReference(false)
			.WidthOverride(ContentRefWidth)
			]
		);

		// Right side padding
		AbilityEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));
	}
}

void FAblAbilityEditorToolbar::AddTimelineToolbar(TSharedPtr<FExtender> Extender)
{
	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		m_AbilityEditor.Pin()->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FAblAbilityEditorToolbar::FillTimelineModeToolbar));
}

void FAblAbilityEditorToolbar::OnPreviewAssetSelected(const FAssetData& Asset)
{
	if (!IsAssetValid(Asset))
	{
		FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, LOCTEXT("InvalidAssetErrorMessage", "The Asset choosen isn't an Actor. Please choose an appropriate actor."));

		return;
	}

	m_AbilityEditor.Pin()->SetPreviewAsset(Asset);
}

void FAblAbilityEditorToolbar::OnTargetAssetSelected(const FAssetData& Asset )
{
	if (!IsAssetValid(Asset))
	{
		FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, LOCTEXT("InvalidAssetErrorMessage", "The Asset choosen isn't an Actor. Please choose an appropriate actor."));

		return;
	}

	m_AbilityEditor.Pin()->SetTargetAsset(Asset);
}

FAssetData FAblAbilityEditorToolbar::OnGetPreviewAsset() const
{
	return FAssetData(m_AbilityEditorSettings->m_PreviewAsset.TryLoad());
}

FAssetData FAblAbilityEditorToolbar::OnGetTargetAsset() const
{
	return FAssetData(m_AbilityEditorSettings->m_TargetAsset.TryLoad());
}

TSharedRef<SWidget> FAblAbilityEditorToolbar::OnAddPreviewAssetWidgets()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Toolbar.Button")
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			//.OnClicked(this, &FAblAbilityEditorToolbar::OnResetPreviewAsset)
			.ToolTipText(LOCTEXT("PreviewResetTooltip", "Reset the Preview actor to it's last position before an Ability was executed."))
			[
				SNew(SImage).Image(FAbleStyle::Get()->GetBrush("AblAbilityEditor.ResetButton"))
			]
		];
}

TSharedRef<SWidget> FAblAbilityEditorToolbar::OnAddTargetAssetWidgets()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Toolbar.Button")
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			//.OnClicked(this, &FAblAbilityEditorToolbar::OnResetTargetAsset)
			.ToolTipText(LOCTEXT("TargetResetTooltip", "Reset the Target actor(s) to their last position before an Ability was executed."))
			[
				SNew(SImage).Image(FAbleStyle::Get()->GetBrush("AblAbilityEditor.ResetButton"))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Toolbar.Button")
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &FAblAbilityEditorToolbar::OnAddTargetAsset)
			.ToolTipText(LOCTEXT("TargetAddTooltip", "Add a Target actor to the scene."))
			[
				SNew(SImage).Image(FAbleStyle::Get()->GetBrush("AblAbilityEditor.AddButton"))
			]
		];
}

TSharedRef<SWidget> FAblAbilityEditorToolbar::GenerateForceTargetComboBox()
{
	const TArray<TWeakObjectPtr<AActor>>& allTargets = m_AbilityEditor.Pin()->GetAbilityPreviewTargets();

	FMenuBuilder TargetMenu(true, nullptr);
	TargetMenu.BeginSection(NAME_None, LOCTEXT("TargetMenuLabel", "Force Target"));

	TArray<FText> entryLabels;
	TArray<FText> entryTooltips;

	entryLabels.Add(LOCTEXT("TargetSelectNoneLabel", "None"));
	entryTooltips.Add(LOCTEXT("TargetSelectNoneTooltip", "Does not override the Ability Target. (Default)"));

	FText genericToolTip = LOCTEXT("TargetSelectooltip", "Forces the Ability to set {0} as the Target.");
	for (const TWeakObjectPtr<AActor>& Target : allTargets)
	{
		if (Target.IsValid())
		{
			FText targetName = FText::FromString(Target->GetName());
			entryLabels.Add(targetName);
			entryTooltips.Add(FText::FormatOrdered(genericToolTip, targetName));
		}
	}

	for (int i = 0; i < entryLabels.Num(); ++i)
	{
		TargetMenu.AddMenuEntry(entryLabels[i], entryTooltips[i], FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &FAblAbilityEditorToolbar::SetForceTarget, i)));
	}

	return TargetMenu.MakeWidget();
}

void FAblAbilityEditorToolbar::SetForceTarget(int index)
{
	if (index == 0)
	{
		m_AbilityEditor.Pin()->ClearForceTarget();
	}
	else
	{
		m_AbilityEditor.Pin()->SetForceTarget(index - 1);
	}
}

void FAblAbilityEditorToolbar::OnResetPreviewAsset()
{
	m_AbilityEditor.Pin()->ResetPreviewActor();
}

void FAblAbilityEditorToolbar::OnResetTargetAsset()
{
	m_AbilityEditor.Pin()->ResetTargetActors();
}

FReply FAblAbilityEditorToolbar::OnAddTargetAsset()
{
	m_AbilityEditor.Pin()->AddAbilityPreviewTarget();

	return FReply::Handled();
}

void FAblAbilityEditorToolbar::FillTimelineModeToolbar(FToolBarBuilder& ToolbarBuilder)
{
		const FAblAbilityEditorCommands& Commands = FAblAbilityEditorCommands::Get();
		
		ToolbarBuilder.BeginSection("Thumbnail");
		ToolbarBuilder.AddToolBarButton(Commands.m_CaptureThumbnail);
		ToolbarBuilder.EndSection();

		ToolbarBuilder.BeginSection("Timeline");
		ToolbarBuilder.AddToolBarButton(Commands.m_AddTask);
		ToolbarBuilder.AddToolBarButton(Commands.m_StepAbilityBackwards);
		ToolbarBuilder.AddToolBarButton(Commands.m_PlayAbility,
			NAME_None,
			TAttribute<FText>(this, &FAblAbilityEditorToolbar::GetPlayText),
			TAttribute<FText>(this, &FAblAbilityEditorToolbar::GetPlayToolTip),
			TAttribute<FSlateIcon>(this, &FAblAbilityEditorToolbar::GetPlayIcon));
		ToolbarBuilder.AddToolBarButton(Commands.m_StepAbility);
		ToolbarBuilder.AddToolBarButton(Commands.m_StopAbility);
		ToolbarBuilder.AddToolBarButton(Commands.m_Resize);
		ToolbarBuilder.AddToolBarButton(Commands.m_Validate);
		//ToolbarBuilder.AddToolBarButton(Commands.m_ToggleCost); // TODO: Reimplement this.

		UAssetManager& AssetManager = UAssetManager::Get();
		UAblAbilityEditorSettings& EditorSettings = m_AbilityEditor.Pin()->GetEditorSettings();


		if (EditorSettings.m_PreviewAsset.IsValid())
		{
			FAssetData PreviewAssetData;
			AssetManager.GetAssetDataForPath(EditorSettings.m_PreviewAsset, PreviewAssetData);
		}

		const FSlateBrush* DefaultPreviewBrush = FAppStyle::Get().GetBrush("Persona.AssetClass.Skeleton");
		FSlateColor DefaultPreviewIconColor = FSlateColor(FColor::Green);

		TSharedPtr<FAblAssetShortcutParams> PreviewAssetFamily = MakeShareable(new FAblAssetShortcutParams(APawn::StaticClass(), LOCTEXT("PreviewAssetButton", "Preview Asset\n\nClick to reset the actor in the Editor."), DefaultPreviewBrush, DefaultPreviewIconColor));
		PreviewAssetFamily->AssetCallback.BindSP(this, &FAblAbilityEditorToolbar::OnPreviewAssetSelected);
		PreviewAssetFamily->AssetClickCallback.BindSP(this, &FAblAbilityEditorToolbar::OnResetPreviewAsset);
		
		TSharedPtr<FAblAssetShortcutParams> TargetAssetFamily = MakeShareable(new FAblAssetShortcutParams(APawn::StaticClass(), LOCTEXT("TargetAssetButton", "Target Asset\n\nClick to reset the actor in the Editor."), DefaultPreviewBrush, DefaultPreviewIconColor));
		TargetAssetFamily->AssetCallback.BindSP(this, &FAblAbilityEditorToolbar::OnTargetAssetSelected);
		PreviewAssetFamily->AssetClickCallback.BindSP(this, &FAblAbilityEditorToolbar::OnResetTargetAsset);

		TArray<TSharedPtr<FAblAssetShortcutParams>> Shortcuts;
		Shortcuts.Add(PreviewAssetFamily);
		Shortcuts.Add(TargetAssetFamily);
		
		TSharedRef<SAblAssetFamilyShortcutBar> ShortcutBar = SNew(SAblAssetFamilyShortcutBar, m_AbilityEditor.Pin().ToSharedRef(), Shortcuts);

		ToolbarBuilder.AddWidget(ShortcutBar);

		ToolbarBuilder.AddComboButton(FUIAction(),
			FOnGetContent::CreateSP(this, &FAblAbilityEditorToolbar::GenerateForceTargetComboBox),
			LOCTEXT("ForceTargetLabel", "Override Target"),
			LOCTEXT("ForceTargetTooltip", "Forces the Ability to set the provided actor as the target."));

		ToolbarBuilder.EndSection();

}

FSlateIcon FAblAbilityEditorToolbar::GetPlayIcon() const
{
	if (m_AbilityEditor.Pin()->IsPlayingAbility() && !m_AbilityEditor.Pin()->IsPaused())
	{
		return m_PauseIcon;
	}

	return m_PlayIcon;
}

FText FAblAbilityEditorToolbar::GetPlayText() const
{
	if (m_AbilityEditor.Pin()->IsPlayingAbility() && !m_AbilityEditor.Pin()->IsPaused())
	{
		return LOCTEXT("PauseCommand", "Pause");
	}

	return LOCTEXT("PlayCommand", "Play");
}

FText FAblAbilityEditorToolbar::GetPlayToolTip() const
{
	if (m_AbilityEditor.Pin()->IsPlayingAbility() && !m_AbilityEditor.Pin()->IsPaused())
	{
		return LOCTEXT("PauseCommandToolTip", "Pause the currently playing Ability.");
	}

	return LOCTEXT("PlayCommandToolTip", "Play the current Ability.");
}

#undef LOCTEXT_NAMESPACE