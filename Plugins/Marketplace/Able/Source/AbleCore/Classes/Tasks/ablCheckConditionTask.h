// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablCheckConditionTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

class UAblBranchCondition;
class UInputSettings;

UCLASS(Transient)
class UAblCheckConditionTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblCheckConditionTaskScratchPad();
	virtual ~UAblCheckConditionTaskScratchPad();

    /* Cached for the Custom Branch Conditional*/
    UPROPERTY(transient)
    bool ConditionMet;
};

UCLASS(EditInlineNew, hidecategories = ("Targets"))
class UAblCheckConditionTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblCheckConditionTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCheckConditionTask();

    /* Called to determine if a Task can end. Default behavior is to see if our context time is > than our end time. */
    virtual bool IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* On Task Tick*/
	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;

	/* Returns true if our Task is Async supported. */
	virtual bool IsAsyncFriendly() const override { return false; }
	
	/* Returns true if this task needs its Tick method called. */
	virtual bool NeedsTick() const override { return true; }

	/* Returns the Realm to execute this task in. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_ClientAndServer; } // Client for Auth client, Server for AIs/Proxies.

	/* Create the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Return the Profiler Stat Id for this Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category for this Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblCheckConditionCategory", "Logic"); }
	
	/* Returns the name of the Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblCheckConditionTask", "Check Condition"); }
	
	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of the Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblCheckConditionTaskDesc", "Polls the ability blueprint for a condition."); }
	
	/* Returns what color to use for this Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0f, 0.0f, 1.0f); } //Purple
	
	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const override { return UAblAbilityTask::GetEstimatedTaskCost() + ABLTASK_EST_BP_EVENT; } // Assume worst case and they are using a BP Custom condition.
#endif
protected:
	/* Helper method to check our conditions. */
	bool CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, const UAblCheckConditionTaskScratchPad& ScratchPad) const;

    // If using Dynamic Branch Ability, this name will be passed along when calling the function (optional).
    UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Custom Event Name"))
    FName m_ConditionEventName;
};

#undef LOCTEXT_NAMESPACE