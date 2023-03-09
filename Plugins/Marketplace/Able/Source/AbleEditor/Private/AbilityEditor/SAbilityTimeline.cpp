// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityTimeline.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "ablAbilityBlueprint.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/ablAbilityEditorCommands.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbilityEditor/ablAbilityEditorModes.h"
#include "AbilityEditor/SAbilityPropertyDlgs.h"
#include "AbilityEditor/SAbilityTimelineScrubPanel.h"
#include "AbilityEditor/SAbilityTimelineStatus.h"
#include "AbilityEditor/SAbilityTimelineTrack.h"
#include "EdGraph/EdGraph.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/RichTextLayoutMarshaller.h"
#include "Styling/AppStyle.h"
#include "Rendering/SlateRenderer.h"
#include "Tasks/IAblAbilityTask.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

SAblAbilityTimelinePanel::SAblAbilityTimelinePanel()
	: m_AbilityEditor(),
	m_CachedTaskSize(0),
	m_CachedTimelineLength(0.0f)
{

}

void SAblAbilityTimelinePanel::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;

	m_FontInfo = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 8);
	m_TimelineFontInfo = /*FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")); */ FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 16);

	m_TaskList = SNew(SAblAbilityTimelineScrollBox).AbilityEditor(InArgs._AbilityEditor).Brush(FAbleStyle::GetBrush("Able.AbilityEditor.ScrollBackground"));

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.26f)
					.HAlign(HAlign_Fill)
					[
						BuildTimelineControls()
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.74f)
					.HAlign(EHorizontalAlignment::HAlign_Fill)
					[
						SNew(SAblAbilityTimelineScrubPanel)
						.AbilityEditor(InArgs._AbilityEditor)
					]
				]

				+ SVerticalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Fill)
				.Padding(0.0f, 4.0f)
				[
					m_TaskList.ToSharedRef()
				]
			]
		];

	BuildTaskList();

	m_AbilityEditor.Pin()->OnRefresh().AddSP(this, &SAblAbilityTimelinePanel::OnRefresh);
}

void SAblAbilityTimelinePanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (m_AbilityEditor.IsValid())
	{
		if (const UAblAbility* Ability = m_AbilityEditor.Pin()->GetConstAbility())
		{
			const float CurrentLength = Ability->GetLength();
			const int32 CurrentTaskSize = Ability->GetTasks().Num();
			if (m_CachedTaskSize != CurrentTaskSize || FMath::Abs(m_CachedTimelineLength - CurrentLength) > KINDA_SMALL_NUMBER)
			{
				BuildTaskList();
				m_CachedTimelineLength = CurrentLength;
			}
		}
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

FReply SAblAbilityTimelinePanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (!m_AbilityEditor.IsValid())
	{
		return FReply::Unhandled();
	}

	TSharedPtr<FAblAbilityEditor> Editor = m_AbilityEditor.Pin();
	if (InKeyEvent.GetKey() == EKeys::N && InKeyEvent.IsControlDown())
	{
		Editor->AddNewTask();

		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Delete)
	{
		if (const UAblAbilityTask* CurrentTask = Editor->GetCurrentlySelectedAbilityTask())
		{
			Editor->RemoveTask(*CurrentTask);

			return FReply::Handled();
		}
	}
	else if (InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		if (Editor->IsPlayingAbility())
		{
			Editor->PauseAbility();
		}
		else
		{
			Editor->PlayAbility();
		}

		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Right || InKeyEvent.GetKey() == EKeys::Left)
	{
		InKeyEvent.GetKey() == EKeys::Right ? Editor->StepAbility() : Editor->StepAbilityBackwards();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

float SAblAbilityTimelinePanel::RoundFloat(float in, int32 numDecimalPoints) const
{
	if (numDecimalPoints == 0)
	{
		return (float)FMath::RoundToInt(in);
	}
	
	float div = FMath::Pow(10.0f, (float)numDecimalPoints);
	int32 FixedPoint = FMath::RoundToInt(in * div);
	return (float)FixedPoint / div;
}

void SAblAbilityTimelinePanel::OnRefresh()
{
	m_CachedTaskSize = 0;
	m_TaskList->ClearChildren();
}

void SAblAbilityTimelinePanel::BuildTaskList()
{
	if (UAblAbility* Ability = m_AbilityEditor.Pin()->GetAbility())
	{
		// Determine the Padding we'll need to fully display our time range.
		const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		FNumberFormattingOptions FmtOptions;
		FmtOptions.SetMaximumFractionalDigits(2);
		FmtOptions.SetMinimumFractionalDigits(1);

		FText LengthText = FText::AsNumber(Ability->GetLength(), &FmtOptions);
		FVector2D LengthSize = FontMeasureService->Measure(LengthText, m_FontInfo);

		// Our Padding is how much it takes to safely display our largest string.
		const float HPadding = LengthSize.X * 0.5f;

		const FSlateBrush* trackBrushes[2] = { FAbleStyle::GetBrush("Able.AbilityEditor.Track"), FAbleStyle::GetBrush("Able.AbilityEditor.TrackAlt") };
		int brushToUse = 0;
		m_TaskList->ClearChildren();
		const TArray<UAblAbilityTask*>& Tasks = Ability->GetTasks();
		m_TaskList->AddSlot()
			.VAlign(VAlign_Top)
			[
				SNew(SSpacer)
				.Size(FVector2D(0.0f, 6.0f))
			];

		for (UAblAbilityTask* Task : Tasks)
		{
			if (!Task)
			{
				continue;
			}

			m_TaskList->AddSlot()
				.VAlign(VAlign_Top)
				.Padding(HPadding, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.26f)
					[
						SNew(SAblAbilityTaskRichTextBlock)
						.AbilityEditor(m_AbilityEditor)
						.Task(Task)
						.Brush(trackBrushes[brushToUse])
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.74f)
					[
						SNew(SAblAbilityTimelineTrack)
						.AbilityEditor(m_AbilityEditor)
						.AbilityTask(Task)
						.Brush(trackBrushes[brushToUse])
					]
				];

			brushToUse ^= 1;
		}
		
		m_CachedTaskSize = Tasks.Num();
	}
}

TSharedRef<SWidget> SAblAbilityTimelinePanel::BuildTimelineControls()
{
	return
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(1.0f, 2.0f)
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.OnClicked(this, &SAblAbilityTimelinePanel::OnStepBackwards)
			.ToolTipText(LOCTEXT("StepBack", "Step Back"))
			.ContentPadding(2.0f)
			[
				SNew(SImage)
				.Image(FAbleStyle::GetBrush("AblAbilityEditor.m_StepAbilityBackwards"))
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(1.0f, 2.0f)
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.OnClicked(this, &SAblAbilityTimelinePanel::OnStop)
			.ToolTipText(LOCTEXT("Stop", "Stop"))
			.ContentPadding(2.0f)
			[
				SNew(SImage)
				.Image(FAbleStyle::GetBrush("AblAbilityEditor.m_StopAbility"))
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(1.0f, 2.0f)
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.OnClicked(this, &SAblAbilityTimelinePanel::OnPlayOrPause)
			.ToolTipText(this, &SAblAbilityTimelinePanel::GetPlayPauseToolTip)
			.ContentPadding(2.0f)
			[
				SNew(SImage)
				.Image(this, &SAblAbilityTimelinePanel::GetPlayPauseIcon)
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(1.0f, 2.0f)
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.OnClicked(this, &SAblAbilityTimelinePanel::OnStepForwards)
			.ToolTipText(LOCTEXT("StepForward", "Step Forward"))
			.ContentPadding(2.0f)
			[
				SNew(SImage)
				.Image(FAbleStyle::GetBrush("AblAbilityEditor.m_StepAbility"))
			]
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SSpacer)
			.Size(FVector2D(16.0f, 1.0f))
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SNumericEntryBox<float>)
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			.AllowSpin(false)
			.MinValue(this, &SAblAbilityTimelinePanel::GetTimeMin)
			.MaxValue(this, &SAblAbilityTimelinePanel::GetTimeMax)
			.Value(this, &SAblAbilityTimelinePanel::GetCurrentTime)
			.OnValueCommitted(this, &SAblAbilityTimelinePanel::OnTimeValueCommitted)
			.ToolTipText(this, &SAblAbilityTimelinePanel::GetTimeTooltipText)
			.Label()
			[
				SNumericEntryBox<float>::BuildLabel(LOCTEXT("AblTimelineTimeLabel", "Time"), FLinearColor::White, FLinearColor(0.2f, 0.2f, 0.2f))
			]
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SSpacer)
			.Size(FVector2D(16.0f, 1.0f))
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SNumericEntryBox<int>)
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			.AllowSpin(true)
			.MinValue(this, &SAblAbilityTimelinePanel::GetFrameMin)
			.MaxValue(this, &SAblAbilityTimelinePanel::GetFrameMax)
			.Value(this, &SAblAbilityTimelinePanel::GetCurrentFrame)
			.OnValueCommitted(this, &SAblAbilityTimelinePanel::OnFrameValueCommitted)
			.ToolTipText(this, &SAblAbilityTimelinePanel::GetFramesTooltipText)
			.Label()
			[
				SNumericEntryBox<float>::BuildLabel(LOCTEXT("AblTimelineFrameLabel", "Frame"), FLinearColor::White, FLinearColor(0.2f, 0.2f, 0.2f))
			]
		]
	];
}

FReply SAblAbilityTimelinePanel::OnStepBackwards()
{
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->StepAbilityBackwards();
	}

	return FReply::Handled();
}

FReply SAblAbilityTimelinePanel::OnPlay()
{
	if (m_AbilityEditor.IsValid())
	{
		if (m_AbilityEditor.Pin()->IsPlayingAbility())
		{
			m_AbilityEditor.Pin()->PauseAbility();
		}
		else
		{
			m_AbilityEditor.Pin()->PlayAbility();
		}
	}

	return FReply::Handled();
}

FReply SAblAbilityTimelinePanel::OnStop()
{
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->StopAbility();
	}

	return FReply::Handled();
}

FReply SAblAbilityTimelinePanel::OnPlayOrPause()
{
	if (m_AbilityEditor.IsValid())
	{
		if (m_AbilityEditor.Pin()->IsPlayingAbility())
		{
			m_AbilityEditor.Pin()->PauseAbility();
		}
		else
		{
			m_AbilityEditor.Pin()->PlayAbility();
		}
	}

	return FReply::Handled();
}

FReply SAblAbilityTimelinePanel::OnStepForwards()
{
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->StepAbility();
	}

	return FReply::Handled();
}

void SAblAbilityTimelinePanel::OnTimeValueCommitted(float NewValue, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		if (m_AbilityEditor.IsValid())
		{
			m_AbilityEditor.Pin()->SetAbilityPreviewTime(NewValue);
		}
	}
}

void SAblAbilityTimelinePanel::OnFrameValueCommitted(int NewValue, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		if (m_AbilityEditor.IsValid())
		{
			const UAblAbilityEditorSettings& Settings = m_AbilityEditor.Pin()->GetEditorSettings();
			m_AbilityEditor.Pin()->SetAbilityPreviewTime(((float)NewValue) *  Settings.GetAbilityTimeStepDelta());
		}
	}
}

FText SAblAbilityTimelinePanel::GetPlayPauseToolTip() const
{
	if (m_AbilityEditor.IsValid() && m_AbilityEditor.Pin()->IsPlayingAbility())
	{
		return LOCTEXT("Pause", "Pause");
	}

	return LOCTEXT("Play", "Play");
}

const FSlateBrush* SAblAbilityTimelinePanel::GetPlayPauseIcon() const
{
	if (m_AbilityEditor.IsValid() && m_AbilityEditor.Pin()->IsPlayingAbility())
	{
		return FAbleStyle::GetBrush("AblAbilityEditor.m_PauseAbility");
	}

	return FAbleStyle::GetBrush("AblAbilityEditor.m_PlayAbility");
}

FText SAblAbilityTimelinePanel::GetFramesTooltipText() const
{
	int CurrentFrame = 0;
	int MaxFrames = 0;
	if (m_AbilityEditor.IsValid())
	{
		const UAblAbilityEditorSettings& Settings = m_AbilityEditor.Pin()->GetEditorSettings();
		CurrentFrame = m_AbilityEditor.Pin()->GetAbilityCurrentTime() / Settings.GetAbilityTimeStepDelta();
		MaxFrames = m_AbilityEditor.Pin()->GetAbilityLength() / Settings.GetAbilityTimeStepDelta();
	}

	return 	FText::FormatOrdered(LOCTEXT("AblFrameTooltipFormat", "Frame {0} / {1}"), FText::AsNumber(CurrentFrame), FText::AsNumber(MaxFrames));
}

FText SAblAbilityTimelinePanel::GetTimeTooltipText() const
{
	float maxTime = 0.0f;
	float currentTime = 0.0f;
	FNumberFormattingOptions FormattingOptions = FNumberFormattingOptions().SetMaximumFractionalDigits(2);

	if (m_AbilityEditor.IsValid())
	{
		currentTime = m_AbilityEditor.Pin()->GetAbilityCurrentTime();
		maxTime = m_AbilityEditor.Pin()->GetAbilityLength();
	}

	return 	FText::FormatOrdered(LOCTEXT("AblTimeTooltipFormat", "Time {0} / {1}"), FText::AsNumber(currentTime, &FormattingOptions), FText::AsNumber(maxTime, &FormattingOptions));
}

TOptional<float> SAblAbilityTimelinePanel::GetTimeMin() const
{
	if (m_AbilityEditor.IsValid())
	{
		return RoundFloat(m_AbilityEditor.Pin()->GetAbilityTimeRange().X, 3);
	}

	return 0.0f;
}

TOptional<float> SAblAbilityTimelinePanel::GetTimeMax() const
{
	if (m_AbilityEditor.IsValid())
	{
		
		return RoundFloat(m_AbilityEditor.Pin()->GetAbilityTimeRange().Y, 3);
	}

	return 0.0f;
}

TOptional<float> SAblAbilityTimelinePanel::GetCurrentTime() const
{
	if (m_AbilityEditor.IsValid())
	{
		return RoundFloat(m_AbilityEditor.Pin()->GetAbilityCurrentTime(), 3);
	}

	return 0.0f;
}

TOptional<int> SAblAbilityTimelinePanel::GetCurrentFrame() const
{
	if (m_AbilityEditor.IsValid())
	{
		const UAblAbilityEditorSettings& Settings = m_AbilityEditor.Pin()->GetEditorSettings();
		return m_AbilityEditor.Pin()->GetAbilityCurrentTime() / Settings.GetAbilityTimeStepDelta();
	}

	return 0;
}

TOptional<int> SAblAbilityTimelinePanel::GetFrameMin() const
{
	if (m_AbilityEditor.IsValid())
	{
		const UAblAbilityEditorSettings& Settings = m_AbilityEditor.Pin()->GetEditorSettings();
		return m_AbilityEditor.Pin()->GetAbilityTimeRange().X / Settings.GetAbilityTimeStepDelta();
	}

	return 0;
}

TOptional<int> SAblAbilityTimelinePanel::GetFrameMax() const
{
	if (m_AbilityEditor.IsValid())
	{
		const UAblAbilityEditorSettings& Settings = m_AbilityEditor.Pin()->GetEditorSettings();
		return m_AbilityEditor.Pin()->GetAbilityTimeRange().Y / Settings.GetAbilityTimeStepDelta();
	}

	return 0;
}

SAblAbilityTimelineScrollBox::SAblAbilityTimelineScrollBox()
{

}

void SAblAbilityTimelineScrollBox::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;
	m_BackgroundBrush = InArgs._Brush;

	SScrollBox::Construct(SScrollBox::FArguments().Orientation(Orient_Vertical).ScrollBarAlwaysVisible(true));
}

int32 SAblAbilityTimelineScrollBox::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FSlateDrawElement::MakeBox(OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		m_BackgroundBrush);

	return SScrollBox::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

FReply SAblAbilityTimelineScrollBox::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (m_AbilityEditor.IsValid())
		{
			TSharedRef<SWidget> MenuContents = m_AbilityEditor.Pin()->GenerateTaskContextMenu();
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuContents, MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
		}
	}

	return SScrollBox::OnMouseButtonUp(MyGeometry, MouseEvent);
}

SAblAbilityTaskRichTextBlock::SAblAbilityTaskRichTextBlock()
{

}

void SAblAbilityTaskRichTextBlock::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;
	m_Task = InArgs._Task;
	m_Brush = InArgs._Brush;
	SRichTextBlock::FArguments Args;
	Args.AutoWrapText(true)
	.Text(this, &SAblAbilityTaskRichTextBlock::GetRichText)
	.Margin(FMargin(4.0f, 4.0f));

	TSharedRef<FRichTextLayoutMarshaller> RichTextMarshaller = FRichTextLayoutMarshaller::Create(
		TArray<TSharedRef<ITextDecorator>>(),
		FAbleStyle::Get().Get()
	);

	RichTextMarshaller->AppendInlineDecorator(SRichTextBlock::HyperlinkDecorator(TEXT("AblTextDecorators.TaskDependency"), this, &SAblAbilityTaskRichTextBlock::OnTaskDependencyClicked));
	RichTextMarshaller->AppendInlineDecorator(SRichTextBlock::HyperlinkDecorator(TEXT("AblTextDecorators.AssetReference"), this, &SAblAbilityTaskRichTextBlock::OnAssetReferenceClicked));
	RichTextMarshaller->AppendInlineDecorator(SRichTextBlock::HyperlinkDecorator(TEXT("AblTextDecorators.GraphReference"), this, &SAblAbilityTaskRichTextBlock::OnGraphReferenceClicked));
	RichTextMarshaller->AppendInlineDecorator(SRichTextBlock::HyperlinkDecorator(TEXT("AblTextDecorators.FloatValue"), this, &SAblAbilityTaskRichTextBlock::OnFloatValueClicked));
	RichTextMarshaller->AppendInlineDecorator(SRichTextBlock::HyperlinkDecorator(TEXT("AblTextDecorators.IntValue"), this, &SAblAbilityTaskRichTextBlock::OnIntValueClicked));

	Args.Marshaller(RichTextMarshaller);
	Args.TextStyle(FAbleStyle::Get(), "RichText.Text");
	Args.DecoratorStyleSet(FAbleStyle::Get().Get());

	SRichTextBlock::Construct(Args);
}

int32 SAblAbilityTaskRichTextBlock::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FSlateDrawElement::MakeBox(OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		m_Brush);

	return SRichTextBlock::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

FText SAblAbilityTaskRichTextBlock::GetRichText() const
{
	if (m_Task.IsValid())
	{
		return m_Task->GetRichTextTaskSummary();
	}

	return FText::GetEmpty();
}

UEdGraph* SAblAbilityTaskRichTextBlock::GetGraphByName(const FName& name) const
{
	if (UAblAbilityBlueprint* blueprint = m_AbilityEditor.Pin()->GetAbilityBlueprint())
	{
		for (UEdGraph* graph : blueprint->FunctionGraphs)
		{
			if (graph->GetFName() == name)
			{
				return graph;
			}
		}
	}

	return nullptr;
}

void SAblAbilityTaskRichTextBlock::OnTaskDependencyClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString IndexKey = TEXT("Index");
	if (Metadata.Contains(IndexKey))
	{
		FString IndexValue = Metadata.FindChecked(IndexKey);
		int Index = FPlatformString::Atoi(*IndexValue);

		const TArray<const UAblAbilityTask*>& Dependences = m_Task->GetTaskDependencies();
		if (Index < Dependences.Num() && Dependences[Index] != nullptr)
		{
			m_AbilityEditor.Pin()->SetCurrentTask(Dependences[Index]);
		}
	}
}

void SAblAbilityTaskRichTextBlock::OnAssetReferenceClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	TArray<FTopLevelAssetPath> Filter;
	const FString FilterKey = TEXT("Filter");
	if (Metadata.Contains(FilterKey))
	{
		FString allFilters = Metadata.FindChecked(FilterKey);
		int32 commaIndex = INDEX_NONE;
		TArray<FString> FilterArray;
		allFilters.ParseIntoArray(FilterArray, TEXT(","));
		for (const FString& filterEntry : FilterArray)
		{
			FTopLevelAssetPath filterPath(filterEntry);
			if (filterPath.IsValid())
			{
				Filter.Add(filterPath);
			}
		}
	}

	FName PropertyName(*Metadata.FindChecked(TEXT("PropertyName")));
	FProperty* Property = m_Task->GetClass()->FindPropertyByName(PropertyName);

	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		UObject* currentObject = ObjectProperty->GetObjectPropertyValue_InContainer(m_Task.Get());
		TSharedPtr<SAblAbilityAssetPropertyDlg> PropertyDlg = SNew(SAblAbilityAssetPropertyDlg)
			.DefaultValue(FAssetData(currentObject))
			.AssetFilter(Filter)
			.PropertyLabel(Property->GetDisplayNameText());

		if (PropertyDlg->DoModal())
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(m_Task.Get(), PropertyDlg->GetValue().GetAsset());
		}
	}

}

void SAblAbilityTaskRichTextBlock::OnGraphReferenceClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString GraphNameKey = TEXT("GraphName");
	if (Metadata.Contains(GraphNameKey))
	{
		FString GraphName = Metadata.FindChecked(GraphNameKey);

		if (UEdGraph* Graph = GetGraphByName(FName(*GraphName)))
		{
			m_AbilityEditor.Pin()->SetCurrentMode(FAblAbilityEditorModes::AbilityBlueprintMode);
			m_AbilityEditor.Pin()->OpenDocument(Graph, FDocumentTracker::OpenNewDocument);
		}
	}
}

void SAblAbilityTaskRichTextBlock::OnFloatValueClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	FName PropertyName(*Metadata.FindChecked(TEXT("PropertyName")));
	FProperty* Property = m_Task->GetClass()->FindPropertyByName(PropertyName);

	if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		float minValue = 0.0f;
		float maxValue = FLT_MAX;

		if (const FString* MinValueString = Metadata.Find(TEXT("MinValue")))
		{
			minValue = FPlatformString::Atof(*(*MinValueString));
		}

		if (const FString* MaxValueString = Metadata.Find(TEXT("MaxValue")))
		{
			maxValue = FPlatformString::Atof(*(*MaxValueString));
		}

		float currentValue = FloatProperty->GetPropertyValue_InContainer(m_Task.Get());
		TSharedPtr<SAblAbilityNumericPropertyDlg<float>> PropertyDlg = SNew(SAblAbilityNumericPropertyDlg<float>)
			.DefaultValue(currentValue)
			.MinValue(minValue)
			.MaxValue(maxValue)
			.ShowSpinner(false)
			.PropertyLabel(FloatProperty->GetDisplayNameText());

		if (PropertyDlg->DoModal())
		{
			FloatProperty->SetPropertyValue_InContainer(m_Task.Get(), PropertyDlg->GetValue());
		}
	}
}

void SAblAbilityTaskRichTextBlock::OnIntValueClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	FName PropertyName(*Metadata.FindChecked(TEXT("PropertyName")));
	FProperty* Property = m_Task->GetClass()->FindPropertyByName(PropertyName);

	if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
	{
		int minValue = 0;
		int maxValue = INT_MAX;

		if (const FString* MinValueString = Metadata.Find(TEXT("MinValue")))
		{
			minValue = FPlatformString::Atoi(*(*MinValueString));
		}

		if (const FString* MaxValueString = Metadata.Find(TEXT("MaxValue")))
		{
			maxValue = FPlatformString::Atoi(*(*MaxValueString));
		}

		int currentValue = IntProperty->GetPropertyValue_InContainer(m_Task.Get());
		TSharedPtr<SAblAbilityNumericPropertyDlg<int>> PropertyDlg = SNew(SAblAbilityNumericPropertyDlg<int>)
			.DefaultValue(currentValue)
			.MinValue(minValue)
			.MaxValue(maxValue)
			.ShowSpinner(true)
			.PropertyLabel(IntProperty->GetDisplayNameText());

		if (PropertyDlg->DoModal())
		{
			UE_LOG(LogAbleEditor, Warning, TEXT("Setting Task value to %d. "), PropertyDlg->GetValue());
			IntProperty->SetPropertyValue_InContainer(m_Task.Get(), PropertyDlg->GetValue());
		}
	}
}

void SAblAbilityTaskRichTextBlock::OnEnumValueClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	FName PropertyName(*Metadata.FindChecked(TEXT("PropertyName")));
	FProperty* Property = m_Task->GetClass()->FindPropertyByName(PropertyName);

	if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		int64* value = EnumProperty->GetUnderlyingProperty()->ContainerPtrToValuePtr<int64>(m_Task.Get());
		int32 index = EnumProperty->GetEnum()->GetIndexByValue(*value);
		if (index != INDEX_NONE)
		{
			TSharedPtr<SAblAbilityEnumPropertyDlg> PropertyDlg = SNew(SAblAbilityEnumPropertyDlg)
				.EnumClass(EnumProperty->GetEnum())
				.InitialSelectionIndex(index)
				.PropertyLabel(EnumProperty->GetDisplayNameText());

			if (PropertyDlg->DoModal())
			{
				*value = EnumProperty->GetEnum()->GetValueByIndex(PropertyDlg->GetValue());
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE