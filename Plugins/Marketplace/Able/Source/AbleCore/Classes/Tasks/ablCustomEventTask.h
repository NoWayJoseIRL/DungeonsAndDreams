// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablCustomEventTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UCLASS(EditInlineNew, hidecategories = ("Targets", "Optimization"))
class UAblCustomEventTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblCustomEventTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCustomEventTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const override { return false; }
	
	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return true; }

	/* Returns the Realm our Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of this Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblCustomEventCategory", "Blueprint|Event"); }
	
	/* Returns the name of this Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblCustomEventTask", "Custom Event"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of this Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblCustomEventTaskDesc", "Calls the OnCustomEvent Blueprint Event on the owning Ability."); }
	
	/* Returns the color of this Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(148.0f / 255.0f, 166.0f / 255.0f, 169.0f / 255.0f); }
	
	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const override { return UAblAbilityTask::GetEstimatedTaskCost() + ABLTASK_EST_BP_EVENT; }

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }
	
	/* Returns true if the user is allowed to edit the Tasks realm. */
	virtual bool CanEditTaskRealm() const override { return true; }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;
#endif

protected:
	/* A String identifier you can use to identify this specific task in the ability blueprint. */
	UPROPERTY(EditAnywhere, Category = "Event", meta = (DisplayName = "Event Name"))
	FName m_EventName;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE