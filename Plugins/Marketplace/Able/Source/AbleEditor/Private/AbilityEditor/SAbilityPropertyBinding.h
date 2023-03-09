// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "EdGraph/EdGraphSchema.h"
#include "UObject/UnrealTypePrivate.h"
#include "PropertyHandle.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "ablAbilityBlueprint.h"

class FMenuBuilder;
class UEdGraph;
class UWidgetBlueprint;
struct FEditorPropertyPath;

class SAblTaskPropertyBinding : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAblTaskPropertyBinding)
		: _GeneratePureBindings(true)
	{}
	SLATE_ARGUMENT(bool, GeneratePureBindings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FAblAbilityEditor> InEditor, FDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property);

protected:
	struct FFunctionInfo
	{
		FFunctionInfo()
			: Function(nullptr)
		{
		}

		FText DisplayName;
		FString Tooltip;

		FName FuncName;
		UFunction* Function;
	};

	TSharedRef<SWidget> OnGenerateDelegateMenu(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle);
	const FSlateBrush* GetCurrentBindingImage(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const;
	FText GetCurrentBindingText(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const;

	bool CanRemoveBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleRemoveBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleCreateAndAddBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle);
	void GotoFunction(UEdGraph* FunctionGraph);

	EVisibility GetGotoBindingVisibility(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const;

	FReply HandleGotoBindingClicked(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle);

	FReply AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action);

private:
	void MapOutputPinsToInputPins(UEdGraphNode& NodeA, UEdGraphNode& NodeB) const;
	UEdGraph* GetGraphByName(const UBlueprint& blueprint, const FName& name) const;
	FName GetBindingNameForProperty(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const;
	bool HasBinding(const UAblAbilityTask* Task, TSharedRef<IPropertyHandle> PropertyHandle) const;
	TWeakPtr<FAblAbilityEditor> Editor;

	UAblAbilityBlueprint* Blueprint;

	bool GeneratePureBindings;
	UFunction* BindableSignature;
};

class SAblAbilityPropertyBinding : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAblAbilityPropertyBinding)
		: _GeneratePureBindings(true)
	{}
	SLATE_ARGUMENT(bool, GeneratePureBindings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FAblAbilityEditor> InEditor, FDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property);

protected:
	struct FFunctionInfo
	{
		FFunctionInfo()
			: Function(nullptr)
		{
		}

		FText DisplayName;
		FString Tooltip;

		FName FuncName;
		UFunction* Function;
	};

	TSharedRef<SWidget> OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle);
	const FSlateBrush* GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const;
	FText GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const;

	bool CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleCreateAndAddBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void GotoFunction(UEdGraph* FunctionGraph);

	EVisibility GetGotoBindingVisibility(TSharedRef<IPropertyHandle> PropertyHandle) const;

	FReply HandleGotoBindingClicked(TSharedRef<IPropertyHandle> PropertyHandle);

	FReply AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action);

private:
	void MapOutputPinsToInputPins(UEdGraphNode& NodeA, UEdGraphNode& NodeB) const;
	UEdGraph* GetGraphByName(const UBlueprint& blueprint, const FName& name) const;
	FName GetBindingNameForProperty(TSharedRef<IPropertyHandle> PropertyHandle) const;
	bool HasBinding(TSharedRef<IPropertyHandle> PropertyHandle) const;

	TWeakPtr<FAblAbilityEditor> Editor;

	UAblAbilityBlueprint* Blueprint;

	bool GeneratePureBindings;
	UFunction* BindableSignature;
};

class SAblMiscPropertyBinding : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAblMiscPropertyBinding)
		: _GeneratePureBindings(true)
	{}
	SLATE_ARGUMENT(bool, GeneratePureBindings)
	SLATE_ARGUMENT(FString, PropertyCategory)
	SLATE_ARGUMENT(FString, EventIdentifier)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FAblAbilityEditor> InEditor, FDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property);

protected:
	struct FFunctionInfo
	{
		FFunctionInfo()
			: Function(nullptr)
		{
		}

		FText DisplayName;
		FString Tooltip;

		FName FuncName;
		UFunction* Function;
	};

	TSharedRef<SWidget> OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle);
	const FSlateBrush* GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const;
	FText GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const;

	bool CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleCreateAndAddBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void GotoFunction(UEdGraph* FunctionGraph);

	EVisibility GetGotoBindingVisibility(TSharedRef<IPropertyHandle> PropertyHandle) const;

	FReply HandleGotoBindingClicked(TSharedRef<IPropertyHandle> PropertyHandle);

	FReply AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action);

private:
	void MapOutputPinsToInputPins(UEdGraphNode& NodeA, UEdGraphNode& NodeB) const;
	UEdGraph* GetGraphByName(const UBlueprint& blueprint, const FName& name) const;
	FName GetBindingNameForProperty(TSharedRef<IPropertyHandle> PropertyHandle) const;
	bool HasBinding(TSharedRef<IPropertyHandle> PropertyHandle) const;
	TWeakPtr<FAblAbilityEditor> Editor;

	UAblAbilityBlueprint* Blueprint;

	bool GeneratePureBindings;
	UFunction* BindableSignature;
	FString PropertyCategory;
	FString EventIdentifier;
};