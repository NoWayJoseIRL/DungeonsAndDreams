// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#if 0

#include "AbilityEditor/ablAbilityEditor.h"
#include "Tasks/IAblAbilityTask.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/SRichTextBlock.h"

/* Task Summary is just a fixed height rich text field. */

class SAblAbilityTaskSummary : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTaskSummary)
		: _AbilityEditor(),
		_Task()
	{}
	SLATE_ARGUMENT(TWeakPtr<FAblAbilityEditor>, AbilityEditor)
	SLATE_ARGUMENT(TWeakObjectPtr<UAblAbilityTask>, Task)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Rendering/Size overrides
	virtual FVector2D ComputeDesiredSize(float) const override;
private:
	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	/* Our Task we're summarizing. */
	TWeakObjectPtr<UAblAbilityTask> m_Task;

	/* Our RT Block. */
	TSharedPtr<SRichTextBlock> m_TextBlock;
};

#endif