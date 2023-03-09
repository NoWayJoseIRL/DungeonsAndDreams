// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Editor/UnrealEd/Public/SViewportToolBar.h"

class FAblAbilityEditor;
class SAbilityEditorViewportTabBody;

/* Viewport widget for our Ability Viewport. */
class SAbilityEditorViewportToolBar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS(SAbilityEditorViewportToolBar) { }
		SLATE_ARGUMENT(TSharedPtr<FAblAbilityEditor>, AbilityEditor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class SAbilityEditorViewportTabBody> InViewport, TSharedPtr<class SEditorViewport> InRealViewport);

private:
	/* Creates the basic view option menu.*/
	TSharedRef<SWidget> GenerateViewMenu();
	
	/* Creates the Viewport Type (Perspective, etc) menu. */
	TSharedRef<SWidget> GenerateViewportTypeMenu();

	/* Camera Labels and Icons*/
	FText GetCameraMenuLabel() const;
	const FSlateBrush* GetCameraMenuLabelIcon() const;

	/* Callback for Transform bar visibility. */
	EVisibility GetTransformToolBarVisibility() const;

	/* Called by the FOV slider in the perspective viewport to get the FOV value */
	float OnGetFOVValue() const;

	/* Called when the FOV slider is adjusted in the perspective viewport */
	void OnFOVValueChanged(float NewValue);
	
	/* Pointer to our Viewport Tab widget. */
	TWeakPtr<class SAbilityEditorViewportTabBody> m_Viewport;

	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;
};