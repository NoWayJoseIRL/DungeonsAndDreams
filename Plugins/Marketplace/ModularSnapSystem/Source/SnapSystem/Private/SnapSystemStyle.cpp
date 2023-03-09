// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#include "SnapSystemStyle.h"
#include "SnapSystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FSnapSystemStyle::StyleInstance = NULL;

void FSnapSystemStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSnapSystemStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSnapSystemStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SnapSystemStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FSnapSystemStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SnapSystemStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SnapSystem")->GetBaseDir() / TEXT("Resources"));

	Style->Set("SnapSystem.SnapSystemIcon", new IMAGE_BRUSH(TEXT("ButtonIcon_16x"), Icon16x16));
#if ENGINE_MAJOR_VERSION == 5 
	Style->Set("SnapSystem.SnapSystemEnable", new IMAGE_BRUSH(TEXT("ButtonIcon5_16x"), Icon16x16));
#else
	Style->Set("SnapSystem.SnapSystemEnable", new IMAGE_BRUSH(TEXT("ButtonIcon_16x"), Icon16x16));
#endif
	Style->Set("SnapSystem.SnapSystemOptions", new IMAGE_BRUSH(TEXT("ButtonIcon_16x"), Icon16x16));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FSnapSystemStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSnapSystemStyle::Get()
{
	return *StyleInstance;
}
