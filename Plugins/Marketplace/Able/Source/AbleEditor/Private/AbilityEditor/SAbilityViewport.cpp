// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityViewport.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditorViewportClient.h"
#include "AbilityEditor/SAbilityViewportToolbar.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandList.h"

#include "Runtime/Engine/Public/Slate/SceneViewport.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Viewports.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

const FName FAblAbilityEditorViewportSummoner::ID(TEXT("AblAbilityViewport"));

SAbilityEditorViewport::~SAbilityEditorViewport()
{

}

void SAbilityEditorViewport::Construct(const FArguments& InArgs, TSharedPtr<class FAblAbilityEditor> InAbilityEditor, TSharedPtr<class SAbilityEditorViewportTabBody> InTabBody)
{
	m_AbilityEditor = InAbilityEditor;
	m_TabBodyPtr = InTabBody;

	SEditorViewport::Construct(
		SEditorViewport::FArguments()
		.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
		.AddMetaData<FTagMetaData>(TEXT("Able.AbilityEditor.Viewport"))
	);

	Client->VisibilityDelegate.BindSP(this, &SAbilityEditorViewport::IsVisible);
}

TSharedRef<FEditorViewportClient> SAbilityEditorViewport::MakeEditorViewportClient()
{
	m_LevelViewportClient = MakeShareable(new FAbilityEditorViewportClient(m_AbilityEditor.Pin()->GetPreviewScene(), m_AbilityEditor, SharedThis(this)));

	m_LevelViewportClient->ViewportType = LVT_Perspective;
	m_LevelViewportClient->bSetListenerPosition = false;
	m_LevelViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
	m_LevelViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
	m_LevelViewportClient->SetRealtime(true);
	m_LevelViewportClient->SetShowStats(true);

	return m_LevelViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SAbilityEditorViewport::MakeViewportToolbar()
{
	return SNew(SAbilityEditorViewportToolBar, m_TabBodyPtr.Pin(), SharedThis(this))
		.Cursor(EMouseCursor::Default)
		.AbilityEditor(m_AbilityEditor.Pin());
}

SAbilityEditorViewportTabBody::SAbilityEditorViewportTabBody()
{

}

SAbilityEditorViewportTabBody::~SAbilityEditorViewportTabBody()
{

}

void SAbilityEditorViewportTabBody::Construct(const FArguments& InArgs)
{
	m_AbilityEditor = InArgs._AbilityEditor;

	check(m_AbilityEditor.IsValid());
	
	m_Commands = MakeShareable(new FUICommandList);

	FAblAbilityEditorViewportCommands::Register();

	m_ViewportWidget = SNew(SAbilityEditorViewport, InArgs._AbilityEditor, SharedThis(this));
	m_LevelViewportClient = m_ViewportWidget->GetViewportClient();

	TSharedPtr<SVerticalBox> ViewportContainer = nullptr;
	this->ChildSlot
		[
			SAssignNew(ViewportContainer, SVerticalBox)

			+ SVerticalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Fill)
			.HAlign(EHorizontalAlignment::HAlign_Fill)
			[
				SNew(SOverlay)

				// The viewport
				+ SOverlay::Slot()
				[
					m_ViewportWidget.ToSharedRef()
				]
			]
		];

	TWeakPtr<SAbilityEditorViewportTabBody> WeakTabBody = SharedThis(this);
	m_AbilityEditor.Pin()->SetViewportWidget(WeakTabBody);
}

void SAbilityEditorViewportTabBody::RefreshViewport()
{
	m_ViewportWidget->Invalidate();
}

void SAbilityEditorViewportTabBody::CaptureThumbnail() const
{
	m_ViewportWidget->GetAbilityViewportClient()->CaptureThumbnail();
}

FAblAbilityEditorViewportSummoner::FAblAbilityEditorViewportSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	:FWorkflowTabFactory(FAblAbilityEditorViewportSummoner::ID, InHostingApp)
{
	TabLabel = LOCTEXT("AblAbilityViewport_TabTitle", "Viewport");
	TabIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), "Able.Tabs.Viewport");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AblAbilityViewport_MenuTitle", "Viewport");
	ViewMenuTooltip = LOCTEXT("AblAbilityViewport_MenuToolTip", "Shows the in-game Viewport");
}

TSharedRef<SWidget> FAblAbilityEditorViewportSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FAblAbilityEditor> AbilityEditor = StaticCastSharedPtr<FAblAbilityEditor>(HostingApp.Pin());

	return SNew(SAbilityEditorViewportTabBody).AbilityEditor(AbilityEditor);
}

#undef LOCTEXT_NAMESPACE