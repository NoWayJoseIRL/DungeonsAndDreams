// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablSetGameplayTagTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblSetGameplayTagTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblSetGameplayTagTaskScratchPad();
	virtual ~UAblSetGameplayTagTaskScratchPad();

	/* Actors we tagged. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<AActor>> TaggedActors;
};

UCLASS()
class ABLECORE_API UAblSetGameplayTagTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblSetGameplayTagTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblSetGameplayTagTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;

	/* Returns true if our Task lasts for a single frame. */
	virtual bool IsSingleFrame() const { return !m_RemoveOnEnd; }

	/* Returns the realm our Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler stat ID of our Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblSetGameplayTaskCategory", "Tags"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblSetGameplayTagTask", "Set Gameplay Tag"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblSetGameplayTagDesc", "Sets the supplied Gameplay Tags on the task targets."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(149.0f / 255.0f, 243.0f / 255.0f, 195.0f / 255.0f); }

	/* Returns the estimated runtime cost of our Task. */
	virtual float GetEstimatedTaskCost() const override { return ABLTASK_EST_SYNC; }

	/* Returns true if the user is allowed to edit the realm this Task belongs to. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	/* Tags to Set. */
	UPROPERTY(EditAnywhere, Category="Tags", meta=(DisplayName="Tag List"))
	TArray<FGameplayTag> m_TagList;

	/* If true, all the tags set by this Task will be removed once the task completes. */
	UPROPERTY(EditAnywhere, Category = "Tags", meta = (DisplayName = "Remove On End"))
	bool m_RemoveOnEnd;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE