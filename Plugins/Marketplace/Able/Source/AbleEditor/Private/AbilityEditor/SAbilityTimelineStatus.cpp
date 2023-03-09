// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityTimelineStatus.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditor.h"
#include "AbleStyle.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Rendering/SlateRenderer.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblAbilityTimelineStatus::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;
	m_Font = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 8);
	m_Brush = FAbleStyle::GetBrush("Able.AbilityEditor.TimelineStatus");
	m_MarkerBrush = FAbleStyle::GetBrush("Able.AbilityEditor.TimelimeStatus.Marker");
}

FReply SAblAbilityTimelineStatus::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const float CurrentPosition = m_AbilityEditor.Pin()->GetAbilityCurrentTime();
	const float AbilityLength = m_AbilityEditor.Pin()->GetAbilityLength();
	const float TimeRatio = CurrentPosition / AbilityLength;

	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (IsTouchingMarker(MyGeometry, MouseEvent))
		{
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}
		else
		{
			// Add any logic to prevent this?... Maybe an option?...
			SetAbilityPreviewTimeToMousePosition(MyGeometry, MouseEvent);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineStatus::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineStatus::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		SetAbilityPreviewTimeToMousePosition(MyGeometry, MouseEvent);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineStatus::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FCursorReply SAblAbilityTimelineStatus::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	if (HasMouseCapture())
	{
		return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
	}
	else if (IsTouchingMarker(MyGeometry, CursorEvent))
	{
		return FCursorReply::Cursor(EMouseCursor::GrabHand);
	}

	return FCursorReply::Unhandled();
}

FVector2D SAblAbilityTimelineStatus::ComputeDesiredSize(float) const
{
	return FVector2D(100, 14);
}

int32 SAblAbilityTimelineStatus::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!m_AbilityEditor.IsValid())
	{
		// We lost a reference, just call the parent method.
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D& WidgetSize = AllottedGeometry.GetLocalSize();

	FVector2D Padding(4.0f, 4.0f);
	FVector2D TextOffset(0.0f, 0.0f);

	int32 CurrentLayer = LayerId;

	FSlateLayoutTransform Transform;

	const float CurrentPosition = m_AbilityEditor.Pin()->GetAbilityCurrentTime();
	const float AbilityLength = m_AbilityEditor.Pin()->GetAbilityLength();
	const float TimeRatio = CurrentPosition / AbilityLength;

	FNumberFormattingOptions FormatRules;
	FormatRules.SetMinimumIntegralDigits(1);
	FormatRules.SetMaximumFractionalDigits(2);
	FormatRules.SetMinimumFractionalDigits(2);

	const FText CurrentTime = FText::AsNumber(CurrentPosition, &FormatRules);
	const FText MaxTime = FText::AsNumber(AbilityLength, &FormatRules);
	const FText TimeText = FText::Format(LOCTEXT("Timespan", "{0}/{1}"), CurrentTime, MaxTime);
	
	const FVector2D TextSize = FontMeasureService->Measure(TimeText, m_Font);
	const FVector2D Offset(WidgetSize.X - TextSize.X - Padding.X, (WidgetSize.Y - TextSize.Y) * 0.5f);

	// Background
	FSlateDrawElement::MakeBox(OutDrawElements,
		CurrentLayer++,
		AllottedGeometry.ToPaintGeometry(),
		m_Brush);

	Transform = FSlateLayoutTransform(Offset);

	// Time Text
	FSlateDrawElement::MakeText(OutDrawElements, 
		CurrentLayer++, 
		AllottedGeometry.ToPaintGeometry(Transform), 
		TimeText, 
		m_Font);

	FVector2D Line[2];
	Line[0].Y = 1;
	Line[1].Y = WidgetSize.Y - 1;

	// Line between our time label and the rest of the widget.
	Line[0].X = Offset.X - Padding.X;
	Line[1].X = Line[0].X;

	TArray<FVector2D> LineArray;
	LineArray.Add(Line[0]);
	LineArray.Add(Line[1]);

	FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer++, AllottedGeometry.ToPaintGeometry(), LineArray);
	
	// Draw our Marker Icon
	FVector2D MarkerSize(12.0f, 12.0f);
	FVector2D MarkerOffset;
	
	// Our Marker track goes up to the line we use to separate the time value from the rest of the widget.
	const float MarkerTrackSize = WidgetSize.X - (WidgetSize.X - Line[0].X);
	MarkerOffset.X = MarkerTrackSize * TimeRatio - MarkerSize.X * 0.5f;
	MarkerOffset.Y = 1;

	Transform = FSlateLayoutTransform(MarkerOffset);

	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer++, AllottedGeometry.ToPaintGeometry(MarkerSize, Transform), m_MarkerBrush);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, CurrentLayer, InWidgetStyle, bParentEnabled);
}

void SAblAbilityTimelineStatus::SetAbilityPreviewTimeToMousePosition(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const float AbilityLength = m_AbilityEditor.Pin()->GetAbilityLength();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D LocalSize = MyGeometry.GetLocalSize() - FVector2D(4.0f, 4.0f); //Padding
	const float PositionRatio = FMath::Clamp(LocalPos.X / LocalSize.X, 0.0f, 1.0f);

	m_AbilityEditor.Pin()->SetAbilityPreviewTime(AbilityLength * PositionRatio);
}

bool SAblAbilityTimelineStatus::IsTouchingMarker(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const float CurrentPosition = m_AbilityEditor.Pin()->GetAbilityCurrentTime();
	const float AbilityLength = m_AbilityEditor.Pin()->GetAbilityLength();
	const float TimeRatio = CurrentPosition / AbilityLength;

	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());	
	const FVector2D MarkerSize(12.0f, 12.0f);
	const FVector2D MarkerPos(MyGeometry.GetLocalSize().X * TimeRatio, 1.0f);

	// We only care about the X dimension. Do need for Y check...
	return LocalPos.X > (MarkerPos.X - MarkerSize.X) && LocalPos.X < (MarkerPos.X + MarkerSize.X);
}

#undef LOCTEXT_NAMESPACE