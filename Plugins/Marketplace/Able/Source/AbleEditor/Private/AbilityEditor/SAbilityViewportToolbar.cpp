// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityViewportToolbar.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditorCommands.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbilityEditor/SAbilityViewport.h"
#include "Editor/UnrealEd/Public/SEditorViewportViewMenu.h"
#include "Editor/UnrealEd/Public/SEditorViewportToolBarMenu.h"
#include "Editor/UnrealEd/Public/STransformViewportToolbar.h"
#include "EditorViewportCommands.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAbilityEditorViewportToolBar::Construct(const FArguments& InArgs, TSharedPtr<class SAbilityEditorViewportTabBody> InViewport, TSharedPtr<class SEditorViewport> InRealViewport)
{
	m_Viewport = InViewport;
	m_AbilityEditor = InArgs._AbilityEditor;

	TSharedRef<SHorizontalBox> LeftToolbar = SNew(SHorizontalBox)
		// Generic viewport options
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 2.0f)
		[
			//Menu
			SNew(SEditorViewportToolbarMenu)
			.ParentToolBar(SharedThis(this))
			.Image("EditorViewportToolBar.MenuDropdown")
			.OnGetMenuContent(this, &SAbilityEditorViewportToolBar::GenerateViewMenu)
		]

	// Camera Type (Perspective/Top/etc...)
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 2.0f)
		[
			SNew(SEditorViewportToolbarMenu)
			.ParentToolBar(SharedThis(this))
		.Label(this, &SAbilityEditorViewportToolBar::GetCameraMenuLabel)
		.LabelIcon(this, &SAbilityEditorViewportToolBar::GetCameraMenuLabelIcon)
		.OnGetMenuContent(this, &SAbilityEditorViewportToolBar::GenerateViewportTypeMenu)
		]

	// View menu (lit, unlit, etc...)
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 2.0f)
		[
			SNew(SEditorViewportViewMenu, InRealViewport.ToSharedRef(), SharedThis(this))
		]

	+ SHorizontalBox::Slot()
		.Padding(3.0f, 1.0f)
		.HAlign(HAlign_Right)
		[
			SNew(STransformViewportToolBar)
			.Viewport(InRealViewport)
		.CommandList(InRealViewport->GetCommandList())
		.Visibility(this, &SAbilityEditorViewportToolBar::GetTransformToolBarVisibility)
		];

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			// Color and opacity is changed based on whether or not the mouse cursor is hovering over the toolbar area
			//.ColorAndOpacity(this, &SViewportToolBar::OnGetColorAndOpacity)
			.ForegroundColor(FAppStyle::GetSlateColor("DefaultForeground"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[

					LeftToolbar
				]
			]
		];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

TSharedRef<SWidget> SAbilityEditorViewportToolBar::GenerateViewMenu()
{
	const FAblAbilityEditorViewportCommands& Actions = FAblAbilityEditorViewportCommands::Get();

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder ViewMenuBuilder(bInShouldCloseWindowAfterMenuSelection, m_Viewport.Pin()->GetCommandList());
	{
		// View modes
		{
			ViewMenuBuilder.AddMenuEntry(FAblAbilityEditorViewportCommands::Get().m_CameraFollow);
		}
	}

	return ViewMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAbilityEditorViewportToolBar::GenerateViewportTypeMenu()
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder CameraMenuBuilder(bInShouldCloseWindowAfterMenuSelection, m_Viewport.Pin()->GetViewportWidget()->GetCommandList());

	// Camera types
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Perspective);

	CameraMenuBuilder.BeginSection("LevelViewportCameraType_Ortho", LOCTEXT("CameraTypeHeader_Ortho", "Orthographic"));
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Top);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Bottom);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Left);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Right);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Front);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Back);
	CameraMenuBuilder.EndSection();

	return CameraMenuBuilder.MakeWidget();
}

FText SAbilityEditorViewportToolBar::GetCameraMenuLabel() const
{
	FText Label = LOCTEXT("Viewport_Default", "Camera");
	TSharedPtr< SAbilityEditorViewportTabBody > PinnedViewport(m_Viewport.Pin());
	if (PinnedViewport.IsValid())
	{
		switch (PinnedViewport->GetLevelViewportClient()->GetViewportType())
		{
		case LVT_Perspective:
			Label = LOCTEXT("CameraMenuTitle_Perspective", "Perspective");
			break;

		case LVT_OrthoXY:
			Label = LOCTEXT("CameraMenuTitle_Top", "Top");
			break;

		case LVT_OrthoYZ:
			Label = LOCTEXT("CameraMenuTitle_Left", "Left");
			break;

		case LVT_OrthoXZ:
			Label = LOCTEXT("CameraMenuTitle_Front", "Front");
			break;

		case LVT_OrthoNegativeXY:
			Label = LOCTEXT("CameraMenuTitle_Bottom", "Bottom");
			break;

		case LVT_OrthoNegativeYZ:
			Label = LOCTEXT("CameraMenuTitle_Right", "Right");
			break;

		case LVT_OrthoNegativeXZ:
			Label = LOCTEXT("CameraMenuTitle_Back", "Back");
			break;
		case LVT_OrthoFreelook:
			break;
		}
	}

	return Label;
}

const FSlateBrush* SAbilityEditorViewportToolBar::GetCameraMenuLabelIcon() const
{
	FName Icon = NAME_None;
	TSharedPtr< SAbilityEditorViewportTabBody > PinnedViewport(m_Viewport.Pin());
	if (PinnedViewport.IsValid())
	{
		switch (PinnedViewport->GetLevelViewportClient()->GetViewportType())
		{
		case LVT_Perspective:
			Icon = FName("EditorViewport.Perspective");
			break;

		case LVT_OrthoXY:
			Icon = FName("EditorViewport.Top");
			break;

		case LVT_OrthoYZ:
			Icon = FName("EditorViewport.Left");
			break;

		case LVT_OrthoXZ:
			Icon = FName("EditorViewport.Front");
			break;

		case LVT_OrthoNegativeXY:
			Icon = FName("EditorViewport.Bottom");
			break;

		case LVT_OrthoNegativeYZ:
			Icon = FName("EditorViewport.Right");
			break;

		case LVT_OrthoNegativeXZ:
			Icon = FName("EditorViewport.Back");
			break;
		case LVT_OrthoFreelook:
			break;
		}
	}

	return FAppStyle::GetBrush(Icon);
}

EVisibility SAbilityEditorViewportToolBar::GetTransformToolBarVisibility() const
{
	return EVisibility::Visible;
}

float SAbilityEditorViewportToolBar::OnGetFOVValue() const
{
	return m_AbilityEditor.Pin()->GetEditorSettings().m_FOV;
}

void SAbilityEditorViewportToolBar::OnFOVValueChanged(float NewValue)
{
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->GetEditorSettings().m_FOV = NewValue;

		TSharedPtr<FAbilityEditorViewportClient> ViewportClient = StaticCastSharedPtr<FAbilityEditorViewportClient>(m_Viewport.Pin()->GetLevelViewportClient());
		
		if (ViewportClient.IsValid())
		{
			ViewportClient->Invalidate();
		}

	}
}

#undef LOCTEXT_NAMESPACE