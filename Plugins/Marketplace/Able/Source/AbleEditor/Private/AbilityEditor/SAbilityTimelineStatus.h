// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AbilityEditor/ablAbilityEditor.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SCompoundWidget.h"

/* Timeline Status is a widget meant to be on top of the Timeline Scrub Panel. It displays times information and a marker depicting the current Ability Time. */
class SAblAbilityTimelineStatus : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTimelineStatus)
		: _AbilityEditor()
	{}
	SLATE_ARGUMENT(TWeakPtr<FAblAbilityEditor>, AbilityEditor)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Input catching
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	// Rendering/Size overrides
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
private:
	void SetAbilityPreviewTimeToMousePosition(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	bool IsTouchingMarker(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;

	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;
	
	/* Our Node Brush. */
	const FSlateBrush* m_Brush;

	/* Our Marker Brush. */
	const FSlateBrush* m_MarkerBrush;

	/* Font Info for our Widget. */
	FSlateFontInfo m_Font;
};