#pragma once

#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"
#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"

class FAblAssetShortcutParams
{
public:
	DECLARE_DELEGATE_OneParam(FOnAssetSelectedCallback, const FAssetData& /*AssetData*/);
	DECLARE_DELEGATE(FOnAssetClicked);
	DECLARE_DELEGATE_RetVal(FAssetData, FOnGetInitialAsset);

	FAblAssetShortcutParams();
	FAblAssetShortcutParams(const UClass* InRequiredParentClass, const FText& InDisplayName, const FSlateBrush* InBrush, const FSlateColor& InColor);
	~FAblAssetShortcutParams();

	bool IsAssetCompatible(const FAssetData& InAssetData) const;

	FText ShortcutDisplayName;

	const FSlateBrush* ShortcutIconBrush;

	FSlateColor ShortcutIconColor;

	const UClass* RequiredParentClass;

	FOnAssetSelectedCallback AssetCallback;
	FOnAssetClicked AssetClickCallback;
	FOnGetInitialAsset InitialAssetCallback;
};

class SAblAssetFamilyShortcutBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAssetFamilyShortcutBar)
	{}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<class FWorkflowCentricApplication>& InHostingApp, const TArray<TSharedPtr<FAblAssetShortcutParams>>& InParams);

private:
	/** The thumbnail pool for displaying asset shortcuts */
	TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;
};