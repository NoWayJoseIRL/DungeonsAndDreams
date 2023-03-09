// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/ablAbilityBlueprintFactory.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityGraph.h"
#include "AbilityEditor/ablAbilityGraphSchema.h"
#include "ablAbility.h"
#include "ablAbilityBlueprint.h"
#include "ablAbilityBlueprintGeneratedClass.h"
#include "ablSettings.h"
#include "BlueprintEditorSettings.h"
#include "ClassViewerModule.h" // This has to be before ClassViewFilter
#include "ClassViewerFilter.h"
#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Misc/MessageDialog.h"
#include "Preferences/UnrealEdOptions.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "UAblAbilitiesBlueprintFactory"

/*------------------------------------------------------------------------------
Dialog to configure creation properties
------------------------------------------------------------------------------*/
class FAblAbilityBlueprintParentFilter : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet< const UClass* > m_AllowedChildrenOfClasses;

	FAblAbilityBlueprintParentFilter() {}

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		// If it appears on the allowed child-of classes list (or there is nothing on that list)
		return InFilterFuncs->IfInChildOfClassesSet(m_AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		// If it appears on the allowed child-of classes list (or there is nothing on that list)
		return InFilterFuncs->IfInChildOfClassesSet(m_AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};

class SAblAbilityBlueprintCreateDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityBlueprintCreateDialog) {}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs)
	{
		m_bOkClicked = false;
		m_ParentClass = UAblAbility::StaticClass();

		ChildSlot
			[
				SNew(SBorder)
				.Visibility(EVisibility::Visible)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
			[
				SNew(SBox)
				.Visibility(EVisibility::Visible)
			.WidthOverride(500.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
			.FillHeight(1)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Content()
			[
				SAssignNew(m_ParentClassContainer, SVerticalBox)
			]
			]

		// Ok/Cancel buttons
		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(8)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
			.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
			.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
			+ SUniformGridPanel::Slot(0, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
			.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
			.OnClicked(this, &SAblAbilityBlueprintCreateDialog::OkClicked)
			.Text(LOCTEXT("CreateAblAbilityBlueprintOk", "OK"))
			]
		+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
			.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
			.OnClicked(this, &SAblAbilityBlueprintCreateDialog::CancelClicked)
			.Text(LOCTEXT("CreateAblAbilityBlueprintCancel", "Cancel"))
			]
			]
			]
			]
			];

		MakeParentClassPicker();
	}

	/** Sets properties for the supplied AblAbilityBlueprintFactory */
	bool ConfigureProperties(TWeakObjectPtr<UAblAbilityBlueprintFactory> InAblAbilityBlueprintFactory)
	{
		m_AblAbilityBlueprintFactory = InAblAbilityBlueprintFactory;

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("CreateAblAbilityBlueprintOptions", "Create Able Ability Blueprint"))
			.ClientSize(FVector2D(400, 700))
			.SupportsMinimize(false).SupportsMaximize(false)
			[
				AsShared()
			];

		m_PickerWindow = Window;

		GEditor->EditorAddModalWindow(Window);
		m_AblAbilityBlueprintFactory.Reset();

		return m_bOkClicked;
	}

private:
	/** Creates the combo menu for the parent class */
	void MakeParentClassPicker()
	{
		// Load the classviewer module to display a class picker
		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		// Fill in options
		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;

		// Only allow parenting to base blueprints.
		Options.bIsBlueprintBaseOnly = true;

		TSharedPtr<FAblAbilityBlueprintParentFilter> Filter = MakeShareable(new FAblAbilityBlueprintParentFilter);

		// All child child classes of UAblAbility are valid.
		Filter->m_AllowedChildrenOfClasses.Add(UAblAbility::StaticClass());
		Options.ClassFilters.Add(Filter.ToSharedRef());

		m_ParentClassContainer->ClearChildren();
		m_ParentClassContainer->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ParentClass", "Parent Class:"))
			.ShadowOffset(FVector2D(1.0f, 1.0f))
			];

		m_ParentClassContainer->AddSlot()
			[
				ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SAblAbilityBlueprintCreateDialog::OnClassPicked))
			];
	}

	/** Handler for when a parent class is selected */
	void OnClassPicked(UClass* ChosenClass)
	{
		m_ParentClass = ChosenClass;
	}

	/** Handler for when ok is clicked */
	FReply OkClicked()
	{
		if (m_AblAbilityBlueprintFactory.IsValid())
		{
			m_AblAbilityBlueprintFactory->BlueprintType = BPTYPE_Normal;
			m_AblAbilityBlueprintFactory->ParentClass = m_ParentClass.Get();
		}

		CloseDialog(true);

		return FReply::Handled();
	}

	void CloseDialog(bool bWasPicked = false)
	{
		m_bOkClicked = bWasPicked;
		if (m_PickerWindow.IsValid())
		{
			m_PickerWindow.Pin()->RequestDestroyWindow();
		}
	}

	/** Handler for when cancel is clicked */
	FReply CancelClicked()
	{
		CloseDialog();
		return FReply::Handled();
	}

	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			CloseDialog();
			return FReply::Handled();
		}
		return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}

private:
	/** The factory for which we are setting up properties */
	TWeakObjectPtr<UAblAbilityBlueprintFactory> m_AblAbilityBlueprintFactory;

	/** A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> m_PickerWindow;

	/** The container for the Parent Class picker */
	TSharedPtr<SVerticalBox> m_ParentClassContainer;

	/** The selected class */
	TWeakObjectPtr<UClass> m_ParentClass;

	/** True if Ok was clicked */
	bool m_bOkClicked;
};


UAblAbilityBlueprintFactory::UAblAbilityBlueprintFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UAblAbilityBlueprint::StaticClass();
	ParentClass = UAblAbility::StaticClass();
}

UAblAbilityBlueprintFactory::~UAblAbilityBlueprintFactory()
{

}

bool UAblAbilityBlueprintFactory::ConfigureProperties()
{
	TSharedRef<SAblAbilityBlueprintCreateDialog> Dialog = SNew(SAblAbilityBlueprintCreateDialog);
	return Dialog->ConfigureProperties(this);
};

UObject* UAblAbilityBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Make sure we are trying to factory a gameplay ability blueprint, then create and init one
	check(Class->IsChildOf(UAblAbilityBlueprint::StaticClass()));

	// If they selected an interface, force the parent class to be UInterface
	if (BlueprintType == BPTYPE_Interface)
	{
		ParentClass = UInterface::StaticClass();
	}

	if ((ParentClass == NULL) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(UAblAbility::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != NULL) ? FText::FromString(ParentClass->GetName()) : LOCTEXT("Null", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CannotCreateAblAbilityBlueprint", "Cannot create a Able Ability Blueprint based on the class '{ClassName}'."), Args));
		return NULL;
	}
	else
	{
		UAblAbilityBlueprint* NewBP = CastChecked<UAblAbilityBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BlueprintType, UAblAbilityBlueprint::StaticClass(), UAblAbilityBlueprintGeneratedClass::StaticClass(), CallingContext));

		if (NewBP)
		{
			UAblAbilityBlueprint* AbilityBP = UAblAbilityBlueprint::FindRootGameplayAbilityBlueprint(NewBP);
			if (AbilityBP == NULL)
			{
				const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

				// Only allow an ability graph if there isn't one in a parent blueprint
				UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(NewBP, TEXT("Able Ability Graph"), UAblAbilityGraph::StaticClass(), UAblAbilityGraphSchema::StaticClass());
#if WITH_EDITORONLY_DATA
				if (NewBP->UbergraphPages.Num())
				{
					FBlueprintEditorUtils::RemoveGraphs(NewBP, NewBP->UbergraphPages);
				}
#endif
				FBlueprintEditorUtils::AddUbergraphPage(NewBP, NewGraph);
				NewBP->LastEditedDocuments.Add(NewGraph);
				NewGraph->bAllowDeletion = false;

				UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();
				if (Settings && Settings->bSpawnDefaultBlueprintNodes)
				{
					int32 NodePositionY = 0;
					
					// Just stick to the core ones. Other events are task dependent. 
					FKismetEditorUtilities::AddDefaultEventNode(NewBP, NewGraph, FName(TEXT("OnAbilityStartBP")), UAblAbility::StaticClass(), NodePositionY);
					FKismetEditorUtilities::AddDefaultEventNode(NewBP, NewGraph, FName(TEXT("OnAbilityEndBP")), UAblAbility::StaticClass(), NodePositionY);
					FKismetEditorUtilities::AddDefaultEventNode(NewBP, NewGraph, FName(TEXT("OnAbilityInterruptBP")), UAblAbility::StaticClass(), NodePositionY);
					FKismetEditorUtilities::AddDefaultEventNode(NewBP, NewGraph, FName(TEXT("OnAbilityBranchBP")), UAblAbility::StaticClass(), NodePositionY);
				}
			}
		}

		if (UAblAbility* ParentObj = Cast<UAblAbility>(ParentClass->GetDefaultObject()))
		{
			UAbleSettings* AblSettings = GetMutableDefault<UAbleSettings>();

			if (AblSettings && AblSettings->GetCopyInheritedTasks())
			{
				if (*NewBP->GeneratedClass)
				{
					if (UAblAbility* NewAbility = Cast<UAblAbility>(NewBP->GeneratedClass->GetDefaultObject()))
					{
						NewAbility->CopyInheritedTasks(*ParentObj);
					}
				}
			}
			else
			{
				// Make sure our Tasks are available outside of the package.
				ParentObj->MarkTasksAsPublic();
			}
		}

		return NewBP;
	}
}

UObject* UAblAbilityBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

#undef LOCTEXT_NAMESPACE
