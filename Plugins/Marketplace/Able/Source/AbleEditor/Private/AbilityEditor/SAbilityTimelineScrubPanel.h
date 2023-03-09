// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AbilityEditor/ablAbilityEditor.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SCompoundWidget.h"

/* The Timeline Scrub Panel displays the time of the Ability from 0.0 to Ability Length with optional tick marks defined by the user. */
class SAblAbilityTimelineScrubPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTimelineScrubPanel)
		: _AbilityEditor()
	{}

	SLATE_ARGUMENT(TSharedPtr<FAblAbilityEditor>, AbilityEditor)

	SLATE_END_ARGS()

	SAblAbilityTimelineScrubPanel();

	// Input catching
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	void Construct(const FArguments& InArgs);

	// Rendering/Size overrides
	virtual FVector2D ComputeDesiredSize(float scale) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
private:
	void SetAbilityPreviewTimeToMousePosition(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	bool IsTouchingMarker(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;

	/* Pointer our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	/* Brush for our widget. */
	const FSlateBrush* m_Brush;

	/* Our Marker Brush. */
	const FSlateBrush* m_MarkerBrush;

	/* Font information for our widget. */
	FSlateFontInfo m_Font;
};