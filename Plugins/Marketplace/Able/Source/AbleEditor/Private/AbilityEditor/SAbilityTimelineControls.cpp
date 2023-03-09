// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityTimelineControls.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditor.h"
#include "AbleStyle.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblAbilityTimelineControls::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;
}

FVector2D SAblAbilityTimelineControls::ComputeDesiredSize(float) const
{
	return FVector2D(100.0f, 26.0f); // height specific, width doesn't matter.
}

#undef LOCTEXT_NAMESPACE