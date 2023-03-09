// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityTimelineTrack.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbleStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Rendering/SlateRenderer.h"
#include "ScopedTransaction.h"
#include "Tasks/IAblAbilityTask.h"
#include "Layout/WidgetPath.h"
#include "Widgets/Layout/SBorder.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblAbilityTimelineNode::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;
	m_Task = InArgs._Task;

	m_Font =  FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10);

	m_NodeBrush = FAbleStyle::GetBrush("Able.AbilityEditor.Node");
	m_HighlightNodeBrush = FAbleStyle::GetBrush("Able.AbilityEditor.NodeHighlight");
	m_TextBackgroundBrush = FAbleStyle::GetBrush("Able.AbilityEditor.NodeText");
	m_DependencyIcon = FAbleStyle::GetBrush("Able.AbilityEditor.Timeline.Dependency");
	m_LockIcon = FAbleStyle::GetBrush("Able.AbilityEditor.Timeline.Lock");

	m_DragType = EAbilityTaskDragType::None;
	m_DragStartTime = 0.0f;
	m_DragEndTime = 0.0f;
}

FText SAblAbilityTimelineNode::GetNodeText() const
{
	if (m_Task.IsValid())
	{
		const float TaskStartTime = m_Task->GetStartTime();
		const float AbilityLength = m_AbilityEditor.Pin()->GetAbility()->GetLength();
		const bool DescriptiveTitles = m_AbilityEditor.Pin()->GetEditorSettings().m_ShowDescriptiveTaskTitles;

		if (TaskStartTime > AbilityLength)
		{
			// Our Task is outside of our Length. Show it as Bright red so the user can fix it.
			return FText::FormatOrdered(LOCTEXT("AblAbilityTimelineError", "{0} (Warning: Task outside of Length)"), m_Task->GetTaskName());
		}

		return DescriptiveTitles ? m_Task->GetDescriptiveTaskName() : m_Task->GetTaskName();
	}

	return LOCTEXT("AblAbilityEditor", "Node");
}

FLinearColor SAblAbilityTimelineNode::GetNodeColor() const
{
	if (m_Task.IsValid())
	{
		const float TaskStartTime = m_Task->GetStartTime();
		const float AbilityLength = m_AbilityEditor.Pin()->GetAbility()->GetLength();

		if (TaskStartTime > AbilityLength)
		{
			// Our Task is outside of our Length. Show it as Bright red so the user can fix it.
			return FLinearColor::Red;
		}

		if (m_AbilityEditor.Pin()->ShowTaskCostEstimate())
		{
			return m_AbilityEditor.Pin()->GetTaskCostColor(*m_Task.Get());
		}

		FLinearColor TaskColor = m_Task->HasUserDefinedTaskColor() ? m_Task->GetUserDefinedTaskColor() : m_Task->GetTaskColor();

		if (m_Task->IsAlwaysDisabled())
		{
			TaskColor.A = 0.35f;
		}

		return TaskColor;
	}

	return FLinearColor::White;
}

FReply SAblAbilityTimelineNode::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool IsLMBDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
	const bool IsRMBDown = MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);

	if (IsLMBDown || IsRMBDown)
	{ 
		if (m_AbilityEditor.IsValid() && m_Task.IsValid())
		{
			m_AbilityEditor.Pin()->SetCurrentTask(m_Task.Get());

			if (IsLMBDown && !m_Task->IsLocked())
			{
				m_DragType = DetermineDragType(MyGeometry, MouseEvent);
				if (m_DragType != EAbilityTaskDragType::None)
				{
					m_DragStartTime = m_Task->GetStartTime();
					m_DragEndTime = m_Task->GetEndTime();

					return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
				}
			}
		}
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineNode::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (m_DragType != EAbilityTaskDragType::None)
	{
		CommitDragChanges();

		m_DragType = EAbilityTaskDragType::None;
		
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (m_AbilityEditor.IsValid())
		{
			TSharedRef<SWidget> MenuContents = m_AbilityEditor.Pin()->GenerateTaskContextMenu();
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuContents, MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineNode::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	if (m_DragType != EAbilityTaskDragType::None)
	{
		const FVector2D& ParentWidgetSize = MyGeometry.GetLocalSize();

		const UAblAbility* Ability = m_AbilityEditor.Pin()->GetAbility();
		const UAblAbilityEditorSettings& EditorSettings = m_AbilityEditor.Pin()->GetEditorSettings();
		UAblAbilityTask* Task = m_Task.Get();

		if (!Task)
		{
			return FReply::Unhandled();
		}

		FVector2D MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

		float MouseTimePosition = MousePos.X / ParentWidgetSize.X * Ability->GetLength();

		// Perform snapping.
		if (EditorSettings.m_SnapTasks)
		{
			int NumberOfSnapUnits = FMath::FloorToInt(MouseTimePosition / EditorSettings.m_SnapUnit);
			MouseTimePosition = (float)NumberOfSnapUnits * EditorSettings.m_SnapUnit;
		}

		MouseTimePosition = FMath::Clamp(MouseTimePosition, 0.0f, Ability->GetLength());
		if (m_DragType == EAbilityTaskDragType::MoveStart)
		{
			if (MouseTimePosition < Task->GetEndTime()) // Ignore if we've gone past our end time.
			{
				m_DragStartTime = MouseTimePosition;
			}
		}
		else if (m_DragType == EAbilityTaskDragType::MoveEnd)
		{
			if (MouseTimePosition > Task->GetStartTime()) // Ignore if we're trying to go past our start time.
			{
				m_DragEndTime = MouseTimePosition;
			}
		}
		else // EAbilityTaskDragType::MoveCenter
		{
			if (Task->IsSingleFrame())
			{
				// Just use the pointer position.
				m_DragStartTime = MouseTimePosition;
			}
			else
			{
				// Center around the pointer position.
				float HalfDuration = (m_DragEndTime - m_DragStartTime) * 0.5f;
				float NewStartTime = FMath::Max(0.0f, MouseTimePosition - HalfDuration);
				float NewEndTime = FMath::Min(Ability->GetLength(), MouseTimePosition + HalfDuration);

				m_DragStartTime = NewStartTime;
				m_DragEndTime = NewEndTime;
			}
		}
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineNode::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (m_DragType != EAbilityTaskDragType::None)
	{
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FCursorReply SAblAbilityTimelineNode::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	EAbilityTaskDragType TempDrag = m_DragType;
	if (TempDrag == EAbilityTaskDragType::None)
	{
		// See if we could drag if we wanted.
		TempDrag = DetermineDragType(MyGeometry, CursorEvent);
	}

	switch (TempDrag)
	{
	case EAbilityTaskDragType::MoveCenter:
		{
			if (HasMouseCapture())
			{
				// We're already dragging.
				return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
			}
			return FCursorReply::Cursor(EMouseCursor::GrabHand);
		}
		break;
	case EAbilityTaskDragType::MoveStart:
	case EAbilityTaskDragType::MoveEnd:
		{
			return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
		}
	default:
		break;
	}

	return FCursorReply::Unhandled();
}

FVector2D SAblAbilityTimelineNode::ComputeDesiredSize(float) const
{
	const float WidgetHeight = 64.0f;
	
	return FVector2D(100.0f, WidgetHeight); // This doesn't matter. As long as we have enough room in our track, we're fine. 
}

int32 SAblAbilityTimelineNode::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!m_AbilityEditor.IsValid() || !m_Task.IsValid())
	{
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D& ParentWidgetSize = AllottedGeometry.GetLocalSize();
	const FVector2D DependencyIconSize(12.0f, 12.0f);
	const FVector2D LockIconSize(32.0f, 32.0f);
	const FVector2D TextBackgroundPadding(2.0f, 0.0f);

	const UAblAbility* Ability = m_AbilityEditor.Pin()->GetAbility();
	const UAblAbilityTask* Task = m_Task.Get();
	const bool IsDependency = m_AbilityEditor.Pin()->TaskHasDependency(m_AbilityEditor.Pin()->GetCurrentlySelectedAbilityTask(), Task);
	const bool CurrentlySelected = m_AbilityEditor.Pin()->GetCurrentlySelectedAbilityTask() == Task;

	// For Single frames, we just use a 5% of whatever the Ability length is.
	float TaskSizeRatio =  (Task->IsSingleFrame() ? Ability->GetLength() * 0.05f : GetNodeDuration()) / Ability->GetLength();
	TaskSizeRatio = FMath::Clamp(TaskSizeRatio, 0.05f, 1.0f);

	int validParentSize = ParentWidgetSize.X;
	float Offset = validParentSize * (GetNodeStartTime() / Ability->GetLength());
	const float Size = validParentSize * TaskSizeRatio;

	// Length of Text + Bit of padding on both ends
	const float TextPadding = IsDependency ? DependencyIconSize.X + TextBackgroundPadding.X * 2.0f : TextBackgroundPadding.X * 2.0f;
	const float TextSize = FontMeasureService->Measure(GetNodeText(), m_Font).X + TextPadding;

	const FVector2D NodeSize(Size, ParentWidgetSize.Y);
	const FVector2D TextNodeSize(TextSize, FontMeasureService->GetMaxCharacterHeight(m_Font) + 2);


	if (Offset + Size >= validParentSize)
	{
		// We don't want to display off screen.
		Offset -= (Offset + Size) - validParentSize;
	}
	
	const FVector2D NodeTranslation(Offset, 0.0f);
	const FSlateLayoutTransform NodeOffset(NodeTranslation);

	FVector2D TextBackgroundTranslation = NodeTranslation;
	FVector2D TextTranslation = NodeTranslation;
	if (TextTranslation.X + TextSize > validParentSize)
	{
		const float Overflow = (TextTranslation.X + TextSize) - validParentSize;

		TextTranslation.X -= Overflow;
		TextBackgroundTranslation.X -= Overflow;
	}
	else
	{
		// Move the Text out a bit from the edge of the node.
		TextTranslation += TextBackgroundPadding;
	}

	// Move Text away from our dependency icon.
	if (IsDependency)
	{
		TextTranslation.X = TextTranslation.X + DependencyIconSize.X;
	}

	const FSlateLayoutTransform TextOffset(TextTranslation);
	const FSlateLayoutTransform TextBackgroundOffset(TextBackgroundTranslation);

	int32 CurrentLayer = LayerId;

	// Node
	FSlateDrawElement::MakeBox(OutDrawElements,
		CurrentLayer++,
		AllottedGeometry.ToPaintGeometry(NodeSize, NodeOffset),
		CurrentlySelected ? m_HighlightNodeBrush : m_NodeBrush,
		ESlateDrawEffect::None,
		GetNodeColor());

	// If we're playing our ability and within our time frame, show a line to help designers follow the flow.
	if (m_AbilityEditor.Pin()->IsPlayingAbility())
	{
		float CurrentTime = m_AbilityEditor.Pin()->GetAbilityCurrentTime();
		if (CurrentTime > GetNodeStartTime() && CurrentTime < GetNodeEndTime())
		{
			float TimeRatio = CurrentTime / m_AbilityEditor.Pin()->GetAbilityLength();
			FVector2D Line[2];
			Line[0].X = ParentWidgetSize.X * TimeRatio;
			Line[0].Y = 1;
			Line[1].X = Line[0].X;
			Line[1].Y = ParentWidgetSize.Y - 2.0f;

			TArray<FVector2D> LineArray;
			LineArray.Add(Line[0]);
			LineArray.Add(Line[1]);
			FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer++, AllottedGeometry.ToPaintGeometry(), LineArray, ESlateDrawEffect::None, FLinearColor::Red, true, 4.0f);
		}
	}

	// Show our dependency icon.
	if (IsDependency)
	{
		const FVector2D IconOffset(TextBackgroundTranslation + FVector2D(8.0f, (ParentWidgetSize.Y - DependencyIconSize.Y) - 6.0f));
		FSlateLayoutTransform IconTransform(IconOffset);

		FSlateDrawElement::MakeBox(OutDrawElements,
			CurrentLayer++,
			AllottedGeometry.ToPaintGeometry(DependencyIconSize, IconTransform),
			m_DependencyIcon);
	}

	// Show our Lock icon. 
	if (Task->IsLocked())
	{
		const FVector2D IconOffset(TextBackgroundTranslation + FVector2D(0.0f, 2.0f));
		FSlateLayoutTransform IconTransform(IconOffset);

		FSlateDrawElement::MakeBox(OutDrawElements,
			CurrentLayer++,
			AllottedGeometry.ToPaintGeometry(LockIconSize, IconTransform),
			m_LockIcon);
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, CurrentLayer, InWidgetStyle, bParentEnabled);
}

float SAblAbilityTimelineNode::GetNodeStartTime() const
{
	if (m_DragType != EAbilityTaskDragType::None)
	{
		return m_DragStartTime;
	}
	else if (m_Task.IsValid())
	{
		return m_Task->GetStartTime();
	}

	return 0.0f;
}

float SAblAbilityTimelineNode::GetNodeEndTime() const
{
	if (m_DragType != EAbilityTaskDragType::None)
	{
		return m_DragEndTime;
	}
	else if (m_Task.IsValid())
	{
		return m_Task->GetEndTime();
	}

	return 0.0f;
}

float SAblAbilityTimelineNode::GetNodeDuration() const
{
	return GetNodeEndTime() - GetNodeStartTime();
}

void SAblAbilityTimelineNode::CommitDragChanges() const
{
	if (UAblAbilityTask* Task = m_Task.Get())
	{
		if (Task->IsLocked())
		{
			return;
		}

		switch (m_DragType)
		{
		case EAbilityTaskDragType::MoveStart:
		{
			if (!FMath::IsNearlyEqual(m_DragStartTime, Task->GetStartTime()))
			{
				const FScopedTransaction ResizeTransaction(LOCTEXT("TaskResizeTransaction", "Resize Task"));

				Task->Modify();

				Task->SetStartTime(m_DragStartTime);
			}

		}
		break;
		case EAbilityTaskDragType::MoveEnd:
		{
			if (!FMath::IsNearlyEqual(m_DragEndTime, Task->GetEndTime()))
			{
				const FScopedTransaction ResizeTransaction(LOCTEXT("TaskResizeTransaction", "Resize Task"));

				Task->Modify();

				Task->SetEndTime(m_DragEndTime);
			}
		}
		break;
		case EAbilityTaskDragType::MoveCenter:
		{
			if (!FMath::IsNearlyEqual(m_DragStartTime, Task->GetStartTime()) || !FMath::IsNearlyEqual(m_DragEndTime, Task->GetEndTime()))
			{
				const FScopedTransaction ResizeTransaction(LOCTEXT("TaskMoveTransaction", "Move Task"));

				Task->Modify();

				Task->SetStartTime(m_DragStartTime);
				Task->SetEndTime(m_DragEndTime);
			}
		}
		break;
		case EAbilityTaskDragType::None:
		default:
			break;
		}
	}

}

EAbilityTaskDragType SAblAbilityTimelineNode::DetermineDragType(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const FVector2D& ParentWidgetSize = MyGeometry.GetLocalSize();

	const UAblAbility* Ability = m_AbilityEditor.Pin()->GetAbility();
	const UAblAbilityTask* Task = m_Task.Get();

	// Ignore if the Task is locked, or missing.
	if (!Task || (Task && Task->IsLocked()) )
	{
		return EAbilityTaskDragType::None;
	}

	// For Single frames, we just use a 5% of whatever the Ability length is.
	float TaskSizeRatio = (Task->IsSingleFrame() ? Ability->GetLength() * 0.05f : Task->GetDuration()) / Ability->GetLength();
	TaskSizeRatio = FMath::Clamp(TaskSizeRatio, 0.05f, 1.0f);

	int validParentSize = ParentWidgetSize.X;
	float Size = validParentSize * TaskSizeRatio;
	float Offset = validParentSize * (Task->GetStartTime() / Ability->GetLength());

	if (Offset + Size >= validParentSize)
	{
		// We don't want to display off screen.
		Offset -= (Offset + Size) - validParentSize;
	}

	FVector2D NodeSize(Size, ParentWidgetSize.Y);

	const FVector2D MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (MousePos.X > Offset && MousePos.X < Offset + Size)
	{
		if (!Task->IsSingleFrame()) // Only allow dynamic time sizing if it isn't single frame.
		{
			float ClickPosRelativeToWidget = (MousePos.X - Offset) / Size;
			if (ClickPosRelativeToWidget < 0.1f)
			{
				// Dragging the left side of the node.
				return EAbilityTaskDragType::MoveStart;
			}
			else if (ClickPosRelativeToWidget > 0.9f)
			{
				// Dragging the right side of the node.
				return EAbilityTaskDragType::MoveEnd;
			}
		}

		// Moving the entire node.
		return EAbilityTaskDragType::MoveCenter;
	}

	return EAbilityTaskDragType::None;
}

void SAblAbilityTimelineTrack::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;

	this->ChildSlot
		.HAlign(HAlign_Fill)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Gray)
			.Content()
			[
				SAssignNew(m_Node, SAblAbilityTimelineNode)
				.AbilityEditor(m_AbilityEditor)
				.Task(InArgs._AbilityTask)
			]
		];

	m_TrackBrush = InArgs._Brush;
}

FVector2D SAblAbilityTimelineTrack::ComputeDesiredSize(float scale) const
{
	// Since it's Horizontal Fill, the X is ignored, we only care about the Y (Height).
	if (m_Node.IsValid())
	{
		return m_Node.Get()->ComputeDesiredSize(scale);
	}

	return FVector2D(100.0f, 50.0f);
}

int32 SAblAbilityTimelineTrack::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FSlateDrawElement::MakeBox(OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		m_TrackBrush);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId + 1, InWidgetStyle, bParentEnabled);
}

#undef LOCTEXT_NAMESPACE

