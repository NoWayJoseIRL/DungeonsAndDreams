// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityTimelineScrubPanel.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbleStyle.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Rendering/SlateRenderer.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

bool IsWholeNumber(float num)
{
	return FMath::IsNearlyEqual(FMath::FloorToFloat(num), num);
}

SAblAbilityTimelineScrubPanel::SAblAbilityTimelineScrubPanel()
{

}

FReply SAblAbilityTimelineScrubPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

FReply SAblAbilityTimelineScrubPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineScrubPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		SetAbilityPreviewTimeToMousePosition(MyGeometry, MouseEvent);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SAblAbilityTimelineScrubPanel::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FCursorReply SAblAbilityTimelineScrubPanel::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
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

void SAblAbilityTimelineScrubPanel::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;

	m_Brush = FAbleStyle::GetBrush("Able.AbilityEditor.Timeline");
	m_MarkerBrush = FAbleStyle::GetBrush("Able.AbilityEditor.TimelimeStatus.Marker");

	m_Font = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 8);
}

FVector2D SAblAbilityTimelineScrubPanel::ComputeDesiredSize(float scale) const
{
	// Width is assumed to be stretched to fill, so only height really matters here.
	return FVector2D(100.0f, 40.0f);
}

int32 SAblAbilityTimelineScrubPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FVector2D TimeRange(0.0f, 1.0f);
	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const UAblAbilityEditorSettings* EditorSettings = GetDefault<UAblAbilityEditorSettings>();

	if (m_AbilityEditor.IsValid())
	{
		TimeRange = m_AbilityEditor.Pin()->GetAbilityTimeRange();
	}

	const float CurrentPosition = m_AbilityEditor.Pin()->GetAbilityCurrentTime();
	const float AbilityLength = m_AbilityEditor.Pin()->GetAbilityLength();
	const float TimeRatio = CurrentPosition / AbilityLength;

	FNumberFormattingOptions WholeNumberFmt;
	WholeNumberFmt.SetMaximumFractionalDigits(0);

	FNumberFormattingOptions FmtOptions;
	FmtOptions.SetMaximumFractionalDigits(2);
	FmtOptions.SetMinimumFractionalDigits(1);

	FText MinText = FText::AsNumber(TimeRange.X, &WholeNumberFmt);
	FText MaxText = FText::AsNumber(TimeRange.Y, IsWholeNumber(TimeRange.Y) ? &WholeNumberFmt : &FmtOptions);
	FVector2D MinSize = FontMeasureService->Measure(MinText, m_Font);
	FVector2D MaxSize = FontMeasureService->Measure(MaxText, m_Font);

	const FVector2D& WidgetSize = AllottedGeometry.GetLocalSize();

	// Our Padding is how much it takes to safely display our largest string.
	const float Padding = MaxSize.X * 0.5f + 4.0f;
	const float ScrollbarWidth = 12.0f;
	const float VertPadding = 4.0f;
	FVector2D TextOffset(0.0f, VertPadding);


	int32 CurrentLayer = LayerId;

	FSlateDrawElement::MakeBox(OutDrawElements,
		CurrentLayer++,
		AllottedGeometry.ToPaintGeometry(),
		m_Brush);

	FSlateLayoutTransform Transform;

	if (!EditorSettings->m_ShowGuidelineLabels) // Our Guideline will always get the lower value.
	{
		// Left side (Min Time Label)
		TextOffset.X = Padding + MinSize.X * 0.5f;
		TextOffset.Y = VertPadding;

		Transform = FSlateLayoutTransform(TextOffset);

		FSlateDrawElement::MakeText(OutDrawElements,
			CurrentLayer++,
			AllottedGeometry.ToPaintGeometry(Transform),
			MinText,
			m_Font);
	}

	// Right side (Max Time Label)
	TextOffset.X = WidgetSize.X - MaxSize.X * 0.5f - Padding - ScrollbarWidth;
	
	Transform = FSlateLayoutTransform(TextOffset);
	
	FSlateDrawElement::MakeText(OutDrawElements,
		CurrentLayer++,
		AllottedGeometry.ToPaintGeometry(Transform),
		MaxText,
		m_Font);

	// Lines
	float MajorLineSize = WidgetSize.Y * 0.45f;
	float MinorLineSize = WidgetSize.Y * 0.20f;

	TArray<FVector2D> Line; // Start/End points.
	Line.SetNum(2);

	float MajorFreq = EditorSettings->m_MajorLineFrequency;
	float MinorFreq = EditorSettings->m_MinorLineFrequency;

	// Major
	float StepTime = 0.0f;
	FText MajorLabel;

	Line[0].Y = WidgetSize.Y - 1.0f - VertPadding; // Bottom, minus a bit of padding.
	Line[1].Y = WidgetSize.Y - MajorLineSize + VertPadding;


	const float WidgetInteriorSize = WidgetSize.X - Padding * 2.0f - ScrollbarWidth; // We have padding on both ends.

	for (; StepTime < TimeRange.Y; StepTime += MajorFreq)
	{
		Line[0].X =  Padding + WidgetInteriorSize * (StepTime / TimeRange.Y);
		Line[1].X = Line[0].X;

		FSlateDrawElement::MakeLines(OutDrawElements,
			CurrentLayer++,
			AllottedGeometry.ToPaintGeometry(),
			Line);

		if (EditorSettings->m_ShowGuidelineLabels)
		{
			MajorLabel = FText::AsNumber(StepTime, IsWholeNumber(StepTime) ? &WholeNumberFmt : &FmtOptions);

			TextOffset.X = Line[0].X - FontMeasureService->Measure(MajorLabel, m_Font).X * 0.5f;
			Transform = FSlateLayoutTransform(TextOffset);

			FSlateDrawElement::MakeText(OutDrawElements,
				CurrentLayer++,
				AllottedGeometry.ToPaintGeometry(Transform),
				MajorLabel,
				m_Font);
		}
	}

	// Minor
	for (StepTime = 0.0f; StepTime < TimeRange.Y; StepTime += MinorFreq)
	{
		Line[0].X = Padding + WidgetInteriorSize * (StepTime / TimeRange.Y);
		Line[1].X = Line[0].X;

		FSlateDrawElement::MakeLines(OutDrawElements,
			CurrentLayer++,
			AllottedGeometry.ToPaintGeometry(),
			Line);
	}

	// Far Right Line
	Line[0].X = WidgetSize.X - Padding - ScrollbarWidth;
	Line[1].X = Line[0].X;

	FSlateDrawElement::MakeLines(OutDrawElements,
		CurrentLayer++,
		AllottedGeometry.ToPaintGeometry(),
		Line);

	// Draw our Marker Icon
	FVector2D MarkerSize(12.0f, 12.0f);
	FVector2D MarkerOffset;

	// Our Marker track goes up to the line we use to separate the time value from the rest of the widget.
	MarkerOffset.X = (Padding + WidgetInteriorSize * TimeRatio - MarkerSize.X * 0.5f);
	MarkerOffset.Y = 4; // Put it under our outline.

	Transform = FSlateLayoutTransform(MarkerOffset);

	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer++, AllottedGeometry.ToPaintGeometry(MarkerSize, Transform), m_MarkerBrush);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, CurrentLayer, InWidgetStyle, bParentEnabled);
}

void SAblAbilityTimelineScrubPanel::SetAbilityPreviewTimeToMousePosition(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const float AbilityLength = m_AbilityEditor.Pin()->GetAbilityLength();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D LocalSize = MyGeometry.GetLocalSize() - FVector2D(4.0f, 4.0f); //Padding
	const float PositionRatio = FMath::Clamp(LocalPos.X / LocalSize.X, 0.0f, 1.0f);

	m_AbilityEditor.Pin()->SetAbilityPreviewTime(AbilityLength * PositionRatio);
}

bool SAblAbilityTimelineScrubPanel::IsTouchingMarker(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const float CurrentPosition = m_AbilityEditor.Pin()->GetAbilityCurrentTime();
	const float AbilityLength = m_AbilityEditor.Pin()->GetAbilityLength();
	const float TimeRatio = CurrentPosition / AbilityLength;

	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D MarkerSize(12.0f, 12.0f);
	const FVector2D MarkerPos(MyGeometry.GetLocalSize().X * TimeRatio, 1.0f);

	// We only care about the X dimension. No need for Y check...
	return LocalPos.X > (MarkerPos.X - MarkerSize.X) && LocalPos.X < (MarkerPos.X + MarkerSize.X);
}

#undef LOCTEXT_NAMESPACE