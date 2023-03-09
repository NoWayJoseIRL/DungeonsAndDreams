// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once
#include "ablAbility.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablBranchTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

class UAblBranchCondition;
class UInputSettings;

UCLASS(Transient)
class UAblBranchTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblBranchTaskScratchPad();
	virtual ~UAblBranchTaskScratchPad();

	/* Cached for the Custom Branch Conditional*/
	UPROPERTY(transient)
	const UAblAbility* BranchAbility;

	/* Keys to check for the Input Conditional */
	UPROPERTY(transient)
	TArray<struct FKey> CachedKeys;

    UPROPERTY(transient)
    bool BranchConditionsMet;
};

UCLASS(EditInlineNew, hidecategories = ("Targets"))
class ABLECORE_API UAblBranchTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblBranchTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblBranchTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* On Task Tick*/
	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;

    /* On Task End*/
    virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;

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

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAblAbility* Ability) override;

	/* Return this Branch Tasks' conditions. */
	const TArray<UAblBranchCondition*>& GetBranchConditions() const { return m_Conditions; }

#if WITH_EDITOR
	/* Returns the category for this Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblBranchCategory", "Logic"); }
	
	/* Returns the name of the Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblBranchTask", "Branch"); }
	
	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of the Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblBranchTaskDesc", "Allows this ability to immediately branch into another ability if the branch condition passes."); }

	/* Returns the Rich Text description, with mark ups. */
	virtual FText GetRichTextTaskSummary() const override;

	/* Returns what color to use for this Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(79.0f / 255.0f, 159.0f / 255.0f, 206.0f / 255.0f); } 
	
	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const override { return UAblAbilityTask::GetEstimatedTaskCost() + ABLTASK_EST_BP_EVENT; } // Assume worst case and they are using a BP Custom condition.

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Fix up our flags. */
	virtual bool FixUpObjectFlags() override;
#endif
protected:
	/* Helper method to check our conditions. */
	bool CheckBranchCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	/* The Ability to Branch to. */
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Ability", AblBindableProperty, AblDefaultBinding = "OnGetBranchAbilityBP"))
	TSubclassOf<UAblAbility> m_BranchAbility;

	UPROPERTY()
	FGetAblAbility m_BranchAbilityDelegate;

	/* The Conditions for the Ability to Branch. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Branch", meta = (DisplayName = "Conditions"))
	TArray<UAblBranchCondition*> m_Conditions;

	/* The Conditions for the Ability to Branch. */
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Must Pass All Conditions", AblBindableProperty))
	bool m_MustPassAllConditions;

	UPROPERTY()
	FGetAblBool m_MustPassAllConditionsDelegate;

	// If true, you're existing targets will be carried over to the branched Ability.
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Copy Targets on Branch", AblBindableProperty))
	bool m_CopyTargetsOnBranch;

	UPROPERTY()
	FGetAblBool m_CopyTargetsOnBranchDelegate;

	// If true, the branch conditions will be checked during the task window, but the actual branch itself will activate at the end of the task window.
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Branch on Task End", AblBindableProperty))
	bool m_BranchOnTaskEnd;

	UPROPERTY()
	FGetAblBoolWithResult m_BranchOnTaskEndDelegate;

private:
	/* Helper method to consolidate logic. */
	void InternalDoBranch(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;
};

#undef LOCTEXT_NAMESPACE