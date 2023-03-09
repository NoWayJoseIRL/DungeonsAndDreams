// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#include "SMSSViewportToolBarComboMenu.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Input/SButton.h"
#include "EditorStyleSet.h"

void SMSSViewportToolBarComboMenu::Construct( const FArguments& InArgs )
{
#if ENGINE_MAJOR_VERSION == 5 

	EMultiBlockLocation::Type BlockLocation = InArgs._BlockLocation;

//	const FButtonStyle& ButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("EditorViewportToolBar.ComboMenu.ButtonStyle");
//	const FCheckBoxStyle& CheckBoxStyle = FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ComboMenu.ToggleButton");
//	const FTextBlockStyle& LabelStyle = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("EditorViewportToolBar.ComboMenu.LabelStyle");

	const FButtonStyle& ButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("EditorViewportToolBar.ComboMenu.ButtonStyle");
	const FCheckBoxStyle& CheckBoxStyle = FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ToggleButton.Start");
	const FTextBlockStyle& LabelStyle = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("EditorViewportToolBar.ComboMenu.LabelStyle");

	const FSlateIcon& Icon = InArgs._Icon.Get();

	TSharedPtr<SCheckBox> ToggleControl;
	{
		ToggleControl = SNew(SCheckBox)
			//.Padding(0.f)
			.Style(&CheckBoxStyle)
			.OnCheckStateChanged(InArgs._OnCheckStateChanged)
			.ToolTipText(InArgs._ToggleButtonToolTip)
			.IsChecked(InArgs._IsChecked)
			[
				SNew(SImage)
				.Image(Icon.GetIcon())
				.ColorAndOpacity(FSlateColor::UseForeground())
			
			];
	}

	{
		TSharedRef<SWidget> ButtonContents =
			SNew(SButton)
			.ButtonStyle(&ButtonStyle)
			//.ContentPadding(0.f)
			.ToolTipText(InArgs._MenuButtonToolTip)
			.OnClicked(this, &SMSSViewportToolBarComboMenu::OnMenuClicked)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&LabelStyle)
				.Text(InArgs._Label)
				

			];

		if (InArgs._MinDesiredButtonWidth > 0.0f)
		{
			ButtonContents =
				SNew(SBox)
				.MinDesiredWidth(InArgs._MinDesiredButtonWidth > 0.0f ? InArgs._MinDesiredButtonWidth : FOptionalSize())
				[
					ButtonContents
				];
		}

		MenuAnchor = SNew(SMenuAnchor)
			.Placement(MenuPlacement_BelowAnchor)
			[
				ButtonContents
			]
		.OnGetMenuContent(InArgs._OnGetMenuContent);
	}


	//ChildSlot
	//	[

	//		SNew(SBorder)
	//		.Padding(FMargin(6.f, 0.f, 6.f, 0.f))
	//	.BorderImage(FAppStyle::Get().GetBrush("EditorViewportToolBar.Group"))
	//	[
	//		SNew(SHorizontalBox)

	//		+ SHorizontalBox::Slot()
	//	.VAlign(VAlign_Center)
	//	.Padding(2.0f, 0.0f)
	//	.AutoWidth()
	//	[
	//		ToggleControl.ToSharedRef()
	//	]

	//+ SHorizontalBox::Slot()
	//	.Padding(2.0f, 0.0f)
	//	.VAlign(VAlign_Center)
	//	.AutoWidth()
	//	[
	//		MenuAnchor.ToSharedRef()
	//	]
	//	]
	//	];


	ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			ToggleControl.ToSharedRef()
		]

	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			MenuAnchor.ToSharedRef()
		]
		];

#else
	FName ButtonStyle = FEditorStyle::Join(InArgs._Style.Get(), ".Button");
	FName CheckboxStyle = FEditorStyle::Join(InArgs._Style.Get(), ".ToggleButton");

	const FSlateIcon& Icon = InArgs._Icon.Get();

	//ParentToolBar = InArgs._ParentToolBar;

	TSharedPtr<SCheckBox> ToggleControl;
	{
		ToggleControl = SNew(SCheckBox)
		.Cursor( EMouseCursor::Default )
		.Padding(FMargin( 4.0f ))
		.Style(FEditorStyle::Get(), EMultiBlockLocation::ToName(CheckboxStyle, InArgs._BlockLocation))
		.OnCheckStateChanged(InArgs._OnCheckStateChanged)
		.ToolTipText(InArgs._ToggleButtonToolTip)
		.IsChecked(InArgs._IsChecked)
		.Content()
		[
			SNew( SBox )
			.WidthOverride( 16 )
			.HeightOverride( 16 )
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(Icon.GetIcon())
			]
		];
	}

	{
		TSharedRef<SWidget> ButtonContents =
			SNew(SButton)
			.ButtonStyle( FEditorStyle::Get(), EMultiBlockLocation::ToName(ButtonStyle,EMultiBlockLocation::End) )
			.ContentPadding( FMargin( 5.0f, 0.0f ) )
			.ToolTipText(InArgs._MenuButtonToolTip)
			.OnClicked(this, &SMSSViewportToolBarComboMenu::OnMenuClicked)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.TextStyle( FEditorStyle::Get(), FEditorStyle::Join( InArgs._Style.Get(), ".Label" ) )
					.Text(InArgs._Label)
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Bottom)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(1.0f)

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SBox )
						.WidthOverride( 4 )
						.HeightOverride( 4 )
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
							.ColorAndOpacity(FLinearColor::Black)
						]
					]

					+SHorizontalBox::Slot()
						.FillWidth(1.0f)
				]
			];
		
		if (InArgs._MinDesiredButtonWidth > 0.0f)
		{
			ButtonContents =
				SNew(SBox)
				.MinDesiredWidth(InArgs._MinDesiredButtonWidth)
				[
					ButtonContents
				];
		}

		MenuAnchor = SNew(SMenuAnchor)
			
		.Placement( MenuPlacement_BelowRightAnchor )
		[
			ButtonContents
		]
		.OnGetMenuContent( InArgs._OnGetMenuContent );
	}


	ChildSlot
	[
		SNew(SHorizontalBox)

		//Checkbox concept
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			ToggleControl->AsShared()
		]

		// Black Separator line
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBorder)
			.Padding(FMargin(1.0f, 0.0f, 0.0f, 0.0f))
			.BorderImage(FEditorStyle::GetDefaultBrush())
			.BorderBackgroundColor(FLinearColor::Black)
		]

		// Menu dropdown concept
		+SHorizontalBox::Slot()
		.AutoWidth()
			
		[
			MenuAnchor->AsShared()
		]
	];
#endif
}

FReply SMSSViewportToolBarComboMenu::OnMenuClicked()
{
	// If the menu button is clicked toggle the state of the menu anchor which will open or close the menu
	MenuAnchor->SetIsOpen( !MenuAnchor->IsOpen() );
	//ParentToolBar.Pin()->SetOpenMenu( MenuAnchor );
	return FReply::Handled();
}

void SMSSViewportToolBarComboMenu::OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// See if there is another menu on the same tool bar already open
	//TWeakPtr<SMenuAnchor> OpenedMenu = ParentToolBar.Pin()->GetOpenMenu();
//	if( OpenedMenu.IsValid() && OpenedMenu.Pin()->IsOpen() && OpenedMenu.Pin() != MenuAnchor )
//	{
		// There is another menu open so we open this menu and close the other
		//ParentToolBar.Pin()->SetOpenMenu( MenuAnchor ); 
//		MenuAnchor->SetIsOpen( true );
//	}
}
