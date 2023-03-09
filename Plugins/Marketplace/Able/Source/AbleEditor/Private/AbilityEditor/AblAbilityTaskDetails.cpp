// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AblAbilityTaskDetails.h"

#include "AbleEditorPrivate.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "PropertyEditorModule.h"

#include "Tasks/IAblAbilityTask.h"

#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ablAbilityEditor"

void FAblAbilityTaskDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	if (UAblAbilityTask* TaskBeingViewed = GetCurrentlyViewedTask(DetailBuilder))
	{
		TSharedRef<IPropertyHandle> StartTimeHandle = DetailBuilder.GetProperty(TEXT("m_StartTime"));
		TSharedRef<IPropertyHandle> EndTimeHandle = DetailBuilder.GetProperty(TEXT("m_EndTime"));

		if (TaskBeingViewed->ShowEndTime() == EVisibility::Collapsed)
		{
			// Hide both Single frame and the end time handle since both should be programmatically set if we aren't allowing the user to change the end time.
			DetailBuilder.HideProperty(EndTimeHandle);
		}
		else if (TaskBeingViewed->ShowEndTime() == EVisibility::Hidden) // Hidden is read only effectively (at least for Tasks).
		{
			IDetailCategoryBuilder& TimingCategory = DetailBuilder.EditCategory("Timing");

			// To add the custom End property in the right position (e.g. the end of the category). We hide both properties.
			// We then create a "Custom" row for our start time which is just the default widget, and a true custom row for our end time.
			DetailBuilder.HideProperty(StartTimeHandle);
			DetailBuilder.HideProperty(EndTimeHandle);

			FDetailWidgetRow& StartTimeWidgetRow = TimingCategory.AddCustomRow(LOCTEXT("StartTime", "Start Time"));
			StartTimeWidgetRow.NameContent()
			[
				StartTimeHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				StartTimeHandle->CreatePropertyValueWidget()
			];

			FDetailWidgetRow& EndTimeWidgetRow = TimingCategory.AddCustomRow(LOCTEXT("EndTime", "End Time"));
			GenerateReadOnlyEndTimeRow(EndTimeWidgetRow, DetailBuilder);
		}

		// Handle our Network Realm
		IDetailCategoryBuilder& NetworkCategory = DetailBuilder.EditCategory("Realm", LOCTEXT("RealmCategory", "Realm"));

		if (!TaskBeingViewed->CanEditTaskRealm())
		{
			// The property doesn't exist, but we still want to show the user what realm this task exists in.
			FDetailWidgetRow& TaskRealmWidgetRow = NetworkCategory.AddCustomRow(LOCTEXT("TaskRealm", "Realm"));
			GenerateReadOnlyTaskRealmRow(TaskRealmWidgetRow, DetailBuilder);
		}
	}

}

UAblAbilityTask* FAblAbilityTaskDetails::GetCurrentlyViewedTask(IDetailLayoutBuilder& DetailBuilder) const
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingViewed;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingViewed);

	UAblAbilityTask* TaskBeingViewed = nullptr;
	for (TWeakObjectPtr<UObject> Object : ObjectsBeingViewed)
	{
		TaskBeingViewed = Cast<UAblAbilityTask>(Object.Get());
		if (TaskBeingViewed)
		{
			return TaskBeingViewed;
		}
	}

	return nullptr;
}

void FAblAbilityTaskDetails::GenerateReadOnlyEndTimeRow(FDetailWidgetRow& OutRow, IDetailLayoutBuilder& DetailBuilder) const
{	
	OutRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EndTime", "End Time"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SEditableTextBox)
				.Text(this, &FAblAbilityTaskDetails::GetEndTimeText, &DetailBuilder)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.IsReadOnly(true)
			]
		];
}

FText FAblAbilityTaskDetails::GetEndTimeText(IDetailLayoutBuilder* DetailBuilder) const
{
	if (UAblAbilityTask* Task = GetCurrentlyViewedTask(*DetailBuilder))
	{
		FNumberFormattingOptions Fmt;
		Fmt.SetMinimumIntegralDigits(1);
		Fmt.SetMinimumFractionalDigits(1);

		return FText::AsNumber(Task->GetEndTime(), &Fmt);
	}

	return FText();
}

void FAblAbilityTaskDetails::GenerateReadOnlyTaskRealmRow(class FDetailWidgetRow& OutRow, IDetailLayoutBuilder& DetailBuilder) const
{
	OutRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TaskRealm", "Realm"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SEditableTextBox)
				.Text(this, &FAblAbilityTaskDetails::GetTaskRealmText, &DetailBuilder)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.IsReadOnly(true)
			]
		];
}

FText FAblAbilityTaskDetails::GetTaskRealmText(IDetailLayoutBuilder* DetailBuilder) const
{
	if (UAblAbilityTask* Task = GetCurrentlyViewedTask(*DetailBuilder))
	{
		switch (Task->GetTaskRealm())
		{
		case EAblAbilityTaskRealm::ATR_Client: return LOCTEXT("ClientRealm", "Client");
		case EAblAbilityTaskRealm::ATR_ClientAndServer: return LOCTEXT("ClientAndServerRealm", "Client And Server");
		case EAblAbilityTaskRealm::ATR_Server: return LOCTEXT("ServerRealm", "Server");
		default: checkNoEntry(); break;
		}
	}

	return FText();
}

#undef LOCTEXT_NAMESPACE