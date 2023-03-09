// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "IDetailCustomization.h"
#include "PropertyEditorModule.h"

#include "ablAbilityEditor.h"
#include "ablAbilityBlueprint.h"

class IDetailCategoryBuilder;

/* Property Customizer for UAblAbilityTask. */
class FAblAbilityTaskDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance(TSharedRef<FAblAbilityEditor> InEditor, UBlueprint* InBlueprint)
	{
		return MakeShareable(new FAblAbilityTaskDetails(InEditor, InBlueprint));
	}

	FAblAbilityTaskDetails(TSharedRef<FAblAbilityEditor> InEditor, UBlueprint* InBlueprint)
		: Editor(InEditor)
		, Blueprint(CastChecked<UAblAbilityBlueprint>(InBlueprint))
	{
	}

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface
private:
	class UAblAbilityTask* GetCurrentlyViewedTask(IDetailLayoutBuilder& DetailBuilder) const;

	TWeakPtr<FAblAbilityEditor> Editor;

	UAblAbilityBlueprint* Blueprint;

protected:
	// End Time
	virtual void GenerateReadOnlyEndTimeRow(class FDetailWidgetRow& OutRow, IDetailLayoutBuilder& DetailBuilder) const;
	virtual FText GetEndTimeText(IDetailLayoutBuilder* DetailBuilder) const;

	// Task Realm
	virtual void GenerateReadOnlyTaskRealmRow(class FDetailWidgetRow& OutRow, IDetailLayoutBuilder& DetailBuilder) const;
	virtual FText GetTaskRealmText(IDetailLayoutBuilder* DetailBuilder) const;
};