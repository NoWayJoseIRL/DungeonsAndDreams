
#include "AbilityEditor/SAbilityDetailView.h"

#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"

#include "IDetailsView.h"
#include "IDetailKeyframeHandler.h"
#include "EditorClassUtils.h"


#include "PropertyEditorModule.h"
#include "PropertyEditor/Public/PropertyHandle.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Blueprint/WidgetTree.h"
#include "ScopedTransaction.h"
#include "Styling/SlateIconFinder.h"

#include "AbilityEditor/SAbilityPropertyBinding.h"
#include "AbilityEditor/AblAbilityTaskDetails.h"
#include "ablAbility.h"
#include "Channeling/ablChannelingBase.h"
#include "Targeting/ablTargetingBase.h"
#include "Tasks/IAblAbilityTask.h"
#include "Tasks/ablBranchCondition.h"
#include "Tasks/ablCollisionFilters.h"
#include "Tasks/ablCollisionQueryTypes.h"
#include "Tasks/ablCollisionSweepTypes.h"
#include "Tasks/ablPlayParticleEffectParams.h"
#include "Tasks/ablSetShaderParameterValue.h"

void SAblAbilityDetailsView::Construct(const FArguments& InArgs, TSharedPtr<FAblAbilityEditor> InBlueprintEditor)
{
	BlueprintEditor = InBlueprintEditor;

	// Create a property view
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FNotifyHook* NotifyHook = this;

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NotifyHook = NotifyHook;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;

	PropertyView = EditModule.CreateDetailView(DetailsViewArgs);

	// Create a handler for property binding via the details panel
	TSharedRef<FAblDetailWidgetExtensionHandler> BindingHandler = MakeShareable(new FAblDetailWidgetExtensionHandler(InBlueprintEditor));
	PropertyView->SetExtensionHandler(BindingHandler);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0)
		[
			PopulateSlot(PropertyView.ToSharedRef())
		]
	];

	RegisterCustomizations();
}

SAblAbilityDetailsView::~SAblAbilityDetailsView()
{

}

void SAblAbilityDetailsView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	UObject* ObjectToObserve = GetObjectToObserve();
	if (LastObservedObject != ObjectToObserve)
	{
		TArray<UObject*> SelectedObjs;
		if (ObjectToObserve)
		{
			SelectedObjs.Add(ObjectToObserve);
		}
		PropertyView->SetObjects(SelectedObjs);
	}
}

void SAblAbilityDetailsView::NotifyPreChange(FEditPropertyChain* PropertyAboutToChange)
{

}

void SAblAbilityDetailsView::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged)
{

}

void SAblAbilityDetailsView::RegisterCustomizations()
{
	PropertyView->RegisterInstancedCustomPropertyLayout(UAblAbilityTask::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FAblAbilityTaskDetails::MakeInstance, BlueprintEditor.Pin().ToSharedRef(), BlueprintEditor.Pin()->GetBlueprintObj()));
}

void SAblAbilityDetailsView::OnEditorSelectionChanging()
{
	ClearFocusIfOwned();

	// We force the destruction of the currently monitored object when selection is about to change, to ensure all migrations occur
	// immediately.
	TArray<UObject*> SelectedObjs;
	PropertyView->SetObjects(SelectedObjs);
}

void SAblAbilityDetailsView::OnEditorSelectionChanged()
{

}

void SAblAbilityDetailsView::OnPropertyViewObjectArrayChanged(const FString& InTitle, const TArray<TWeakObjectPtr<UObject>>& InObjects)
{

}

void SAblAbilityDetailsView::ClearFocusIfOwned()
{
	static bool bIsReentrant = false;
	if (!bIsReentrant)
	{
		bIsReentrant = true;
		// When the selection is changed, we may be potentially actively editing a property,
		// if this occurs we need need to immediately clear keyboard focus
		if (FSlateApplication::Get().HasFocusedDescendants(AsShared()))
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Mouse);
		}
		bIsReentrant = false;
	}
}

FAblDetailWidgetExtensionHandler::FAblDetailWidgetExtensionHandler(TSharedPtr<class FAblAbilityEditor> InBlueprintEditor)
{
	BlueprintEditor = InBlueprintEditor;
}

bool FAblDetailWidgetExtensionHandler::IsPropertyExtendable(const UClass* InObjectClass, const IPropertyHandle& PropertyHandle) const
{
	static const FName AblBindablePropertyName("AblBindableProperty");
	return PropertyHandle.IsValidHandle() && PropertyHandle.HasMetaData(AblBindablePropertyName);
}

TSharedRef<SWidget> FAblDetailWidgetExtensionHandler::GenerateExtensionWidget(const IDetailLayoutBuilder& InDetailBuilder, const UClass* InObjectClass, TSharedPtr<IPropertyHandle> PropertyHandle)
{
	FProperty* Property = PropertyHandle->GetProperty();
	FString DelegateName = Property->GetName() + "Delegate";

	FDelegateProperty* DelegateProperty = FindFieldChecked<FDelegateProperty>(Property->GetOwnerClass(), FName(*DelegateName));

	const bool bIsEditable = Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
	const bool bDoSignaturesMatch = DelegateProperty->SignatureFunction->GetReturnProperty()->SameType(Property);

	if (!(bIsEditable && bDoSignaturesMatch))
	{
		return SNullWidget::NullWidget;
	}

	// Not my favorite way to do this but not sure if we can swap this for a Case/Switch somehow...
	if (InObjectClass->IsChildOf<UAblAbilityTask>())
	{
		return SNew(SAblTaskPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false);
	}
	else if (InObjectClass->IsChildOf<UAblAbility>())
	{
		return SNew(SAblAbilityPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false);
	}
	else if (InObjectClass->IsChildOf<UAblChannelingBase>())
	{
		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Channeling"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}
	else if (InObjectClass->IsChildOf<UAblTargetingBase>())
	{

		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Targeting"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}
	else if (InObjectClass->IsChildOf<UAblCollisionShape>())
	{
		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Query"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}
	else if (InObjectClass->IsChildOf<UAblCollisionSweepShape>())
	{
		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Sweep"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}
	else if (InObjectClass->IsChildOf<UAblParticleEffectParam>())
	{
		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Particle"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}
	else if (InObjectClass->IsChildOf<UAblSetParameterValue>())
	{
		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Shader"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}
	else if (InObjectClass->IsChildOf<UAblCollisionFilter>())
	{
		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Filter"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}
	else if (InObjectClass->IsChildOf<UAblBranchCondition>())
	{
		return SNew(SAblMiscPropertyBinding, BlueprintEditor.Pin().ToSharedRef(), DelegateProperty, PropertyHandle.ToSharedRef())
			.GeneratePureBindings(false)
			.PropertyCategory(TEXT("Branch"))
			.EventIdentifier(BlueprintEditor.Pin()->GetDynamicBindingIdentifierForClass(InObjectClass));
	}

	return SNullWidget::NullWidget;
}