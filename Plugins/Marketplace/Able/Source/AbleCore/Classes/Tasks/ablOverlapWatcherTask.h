// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"
#include "Components/SceneComponent.h"
#include "Tasks/IAblAbilityTask.h"
#include "Tasks/ablCollisionFilters.h"
#include "Tasks/ablCollisionQueryTypes.h"
#include "WorldCollision.h"

#include "ablOverlapWatcherTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UCLASS(Transient)
class UAblOverlapWatcherTaskScratchPad : public UAblAbilityTaskScratchPad
{
    GENERATED_BODY()
public:
    UAblOverlapWatcherTaskScratchPad();
    virtual ~UAblOverlapWatcherTaskScratchPad();
    
	/* Original Query Transform*/
	UPROPERTY(Transient)
	FTransform QueryTransform;

	/* Async Query Handle*/
	FTraceHandle AsyncHandle;

	/* Whether or not the Async query has been processed. */
	UPROPERTY(transient)
	bool TaskComplete;

	/* Whether or not the Initial Targets have been cleared. */
	UPROPERTY(transient)
	bool HasClearedInitialTargets;

    /* The Overlapped Components of all the actors we affected. */
    UPROPERTY(transient)
    TSet<TWeakObjectPtr<AActor>> IgnoreActors;
};

UCLASS(EditInlineNew, hidecategories = ("Targets", "Optimization"))
class UAblOverlapWatcherTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblOverlapWatcherTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblOverlapWatcherTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

    /* On Task Tick*/
    virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const override { return m_QueryShape ? m_QueryShape->IsAsync() && !m_FireEvent : false; }

    /* Returns true if this task needs its Tick method called. */
    virtual bool NeedsTick() const override { return true; }

	/* Returns the Realm our Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

    /* Returns the Copy Results to Context Parameter. */
    FORCEINLINE bool GetCopyResultsToContext() const { return m_CopyResultsToContext; }

    /* Returns true if our Task is completed and ready for clean up. */
    virtual bool IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

    /* Create the Scratchpad for this Task. */
    virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

#if WITH_EDITOR
	/* Returns the category of this Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblOverlapWatcherCategory", "Blueprint|Collision"); }
	
	/* Returns the name of this Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblOverlapWatcherTask", "Overlap Watcher"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

    /* Returns the Rich Text description, with mark ups. */
    virtual FText GetRichTextTaskSummary() const override;

	/* Returns the description of this Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblOverlapWatcherTaskDesc", "Calls the Blueprint Event on the owning Ability when a Overlap condition is detected."); }
	
	/* Returns the color of this Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0f, 0.0f, 0.6f); } //Pinkish
	
	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const override { return UAblAbilityTask::GetEstimatedTaskCost() + ABLTASK_EST_BP_EVENT; }

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }
	
	/* Returns true if the user is allowed to edit the Tasks realm. */
	virtual bool CanEditTaskRealm() const override { return true; }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Called by Ability Editor to allow any special logic. */
    void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;
#endif

private:
    /* Helper method to copy our query results into our Ability Context. */
    void CopyResultsToContext(const TArray<FAblQueryResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context, bool ClearTargets) const;

	/* Helper that processes the results. */
	void ProcessResults(TArray<FAblQueryResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	/* Helper that does the actual checks.*/
    void CheckForOverlaps(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;
protected:

    /* If true, we'll fire the OnCollisionEvent in the Ability Blueprint. */
    UPROPERTY(EditAnywhere, Category = "Query|Event", meta = (DisplayName = "Fire Event"))
    bool m_FireEvent;

    /* A String identifier you can use to identify this specific task in the ability blueprint. */
    UPROPERTY(EditAnywhere, Category = "Query|Event", meta = (DisplayName = "Name", EditCondition = m_FireEvent))
    FName m_Name;

    UPROPERTY(EditAnywhere, Instanced, Category = "Query", meta = (DisplayName = "Query Shape"))
    UAblCollisionShape* m_QueryShape;

    /* Filters for our results. */
    UPROPERTY(EditAnywhere, Instanced, Category = "Query|Filter", meta = (DisplayName = "Filters"))
    TArray<UAblCollisionFilter*> m_Filters;

    /* If true, the results of the query will be added to the Target Actor Array in the Ability Context. Note this takes 1 full frame to complete.*/
    UPROPERTY(EditAnywhere, Category = "Query|Misc", meta = (DisplayName = "Copy to Context"))
    bool m_CopyResultsToContext;

    /* If true, we won't check for already existing items when copying results to the context.*/
    UPROPERTY(EditAnywhere, Category = "Query|Misc", meta = (DisplayName = "Allow Duplicates", EditCondition = m_CopyResultsToContext))
    bool m_AllowDuplicateEntries;

	/* If true, we'll clear the Target Actor list before copying our context targets in. */
	UPROPERTY(EditAnywhere, Category = "Query|Misc", meta = (DisplayName = "Clear Existing Targets", EditCondition = m_CopyResultsToContext))
	bool m_ClearExistingTargets;

	/* If true, we'll keep clearing the Target Actor list each time we encounter an overlap. Otherwise the clear is only applied on the first overlap attempt.*/
	UPROPERTY(EditAnywhere, Category = "Query|Misc", meta = (DisplayName = "Continually Clear Targets", EditCondition = m_CopyResultsToContext))
	bool m_ContinuallyClearTargets;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;

};

#undef LOCTEXT_NAMESPACE