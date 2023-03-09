// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AbilityEditor/ablAbilityEditor.h"

#include "Fonts/SlateFontInfo.h"
#include "ScopedTransaction.h"
#include "Tasks/IAblAbilityTask.h"
#include "Widgets/SCompoundWidget.h"

struct FSlateBrush;

enum EAbilityTaskDragType
{
   None = 0,
   MoveCenter,
   MoveStart,
   MoveEnd,

   TotalTypes
};

/* This widget represents a single Task within an Ability's Timeline.*/
class SAblAbilityTimelineNode : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTimelineNode)
		: _AbilityEditor(),
		  _Task()
	{}
	SLATE_ARGUMENT(TWeakPtr<FAblAbilityEditor>, AbilityEditor)
	SLATE_ARGUMENT(TWeakObjectPtr<UAblAbilityTask>, Task)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FText GetNodeText() const;
	FLinearColor GetNodeColor() const;
	const UAblAbilityTask* GetTask() const { return m_Task.Get(); }

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
	/* Get Node Start Time. */
	float GetNodeStartTime() const;

	/* Get Node End Time. */
	float GetNodeEndTime() const;

	/* Get Node Duration. */
	float GetNodeDuration() const;

	/* Commits the changes made during dragging. */
	void CommitDragChanges() const;

	/* Helper method, given a location it determines what Drag operation would be done if the user chooses to drag their cursor. */
	EAbilityTaskDragType DetermineDragType(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;

	/* Our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	/* The Task we represent. */
	TWeakObjectPtr<UAblAbilityTask> m_Task;

	/* Our Node brush. */
	const FSlateBrush* m_NodeBrush;

	/* Our Highlighted Node Brush. */
	const FSlateBrush* m_HighlightNodeBrush;

	/* Our Text Background brush. */
	const FSlateBrush* m_TextBackgroundBrush;

	/* Our Dependency Icon brush. */
	const FSlateBrush* m_DependencyIcon;

	/* Our Lock Icon brush. */
	const FSlateBrush* m_LockIcon;

	/* Cached Font info we'll be using for our text.*/
	FSlateFontInfo m_Font;

	/* Our current Drag operation type. */
	EAbilityTaskDragType m_DragType;

	/* Currently dragged Start time. */
	float m_DragStartTime;

	/* Currently dragged End time. */
	float m_DragEndTime;
};

/* This widget represents a slot in which a TimelineNode can move around in. */
class SAblAbilityTimelineTrack : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTimelineTrack)
		: _AbilityEditor(),
		  _AbilityTask()
	{}

	SLATE_ARGUMENT(TWeakPtr<FAblAbilityEditor>, AbilityEditor)
	SLATE_ARGUMENT(TWeakObjectPtr<UAblAbilityTask>, AbilityTask)
	SLATE_ARGUMENT(const FSlateBrush*, Brush)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Rendering/Size overrides
	virtual FVector2D ComputeDesiredSize(float scale) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	/* Our Ability Editor Instance.*/
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	/* Our Timeline Node. */
	TSharedPtr<SAblAbilityTimelineNode> m_Node;

	/* Our Timeline Track brush. */
	const FSlateBrush* m_TrackBrush;
};