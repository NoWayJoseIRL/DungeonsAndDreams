// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbility.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablPlayAbilityTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UCLASS(EditInlineNew)
class UAblPlayAbilityTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblPlayAbilityTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblPlayAbilityTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return true; }

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const override { return false; }
	
	/* Returns the realm our Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAblAbility* Ability ) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblPlayAbilityCategory", "Gameplay"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblPlayAbilityTask", "Play Ability"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblPlayAbilityTaskDesc", "Creates an Ability Context and calls Activate Ability on the Target's Ability Component. This will cause an interrupt if valid."); }
	
	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(166.0f / 255.0f, 77.0f / 255.0f, 121.0f / 255.0f); } 

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	/* The Ability to Play. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Ability", AblBindableProperty))
	TSubclassOf<UAblAbility> m_Ability;

	UPROPERTY()
	FGetAblAbility m_AbilityDelegate;

	/* Who to set as the "Source" of this damage. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Owner", AblBindableProperty))
	TEnumAsByte<EAblAbilityTargetType> m_Owner;

	UPROPERTY()
	FGetAblTargetType m_OwnerDelegate;

	/* Who to set as the "Source" of this damage. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Instigator", AblBindableProperty))
	TEnumAsByte<EAblAbilityTargetType> m_Instigator;

	UPROPERTY()
	FGetAblTargetType m_InstigatorDelegate;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;

	// If true, you're existing targets will be carried over to the new Ability.
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Copy Targets", AblBindableProperty))
	bool m_CopyTargets;

	UPROPERTY()
	FGetAblBool m_CopyTargetsDelegate;

};

#undef LOCTEXT_NAMESPACE
