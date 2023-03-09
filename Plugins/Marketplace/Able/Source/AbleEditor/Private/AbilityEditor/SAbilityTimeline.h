// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Fonts/SlateFontInfo.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/SCompoundWidget.h"

class UAblAbilityTask;
class FAblAbilityEditor;
class SAblAbilityTimelineTrack;
class UEdGraph;

/* Timeline Panel is a widget that displays all the Tasks in our Ability. When a Task is added or removed, the widget will update itself appropriately. */
class SAblAbilityTimelinePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTimelinePanel)
		: _AbilityEditor()
	{}

	SLATE_ARGUMENT(TSharedPtr<FAblAbilityEditor>, AbilityEditor)

	SLATE_END_ARGS()

	SAblAbilityTimelinePanel();

	void Construct(const FArguments& InArgs);

	// Tick Override 
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// Input Override
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
private:
	float RoundFloat(float in, int32 numDecimalPoints) const;

	/* Callback for when the Editor is refreshed. */
	void OnRefresh();

	/* Builds our list of Task widgets and adds them to our task container. */
	void BuildTaskList();

	TSharedRef<SWidget> BuildTimelineControls();

	FReply OnStepBackwards();
	FReply OnPlay();
	FReply OnStop();
	FReply OnPlayOrPause();
	FReply OnStepForwards();

	TOptional<float> GetTimeMin() const;
	TOptional<float> GetTimeMax() const;
	TOptional<float> GetCurrentTime() const;

	TOptional<int> GetCurrentFrame() const;
	TOptional<int> GetFrameMin() const;
	TOptional<int> GetFrameMax() const;

	void OnTimeValueCommitted(float NewValue, ETextCommit::Type CommitInfo);
	void OnFrameValueCommitted(int NewValue, ETextCommit::Type CommitInfo);

	FText GetPlayPauseToolTip() const;
	const FSlateBrush* GetPlayPauseIcon() const;

	FText GetFramesTooltipText() const;
	FText GetTimeTooltipText() const;

	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	/* Our Task Container */
	TSharedPtr<SScrollBox> m_TaskList;

	/* Font information for our Tasks. */
	FSlateFontInfo m_FontInfo;

	/* Font Information for our Timeline Bar*/
	FSlateFontInfo m_TimelineFontInfo;

	/* The size of the Tasks last time the widget updated itself. */
	int32 m_CachedTaskSize;

	/* The length of the Ability last time the widget updated itself. */
	float m_CachedTimelineLength;
};

/* Thin wrapper around SScrollbox to allow for custom mouse commands. */
class SAblAbilityTimelineScrollBox : public SScrollBox
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTimelineScrollBox)
		: _AbilityEditor()
	{}

	SLATE_ARGUMENT(TSharedPtr<FAblAbilityEditor>, AbilityEditor)
	SLATE_ARGUMENT(const FSlateBrush*, Brush)
	SLATE_END_ARGS()

	SAblAbilityTimelineScrollBox();

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
private:
	/* Pointer to our Ability Editor instance. */
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;
	const FSlateBrush* m_BackgroundBrush;
};


class SAblAbilityTaskRichTextBlock : public SRichTextBlock
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTaskRichTextBlock)
	{}

	SLATE_ARGUMENT(TWeakPtr<FAblAbilityEditor>, AbilityEditor)
	SLATE_ARGUMENT(TWeakObjectPtr<UAblAbilityTask>, Task)
	SLATE_ARGUMENT(const FSlateBrush*, Brush)
	SLATE_END_ARGS()

	SAblAbilityTaskRichTextBlock();

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	FText GetRichText() const;
private:
	UEdGraph* GetGraphByName(const FName& name) const;

	void OnTaskDependencyClicked(const FSlateHyperlinkRun::FMetadata& Metadata);
	void OnAssetReferenceClicked(const FSlateHyperlinkRun::FMetadata& Metadata);
	void OnGraphReferenceClicked(const FSlateHyperlinkRun::FMetadata& Metadata);
	void OnFloatValueClicked(const FSlateHyperlinkRun::FMetadata& Metadata);
	void OnIntValueClicked(const FSlateHyperlinkRun::FMetadata& Metadata);
	void OnEnumValueClicked(const FSlateHyperlinkRun::FMetadata& Metadata);

	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;
	TWeakObjectPtr<UAblAbilityTask> m_Task;
	const FSlateBrush* m_Brush;
};