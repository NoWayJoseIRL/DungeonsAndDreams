// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AbilityEditor/ablAbilityEditor.h"
#include "Widgets/SCompoundWidget.h"

/* A Simple Timeline control widget (pause/play/step/frame/time)*/
class SAblAbilityTimelineControls : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTimelineControls)
		: _AbilityEditor()
	{}
	SLATE_ARGUMENT(TWeakPtr<FAblAbilityEditor>, AbilityEditor)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Rendering/Size overrides
	virtual FVector2D ComputeDesiredSize(float) const override;
private:
	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;
};