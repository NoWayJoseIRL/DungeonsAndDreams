// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityPropertyBinding.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"

#if WITH_EDITOR
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#endif // WITH_EDITOR

#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"

#include "ablAbility.h"
#include "ablAbilityBlueprintGeneratedClass.h"
#include "ablAbilityBlueprint.h"
#include "AbilityEditor/ablAbilityEditorModes.h"
#include "AbilityEditor/ablAbilityGraphSchema.h"

#include "DetailLayoutBuilder.h"
#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

void SAblTaskPropertyBinding::Construct(const FArguments& InArgs, TSharedRef<FAblAbilityEditor> InEditor, FDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property)
{
	Editor = InEditor;
	UAblAbility* Ability = Editor.Pin()->GetAbility();
	const UAblAbilityTask* Task = Editor.Pin()->GetCurrentlySelectedAbilityTask();

	Blueprint = Editor.Pin()->GetAbilityBlueprint();
	BindableSignature = DelegateProperty->SignatureFunction;

	ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SAblTaskPropertyBinding::OnGenerateDelegateMenu, Task, Property)
					.ContentPadding(1)
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(this, &SAblTaskPropertyBinding::GetCurrentBindingImage, Task, Property)
							.ColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f))
						]
						//+ SHorizontalBox::Slot()
						//.AutoWidth()
						//.VAlign(VAlign_Center)
						//.Padding(4, 1, 0, 0)
						//[
						//	SNew(STextBlock)
						//	.Text(this, &SAblTaskPropertyBinding::GetCurrentBindingText, Task, Property)
						//	.Font(IDetailLayoutBuilder::GetDetailFont())
						//]
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.Visibility(this, &SAblTaskPropertyBinding::GetGotoBindingVisibility, Task, Property)
				.OnClicked(this, &SAblTaskPropertyBinding::HandleGotoBindingClicked, Task, Property)
				.VAlign(VAlign_Center)
				.ToolTipText(LOCTEXT("GotoFunction", "Goto Function"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("PropertyWindow.Button_Browse"))
				]
			]
		];
}

TSharedRef<SWidget> SAblTaskPropertyBinding::OnGenerateDelegateMenu(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle)
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);

	MenuBuilder.BeginSection("BindingActions");
	{
		if (CanRemoveBinding(Task, PropertyHandle))
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RemoveBinding", "Remove Binding"),
				LOCTEXT("RemoveBindingToolTip", "Removes the current binding"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Cross"),
				FUIAction(FExecuteAction::CreateSP(this, &SAblTaskPropertyBinding::HandleRemoveBinding, Task, PropertyHandle))
			);
		}

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreateBinding", "Create/View Binding"),
			LOCTEXT("CreateBindingToolTip", "Creates, or view, a function on the ability blueprint that will return the binding data for this property."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Plus"),
			FUIAction(FExecuteAction::CreateSP(this, &SAblTaskPropertyBinding::HandleCreateAndAddBinding, Task, PropertyHandle))
		);
	}
	MenuBuilder.EndSection(); //CreateBinding

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	return
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.MaxHeight(DisplayMetrics.PrimaryDisplayHeight * 0.5)
		[
			MenuBuilder.MakeWidget()
		];
}

const FSlateBrush* SAblTaskPropertyBinding::GetCurrentBindingImage(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const
{
	if (HasBinding(Task, PropertyHandle))
	{
		return FAppStyle::Get().GetBrush("Cross");
	}

	return FAppStyle::Get().GetBrush("Plus");
}

FText SAblTaskPropertyBinding::GetCurrentBindingText(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const
{
	if (HasBinding(Task, PropertyHandle))
	{
		return LOCTEXT("TaskPropertyModeDynamic", "-");
	}

	return LOCTEXT("TaskPropertyModeStatic", "+");
}

bool SAblTaskPropertyBinding::CanRemoveBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(Task, PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		return true;
	}

	return false;
}

void SAblTaskPropertyBinding::HandleRemoveBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(Task, PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		FBlueprintEditorUtils::RemoveGraph(Blueprint, existingFunc);
	}
}

void SAblTaskPropertyBinding::HandleCreateAndAddBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(Task, PropertyHandle);

	if (UEdGraph* existingFunc= GetGraphByName(*Blueprint, FuncName))
	{
		GotoFunction(existingFunc);
	}
	else
	{
		const FScopedTransaction Transaction(LOCTEXT("CreateDelegate", "Create Binding"));

		Blueprint->Modify();

		// Create the function graph.
		UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FuncName,
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		const bool bUserCreated = true;
		FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FunctionGraph, bUserCreated, BindableSignature);

		// Only mark bindings as pure that need to be.
		if (GeneratePureBindings)
		{
			const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());
			Schema_K2->AddExtraFunctionFlags(FunctionGraph, FUNC_BlueprintPure);
		}

		if (PropertyHandle->HasMetaData(TEXT("AblDefaultBinding")))
		{
			const FString& DefaultFunc = PropertyHandle->GetMetaData(TEXT("AblDefaultBinding"));

			// Only go through the binding if we have a method already overloaded.
			if (GetGraphByName(*Blueprint, FName(*DefaultFunc)) != nullptr)
			{
				UAblAbility* Ability = Editor.Pin()->GetAbility();
				if (UFunction* Func = Ability->GetClass()->FindFunctionByName(FName(*DefaultFunc)))
				{
					TArray<UK2Node_FunctionResult*> ReturnNodes;
					FunctionGraph->GetNodesOfClass<UK2Node_FunctionResult>(ReturnNodes);

					TArray<UK2Node_FunctionEntry*> FuncEntryNodes;
					FunctionGraph->GetNodesOfClass<UK2Node_FunctionEntry>(FuncEntryNodes);

					if (ReturnNodes.Num() && FuncEntryNodes.Num())
					{
						UK2Node_FunctionEntry* EntryNode = FuncEntryNodes[0];
						UK2Node_FunctionResult* ReturnNode = ReturnNodes[0];

						const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());

						FGraphNodeCreator<UK2Node_CallFunction> Creator(*FunctionGraph);
						UK2Node_CallFunction* FuncNode = Creator.CreateNode();
						FuncNode->FunctionReference.SetFromField<UFunction>(Func, Ability->GetClass());
						Creator.Finalize();

						MapOutputPinsToInputPins(*EntryNode, *FuncNode);
						MapOutputPinsToInputPins(*FuncNode, *ReturnNode);

						UEdGraphPin* OutFuncEntry = Schema_K2->FindExecutionPin(*EntryNode, EGPD_Output);
						UEdGraphPin* InFuncCall = Schema_K2->FindExecutionPin(*FuncNode, EGPD_Input);
						UEdGraphPin* OutFuncCall = Schema_K2->FindExecutionPin(*FuncNode, EGPD_Output);
						UEdGraphPin* InReturn = Schema_K2->FindExecutionPin(*ReturnNode, EGPD_Input);

						OutFuncEntry->BreakAllPinLinks();
						OutFuncEntry->MakeLinkTo(InFuncCall);
						InReturn->BreakAllPinLinks();
						OutFuncCall->MakeLinkTo(InReturn);
					}
				}
			}
		}

		GotoFunction(FunctionGraph);
	}

}

void SAblTaskPropertyBinding::GotoFunction(UEdGraph* FunctionGraph)
{
	TSharedPtr<FAblAbilityEditor> EditorPin = Editor.Pin();
	if (EditorPin.IsValid())
	{
		EditorPin->SetCurrentMode(FAblAbilityEditorModes::AbilityBlueprintMode);
		EditorPin->OpenDocument(FunctionGraph, FDocumentTracker::OpenNewDocument);
	}
}

EVisibility SAblTaskPropertyBinding::GetGotoBindingVisibility(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const
{
	return HasBinding(Task, PropertyHandle) ? EVisibility::Visible : EVisibility::Hidden;
}

FReply SAblTaskPropertyBinding::HandleGotoBindingClicked(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(Task, PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		GotoFunction(existingFunc);
	}

	return FReply::Handled();
}

FReply SAblTaskPropertyBinding::AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action)
{
	return FReply::Handled();
}

void SAblTaskPropertyBinding::MapOutputPinsToInputPins(UEdGraphNode& NodeA, UEdGraphNode& NodeB) const
{
	for (UEdGraphPin* OutputPin : NodeA.Pins)
	{
		if ((OutputPin->Direction == EGPD_Output) && OutputPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
		{
			if (UEdGraphPin* InputPin = NodeB.FindPin(OutputPin->PinName))
			{
				OutputPin->MakeLinkTo(InputPin);
			}
		}
	}
}

UEdGraph* SAblTaskPropertyBinding::GetGraphByName(const UBlueprint& blueprint, const FName& name) const
{
	for (UEdGraph* graph : blueprint.FunctionGraphs)
	{
		if (graph->GetFName() == name)
		{
			return graph;
		}
	}

	return nullptr;
}

FName SAblTaskPropertyBinding::GetBindingNameForProperty(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FString BPFunctionName = TEXT("OnGetDynamicProperty_") + PropertyHandle->GetPropertyDisplayName().ToString();
	if (Task && !Task->GetDynamicPropertyIdentifier().IsEmpty())
	{
		BPFunctionName += TEXT("_") + Task->GetDynamicPropertyIdentifier();
	}

	return FName(*BPFunctionName);
}

bool SAblTaskPropertyBinding::HasBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FName FuncName = GetBindingNameForProperty(Task, PropertyHandle);

	return GetGraphByName(*Blueprint, FuncName) != nullptr;
}

void SAblAbilityPropertyBinding::Construct(const FArguments& InArgs, TSharedRef<FAblAbilityEditor> InEditor, FDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property)
{
	Editor = InEditor;

	Blueprint = Editor.Pin()->GetAbilityBlueprint();
	BindableSignature = DelegateProperty->SignatureFunction;

	ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SAblAbilityPropertyBinding::OnGenerateDelegateMenu, Property)
					.ContentPadding(1)
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(this, &SAblAbilityPropertyBinding::GetCurrentBindingImage, Property)
							.ColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f))
						]
					//+ SHorizontalBox::Slot()
					//	.AutoWidth()
					//	.VAlign(VAlign_Center)
					//	.Padding(4, 1, 0, 0)
					//	[
					//		SNew(STextBlock)
					//		.Text(this, &SAblAbilityPropertyBinding::GetCurrentBindingText, Property)
					//		.Font(IDetailLayoutBuilder::GetDetailFont())
					//	]
					]
				]
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.Visibility(this, &SAblAbilityPropertyBinding::GetGotoBindingVisibility, Property)
				.OnClicked(this, &SAblAbilityPropertyBinding::HandleGotoBindingClicked, Property)
				.VAlign(VAlign_Center)
				.ToolTipText(LOCTEXT("GotoFunction", "Goto Function"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("PropertyWindow.Button_Browse"))
				]
			]
		];
}

TSharedRef<SWidget> SAblAbilityPropertyBinding::OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle)
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);

	MenuBuilder.BeginSection("BindingActions");
	{
		if (CanRemoveBinding(PropertyHandle))
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RemoveBinding", "Remove Binding"),
				LOCTEXT("RemoveBindingToolTip", "Removes the current binding"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Cross"),
				FUIAction(FExecuteAction::CreateSP(this, &SAblAbilityPropertyBinding::HandleRemoveBinding, PropertyHandle))
			);
		}

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreateBinding", "Create/View Binding"),
			LOCTEXT("CreateBindingToolTip", "Creates, or view, a function on the ability blueprint that will return the binding data for this property."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Plus"),
			FUIAction(FExecuteAction::CreateSP(this, &SAblAbilityPropertyBinding::HandleCreateAndAddBinding, PropertyHandle))
		);
	}
	MenuBuilder.EndSection(); //CreateBinding

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.MaxHeight(DisplayMetrics.PrimaryDisplayHeight * 0.5)
		[
			MenuBuilder.MakeWidget()
		];
}

const FSlateBrush* SAblAbilityPropertyBinding::GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	if (HasBinding(PropertyHandle))
	{
		return FAppStyle::Get().GetBrush("Cross");
	}

	return FAppStyle::Get().GetBrush("Plus");
}

FText SAblAbilityPropertyBinding::GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	if (HasBinding(PropertyHandle))
	{
		return LOCTEXT("AbilityPropertyModeDynamic", "-");
	}

	return LOCTEXT("AbilityPropertyModeStatic", "+");
}

bool SAblAbilityPropertyBinding::CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		return true;
	}

	return false;
}

void SAblAbilityPropertyBinding::HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		FBlueprintEditorUtils::RemoveGraph(Blueprint, existingFunc);
	}
}

void SAblAbilityPropertyBinding::HandleCreateAndAddBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		GotoFunction(existingFunc);
	}
	else
	{
		const FScopedTransaction Transaction(LOCTEXT("CreateDelegate", "Create Binding"));

		Blueprint->Modify();

		// Create the function graph.
		UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FuncName,
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		const bool bUserCreated = true;
		FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FunctionGraph, bUserCreated, BindableSignature);

		// Only mark bindings as pure that need to be.
		if (GeneratePureBindings)
		{
			const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());
			Schema_K2->AddExtraFunctionFlags(FunctionGraph, FUNC_BlueprintPure);
		}

		if (PropertyHandle->HasMetaData(TEXT("AblDefaultBinding")))
		{
			const FString& DefaultFunc = PropertyHandle->GetMetaData(TEXT("AblDefaultBinding"));

			// Only go through the binding if we have a method already overloaded.
			if (GetGraphByName(*Blueprint, FName(*DefaultFunc)) != nullptr)
			{
				UAblAbility* Ability = Editor.Pin()->GetAbility();
				if (UFunction* Func = Ability->GetClass()->FindFunctionByName(FName(*DefaultFunc)))
				{
					TArray<UK2Node_FunctionResult*> ReturnNodes;
					FunctionGraph->GetNodesOfClass<UK2Node_FunctionResult>(ReturnNodes);

					TArray<UK2Node_FunctionEntry*> FuncEntryNodes;
					FunctionGraph->GetNodesOfClass<UK2Node_FunctionEntry>(FuncEntryNodes);

					if (ReturnNodes.Num() && FuncEntryNodes.Num())
					{
						UK2Node_FunctionEntry* EntryNode = FuncEntryNodes[0];
						UK2Node_FunctionResult* ReturnNode = ReturnNodes[0];

						const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());

						FGraphNodeCreator<UK2Node_CallFunction> Creator(*FunctionGraph);
						UK2Node_CallFunction* FuncNode = Creator.CreateNode();
						FuncNode->FunctionReference.SetFromField<UFunction>(Func, Ability->GetClass());
						Creator.Finalize();

						MapOutputPinsToInputPins(*EntryNode, *FuncNode);
						MapOutputPinsToInputPins(*FuncNode, *ReturnNode);

						UEdGraphPin* OutFuncEntry = Schema_K2->FindExecutionPin(*EntryNode, EGPD_Output);
						UEdGraphPin* InFuncCall = Schema_K2->FindExecutionPin(*FuncNode, EGPD_Input);
						UEdGraphPin* OutFuncCall = Schema_K2->FindExecutionPin(*FuncNode, EGPD_Output);
						UEdGraphPin* InReturn = Schema_K2->FindExecutionPin(*ReturnNode, EGPD_Input);

						OutFuncEntry->BreakAllPinLinks();
						OutFuncEntry->MakeLinkTo(InFuncCall);
						InReturn->BreakAllPinLinks();
						OutFuncCall->MakeLinkTo(InReturn);
					}
				}
			}
		}

		GotoFunction(FunctionGraph);
	}
}

void SAblAbilityPropertyBinding::GotoFunction(UEdGraph* FunctionGraph)
{
	TSharedPtr<FAblAbilityEditor> EditorPin = Editor.Pin();
	if (EditorPin.IsValid())
	{
		EditorPin->SetCurrentMode(FAblAbilityEditorModes::AbilityBlueprintMode);
		EditorPin->OpenDocument(FunctionGraph, FDocumentTracker::OpenNewDocument);
	}
}

EVisibility SAblAbilityPropertyBinding::GetGotoBindingVisibility(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	return HasBinding(PropertyHandle) ? EVisibility::Visible : EVisibility::Hidden;
}

FReply SAblAbilityPropertyBinding::HandleGotoBindingClicked(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		GotoFunction(existingFunc);
	}

	return FReply::Handled();
}

FReply SAblAbilityPropertyBinding::AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action)
{
	return FReply::Handled();
}

void SAblAbilityPropertyBinding::MapOutputPinsToInputPins(UEdGraphNode& NodeA, UEdGraphNode& NodeB) const
{
	for (UEdGraphPin* OutputPin : NodeA.Pins)
	{
		if ((OutputPin->Direction == EGPD_Output) && OutputPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
		{
			if (UEdGraphPin* InputPin = NodeB.FindPin(OutputPin->PinName))
			{
				OutputPin->MakeLinkTo(InputPin);
			}
		}
	}
}

UEdGraph* SAblAbilityPropertyBinding::GetGraphByName(const UBlueprint& blueprint, const FName& name) const
{
	for (UEdGraph* graph : blueprint.FunctionGraphs)
	{
		if (graph->GetFName() == name)
		{
			return graph;
		}
	}

	return nullptr;
}

FName SAblAbilityPropertyBinding::GetBindingNameForProperty(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	return FName(*(TEXT("OnGetDynamicProperty_") + PropertyHandle->GetPropertyDisplayName().ToString()));
}

bool SAblAbilityPropertyBinding::HasBinding(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	return GetGraphByName(*Blueprint, FuncName) != nullptr;
}

// Misc Binding (non-Task / Ability )

void SAblMiscPropertyBinding::Construct(const FArguments& InArgs, TSharedRef<FAblAbilityEditor> InEditor, FDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property)
{
	Editor = InEditor;
	UAblAbility* Ability = Editor.Pin()->GetAbility();
	check(Ability);

	PropertyCategory = InArgs._PropertyCategory;
	EventIdentifier = InArgs._EventIdentifier;

	Blueprint = Editor.Pin()->GetAbilityBlueprint();
	BindableSignature = DelegateProperty->SignatureFunction;

	ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SAblMiscPropertyBinding::OnGenerateDelegateMenu, Property)
					.ContentPadding(1)
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(this, &SAblMiscPropertyBinding::GetCurrentBindingImage, Property)
						.ColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f))
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.Visibility(this, &SAblMiscPropertyBinding::GetGotoBindingVisibility, Property)
					.OnClicked(this, &SAblMiscPropertyBinding::HandleGotoBindingClicked, Property)
					.VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("GotoFunction", "Goto Function"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("PropertyWindow.Button_Browse"))
					]
				]
		];
}

TSharedRef<SWidget> SAblMiscPropertyBinding::OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle)
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);

	MenuBuilder.BeginSection("BindingActions");
	{
		if (CanRemoveBinding(PropertyHandle))
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RemoveBinding", "Remove Binding"),
				LOCTEXT("RemoveBindingToolTip", "Removes the current binding"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Cross"),
				FUIAction(FExecuteAction::CreateSP(this, &SAblMiscPropertyBinding::HandleRemoveBinding, PropertyHandle))
			);
		}

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreateBinding", "Create/View Binding"),
			LOCTEXT("CreateBindingToolTip", "Creates, or view, a function on the ability blueprint that will return the binding data for this property."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Plus"),
			FUIAction(FExecuteAction::CreateSP(this, &SAblMiscPropertyBinding::HandleCreateAndAddBinding, PropertyHandle))
		);
	}
	MenuBuilder.EndSection(); //CreateBinding

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	return
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.MaxHeight(DisplayMetrics.PrimaryDisplayHeight * 0.5)
		[
			MenuBuilder.MakeWidget()
		];
}

const FSlateBrush* SAblMiscPropertyBinding::GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	if (HasBinding(PropertyHandle))
	{
		return FAppStyle::Get().GetBrush("Cross");
	}

	return FAppStyle::Get().GetBrush("Plus");
}

FText SAblMiscPropertyBinding::GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	if (HasBinding(PropertyHandle))
	{
		return LOCTEXT("MiscPropertyModeDynamic", "-");
	}

	return LOCTEXT("MiscPropertyModeStatic", "+");
}

bool SAblMiscPropertyBinding::CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		return true;
	}

	return false;
}

void SAblMiscPropertyBinding::HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		FBlueprintEditorUtils::RemoveGraph(Blueprint, existingFunc);
	}
}

void SAblMiscPropertyBinding::HandleCreateAndAddBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		GotoFunction(existingFunc);
	}
	else
	{
		const FScopedTransaction Transaction(LOCTEXT("CreateDelegate", "Create Binding"));

		Blueprint->Modify();

		// Create the function graph.
		UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FuncName,
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		const bool bUserCreated = true;
		FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FunctionGraph, bUserCreated, BindableSignature);

		// Only mark bindings as pure that need to be.
		if (GeneratePureBindings)
		{
			const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());
			Schema_K2->AddExtraFunctionFlags(FunctionGraph, FUNC_BlueprintPure);
		}

		if (PropertyHandle->HasMetaData(TEXT("AblDefaultBinding")))
		{
			const FString& DefaultFunc = PropertyHandle->GetMetaData(TEXT("AblDefaultBinding"));

			// Only go through the binding if we have a method already overloaded.
			if (GetGraphByName(*Blueprint, FName(*DefaultFunc)) != nullptr)
			{
				UAblAbility* Ability = Editor.Pin()->GetAbility();
				if (UFunction* Func = Ability->GetClass()->FindFunctionByName(FName(*DefaultFunc)))
				{
					TArray<UK2Node_FunctionResult*> ReturnNodes;
					FunctionGraph->GetNodesOfClass<UK2Node_FunctionResult>(ReturnNodes);

					TArray<UK2Node_FunctionEntry*> FuncEntryNodes;
					FunctionGraph->GetNodesOfClass<UK2Node_FunctionEntry>(FuncEntryNodes);

					if (ReturnNodes.Num() && FuncEntryNodes.Num())
					{
						UK2Node_FunctionEntry* EntryNode = FuncEntryNodes[0];
						UK2Node_FunctionResult* ReturnNode = ReturnNodes[0];

						const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());

						FGraphNodeCreator<UK2Node_CallFunction> Creator(*FunctionGraph);
						UK2Node_CallFunction* FuncNode = Creator.CreateNode();
						FuncNode->FunctionReference.SetFromField<UFunction>(Func, Ability->GetClass());
						Creator.Finalize();

						MapOutputPinsToInputPins(*EntryNode, *FuncNode);
						MapOutputPinsToInputPins(*FuncNode, *ReturnNode);

						UEdGraphPin* OutFuncEntry = Schema_K2->FindExecutionPin(*EntryNode, EGPD_Output);
						UEdGraphPin* InFuncCall = Schema_K2->FindExecutionPin(*FuncNode, EGPD_Input);
						UEdGraphPin* OutFuncCall = Schema_K2->FindExecutionPin(*FuncNode, EGPD_Output);
						UEdGraphPin* InReturn = Schema_K2->FindExecutionPin(*ReturnNode, EGPD_Input);

						OutFuncEntry->BreakAllPinLinks();
						OutFuncEntry->MakeLinkTo(InFuncCall);
						InReturn->BreakAllPinLinks();
						OutFuncCall->MakeLinkTo(InReturn);
					}
				}
			}
		}

		GotoFunction(FunctionGraph);
	}

}

void SAblMiscPropertyBinding::GotoFunction(UEdGraph* FunctionGraph)
{
	TSharedPtr<FAblAbilityEditor> EditorPin = Editor.Pin();
	if (EditorPin.IsValid())
	{
		EditorPin->SetCurrentMode(FAblAbilityEditorModes::AbilityBlueprintMode);
		EditorPin->OpenDocument(FunctionGraph, FDocumentTracker::OpenNewDocument);
	}
}

EVisibility SAblMiscPropertyBinding::GetGotoBindingVisibility(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	return HasBinding(PropertyHandle) ? EVisibility::Visible : EVisibility::Hidden;
}

FReply SAblMiscPropertyBinding::HandleGotoBindingClicked(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	if (UEdGraph* existingFunc = GetGraphByName(*Blueprint, FuncName))
	{
		GotoFunction(existingFunc);
	}

	return FReply::Handled();
}

FReply SAblMiscPropertyBinding::AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action)
{
	return FReply::Handled();
}

void SAblMiscPropertyBinding::MapOutputPinsToInputPins(UEdGraphNode& NodeA, UEdGraphNode& NodeB) const
{
	for (UEdGraphPin* OutputPin : NodeA.Pins)
	{
		if ((OutputPin->Direction == EGPD_Output) && OutputPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
		{
			if (UEdGraphPin* InputPin = NodeB.FindPin(OutputPin->PinName))
			{
				OutputPin->MakeLinkTo(InputPin);
			}
		}
	}
}

UEdGraph* SAblMiscPropertyBinding::GetGraphByName(const UBlueprint& blueprint, const FName& name) const
{
	for (UEdGraph* graph : blueprint.FunctionGraphs)
	{
		if (graph->GetFName() == name)
		{
			return graph;
		}
	}

	return nullptr;
}

FName SAblMiscPropertyBinding::GetBindingNameForProperty(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FString BPFunctionName;

	if (EventIdentifier.IsEmpty())
	{
		BPFunctionName = FString::Format(TEXT("OnGetDynamicProperty_{0}_{1}"), { PropertyCategory, PropertyHandle->GetPropertyDisplayName().ToString() });
	}
	else
	{
		BPFunctionName = FString::Format(TEXT("OnGetDynamicProperty_{0}_{1}_{2}"), { PropertyCategory, PropertyHandle->GetPropertyDisplayName().ToString(), EventIdentifier });
	}

	return FName(*BPFunctionName);
}

bool SAblMiscPropertyBinding::HasBinding(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FName FuncName = GetBindingNameForProperty(PropertyHandle);

	return GetGraphByName(*Blueprint, FuncName) != nullptr;
}

#undef LOCTEXT_NAMESPACE