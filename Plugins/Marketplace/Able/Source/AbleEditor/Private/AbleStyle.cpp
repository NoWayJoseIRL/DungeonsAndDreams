// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbleStyle.h"

#include "AbleEditorPrivate.h"
#include "Brushes/SlateBorderBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateBoxBrush.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Interfaces/IPluginManager.h"
#include "ClassIconFinder.h"

#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FAbleStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define BOX_PLUGIN_BRUSH( RelativePath, ... ) FSlateBoxBrush( FAbleStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

#define IMAGE_PLUGIN_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(FAbleStyle::InContent(RelativePath, ".svg"), __VA_ARGS__)
#define BOX_PLUGIN_BRUSH_SVG( RelativePath, ... ) FSlateVectorBoxBrush(FAbleStyle::InContent(RelativePath, ".svg"), __VA_ARGS__)
#define BORDER_PLUGIN_BRUSH_SVG( RelativePath, ... ) FSlateVectorBorderBrush(FAbleStyle::InContent(RelativePath, ".svg"), __VA_ARGS__)

#define CORE_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( m_StyleSet->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CORE_BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( m_StyleSet->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CORE_BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( m_StyleSet->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

#define CONTENT_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( m_StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CONTENT_BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( m_StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CONTENT_BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( m_StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

TSharedPtr< FSlateStyleSet > FAbleStyle::m_StyleSet = nullptr;
FName FAbleStyle::m_StyleName(TEXT("Able"));

void FAbleStyle::Initialize()
{
	// Const icon sizes
	const FVector2D Icon8x8(8.0f, 8.0f);
	const FVector2D Icon12x12(12.0f, 12.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon32x32(32.0f, 32.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	// Only register once
	if (m_StyleSet.IsValid())
	{
		return;
	}

	m_StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	m_StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	m_StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	m_StyleSet->Set("AbilityEditor.ModeSeparator", new BOX_PLUGIN_BRUSH("Modes/ModeSeparator", FMargin(15.0f / 16.0f, 22.0f / 24.0f, 1.0f / 16.0f, 1.0f / 24.0f), FLinearColor(1, 1, 1, 0.5f)));
	m_StyleSet->Set("Able.AbilityEditor.Timeline", new BOX_PLUGIN_BRUSH("Brushes/TimeLine_Brush", FMargin(4.0f / 16.0f)));
	m_StyleSet->Set("Able.AbilityEditor.TimelineStatus", new BOX_PLUGIN_BRUSH("Brushes/StatusBackground", FMargin(4.0f / 16.0f)));
	m_StyleSet->Set("Able.AbilityEditor.Node", new BOX_PLUGIN_BRUSH("Brushes/LayerColor_Brush", FMargin(4.0f / 16.0f)));
	m_StyleSet->Set("Able.AbilityEditor.NodeHighlight", new BOX_PLUGIN_BRUSH("Brushes/LayerColor_HL_Brush", FMargin(4.0f / 16.0f)));
	m_StyleSet->Set("Able.AbilityEditor.NodeText", new BOX_PLUGIN_BRUSH("Brushes/TrackBackground", FMargin(4.0f / 16.0f)));
	m_StyleSet->Set("Able.AbilityEditor.TimelimeStatus.Marker", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/StatusBarMarker"), Icon12x12));
	m_StyleSet->Set("Able.AbilityEditor.Timeline.Dependency", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/Dependency_12"), Icon12x12));
	m_StyleSet->Set("Able.AbilityEditor.Timeline.Lock", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/Lock_32"), Icon32x32));
	m_StyleSet->Set("Able.AbilityEditor.Track", new BOX_PLUGIN_BRUSH("Brushes/LayerBar_Brush", FMargin(4.0f / 16.0f)));
	m_StyleSet->Set("Able.AbilityEditor.TrackAlt", new BOX_PLUGIN_BRUSH("Brushes/LayerAltBar_Brush", FMargin(4.0f / 16.0f)));
	m_StyleSet->Set("Able.AbilityEditor.ScrollBackground", new BOX_PLUGIN_BRUSH("Brushes/BackGround_Brush", FMargin(4.0f / 16.0f)));

	m_StyleSet->Set("ClassIcon.AblAbility", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Gear"), Icon16x16));
	m_StyleSet->Set("ClassIcon.AblAbilityBlueprint", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Gear"), Icon16x16));
	m_StyleSet->Set("ClassIcon.AblCustomTask", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CustomTask_40"), Icon40x40));
	m_StyleSet->Set("ClassIcon.AblCustomTask.Small", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CustomTask_16"), Icon16x16));
	m_StyleSet->Set("ClassIcon.AblCustomTaskScratchPad", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CustomScratchPad_40"), Icon40x40));
	m_StyleSet->Set("ClassIcon.AblCustomTaskScratchPad.Small", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CustomScratchPad_16"), Icon16x16));
	m_StyleSet->Set("ClassThumbnail.AblAbility", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Gear"), Icon16x16));
	m_StyleSet->Set("ClassThumbnail.AblAbilityBlueprint", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Gear"), Icon16x16));
	m_StyleSet->Set("ClassThumbnail.AblCustomTask", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CustomTask_40"), Icon40x40));
	m_StyleSet->Set("ClassThumbnail.AblCustomTaskScratchPad", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CustomScratchPad_40"), Icon40x40));

	// Commands
	m_StyleSet->Set("AblAbilityEditor.m_AddTask", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Add"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_DuplicateTask", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Add"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_RemoveTask", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/RemoveTask_40"), Icon40x40));
	m_StyleSet->Set("AblAbilityEditor.m_Validate", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Check"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_SetPreviewAsset", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/SetPreviewAsset_40"), Icon40x40));
	m_StyleSet->Set("AblAbilityEditor.m_SetPreviewAsset.Small", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/SetPreviewAsset_40"), Icon20x20));
	m_StyleSet->Set("AblAbilityEditor.m_PlayAbility", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Play"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_StopAbility", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Stop"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_PauseAbility", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Pause"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_StepAbility", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/StepForwards"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_StepAbilityBackwards", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/StepBackwards"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_ResetPreviewAsset", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Refresh"), Icon16x16));
	//m_StyleSet->Set("AblAbilityEditor.m_ToggleCost", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/ToggleCost_40"), Icon40x40));
	//m_StyleSet->Set("AblAbilityEditor.m_ToggleCost.Small", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/ToggleCost_40"), Icon20x20));
	m_StyleSet->Set("AblAbilityEditor.m_Resize", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Scale"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_CaptureThumbnail", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CaptureThumbnail_40"), Icon40x40));
	m_StyleSet->Set("AblAbilityEditor.m_CaptureThumbnail.Small", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/CaptureThumbnail_40"), Icon20x20));
	m_StyleSet->Set("AblAbilityEditor.ResetButton", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Refresh"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.AddButton", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Add"), Icon16x16));
	// Tab Styles
	m_StyleSet->Set("Able.Tabs.AbilityTimeline", new IMAGE_PLUGIN_BRUSH_SVG(TEXT("Icons/Gear"), Icon16x16));
	m_StyleSet->Set("Able.Tabs.AbilityEditorSettings", new IMAGE_PLUGIN_BRUSH("Icons/EditorSettings_16", Icon16x16));
	m_StyleSet->Set("Able.Tabs.AbilityAssetDetails", new IMAGE_PLUGIN_BRUSH("Icons/AbilityProperties_16", Icon16x16));
	m_StyleSet->Set("Able.Tabs.AbilityTaskAssetDetails", new IMAGE_PLUGIN_BRUSH("Icons/TaskProperties_16", Icon16x16));

	// Mode Icons
	m_StyleSet->Set("AblAbilityEditor.TimelineMode", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/TimelineMode_40"), Icon40x40));
	m_StyleSet->Set("AblAbilityEditor.TimelineMode.Small", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/TimelineMode_16"), Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.GraphMode", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/GraphMode_40"), Icon40x40));
	m_StyleSet->Set("AblAbilityEditor.GraphMode.Small", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/GraphMode_16"), Icon16x16));

	// Images
	m_StyleSet->Set("AblAbilityEditor.PreviewAssetImage", new IMAGE_PLUGIN_BRUSH(TEXT("Images/PreviewActor"), FVector2D(256.0f, 256.0f)));

	// Generic commands
	m_StyleSet->Set("AblAbilityEditor.m_CopyTask", new CORE_IMAGE_BRUSH("Icons/Edit/icon_Edit_Copy_16x", Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_PasteTask", new CORE_IMAGE_BRUSH("Icons/Edit/icon_Edit_Paste_16x", Icon16x16));

	// Community commands.
	m_StyleSet->Set("AblAbilityEditor.m_TutorialWebLink", new CONTENT_IMAGE_BRUSH("Icons/Help/icon_Help_Documentation_16x", Icon16x16));
	m_StyleSet->Set("AblAbilityEditor.m_DiscordWebLink", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/Discord_16"), Icon16x16));

	// Text
	const FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(DEFAULT_FONT("Regular", 8))
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
		.SetHighlightShape(CORE_BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f / 8.f)));

	const FTextBlockStyle NormalRichTextStyle = FTextBlockStyle(NormalText)
		.SetFont(DEFAULT_FONT("Regular", 8))
		.SetColorAndOpacity(FSlateColor::UseForeground());

	m_StyleSet->Set("RichText.Text", NormalRichTextStyle);

	const FButtonStyle DarkHyperlinkButton = FButtonStyle()
		.SetNormal(CORE_BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), FSlateColor::UseForeground()))
		.SetPressed(FSlateNoResource())
		.SetHovered(CORE_BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), FSlateColor::UseForeground()));

	const FHyperlinkStyle DarkHyperlink = FHyperlinkStyle()
		.SetUnderlineStyle(DarkHyperlinkButton)
		.SetTextStyle(NormalRichTextStyle)
		.SetPadding(FMargin(0.0f));

	m_StyleSet->Set("RichText.Hyperlink", DarkHyperlink);

	FSlateStyleRegistry::RegisterSlateStyle(*m_StyleSet.Get());
}

void FAbleStyle::Shutdown()
{
	if (m_StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*m_StyleSet.Get());
		ensure(m_StyleSet.IsUnique());
		m_StyleSet.Reset();
	}
}

TSharedPtr< class ISlateStyle > FAbleStyle::Get()
{
	return m_StyleSet;
}

const FSlateBrush* FAbleStyle::GetBrush(FName BrushName)
{
	if (m_StyleSet.IsValid())
	{
		return m_StyleSet->GetBrush(BrushName);
	}

	return nullptr;
}

FString FAbleStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("Able"))->GetContentDir();
	return (ContentDir / RelativePath) + Extension;
}

#undef IMAGE_PLUGIN_BRUSH