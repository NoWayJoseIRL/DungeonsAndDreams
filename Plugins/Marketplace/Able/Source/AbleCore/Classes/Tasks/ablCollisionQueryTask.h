// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/ablCollisionQueryTypes.h"
#include "Tasks/ablCollisionFilters.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"
#include "WorldCollision.h"

#include "ablCollisionQueryTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Collision Queries. */
UCLASS(Transient)
class UAblCollisionQueryTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblCollisionQueryTaskScratchPad();
	virtual ~UAblCollisionQueryTaskScratchPad();

	/* Original Query Transform*/
	UPROPERTY(Transient)
	FTransform QueryTransform;

	/* Async Query Handle*/
	FTraceHandle AsyncHandle;

	/* Whether or not the Async query has been processed. */
	UPROPERTY(transient)
	bool AsyncProcessed;
};

UCLASS()
class ABLECORE_API UAblCollisionQueryTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblCollisionQueryTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionQueryTask();

	/* Start our Task.*/
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* On Task Tick*/
	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;
	
	/* Returns true if our Task supports Async. */
	virtual bool IsAsyncFriendly() const override { return m_QueryShape ? m_QueryShape->IsAsync() && !m_FireEvent : false; } 
	
	/* Returns true if our Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return m_QueryShape ? !m_QueryShape->IsAsync() : true; }
	
	/* Returns the Realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	/* Return the End Time of our Task. */
	virtual float GetEndTime() const override { return GetStartTime() + 0.05f; }

	/* Returns true if our Task is completed and ready for clean up. */
	virtual bool IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Creates the Scratchpad for our Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

    /* Returns the Copy Results to Context Parameter. */
    FORCEINLINE bool GetCopyResultsToContext() const { return m_CopyResultsToContext; }

#if WITH_EDITOR
	/* Returns the Task Category. */
	virtual FText GetTaskCategory() const { return LOCTEXT("AblCollisionQueryTaskCategory", "Collision"); }
	
	/* Returns the name of the Task. */
	virtual FText GetTaskName() const { return LOCTEXT("AblCollisionQueryTask", "Collision Query"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of the Task. */
	virtual FText GetTaskDescription() const { return LOCTEXT("AblCollisionQueryTaskDesc", "Performs a shape based query in the collision scene and returns any entities inside or outside the query."); }
	
	/* Returns the color of this Task. */
	virtual FLinearColor GetTaskColor() const { return FLinearColor(109.0f / 255.0f, 226.0f / 255.0f, 60.0f / 255.0f); }
	
	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const { return UAblAbilityTask::GetEstimatedTaskCost() + ABLTASK_EST_COLLISION_SIMPLE_QUERY; }

	/* Returns how to display the End time of this Task. */
	virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }
	
	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
	
	/* Returns the Fire Event Parameter. */
	FORCEINLINE bool GetFireEvent() const { return m_FireEvent; }

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Called by Ability Editor to allow any special logic. */
    void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Fix up any flags. */
	virtual bool FixUpObjectFlags() override;
#endif

	/* Query Shape. */
	const UAblCollisionShape* GetShape() const { return m_QueryShape; }

	/* Query Filters */
	const TArray<UAblCollisionFilter*> GetFilters() const { return m_Filters; }
private:
	/* Helper method to copy our query results into our Ability Context. */
	void CopyResultsToContext(const TArray<FAblQueryResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	/* Bind our Dynamic Delegates. */
	virtual void BindDynamicDelegates(UAblAbility* Ability) override;

protected:
	/* If true, we'll fire the OnCollisionEvent in the Ability Blueprint. */
	UPROPERTY(EditAnywhere, Category = "Query|Event", meta = (DisplayName = "Fire Event"))
	bool m_FireEvent;

	/* A String identifier you can use to identify this specific task in the ability blueprint. */
	UPROPERTY(EditAnywhere, Category = "Query|Event", meta = (DisplayName = "Name", EditCondition=m_FireEvent))
	FName m_Name;

	/* The shape for our query. */
	UPROPERTY(EditAnywhere, Instanced, Category="Query", meta=(DisplayName="Query Shape"))
	UAblCollisionShape* m_QueryShape;

	/* Filters for our results. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Query|Filter", meta = (DisplayName = "Filters"))
	TArray<UAblCollisionFilter*> m_Filters;

	/* If true, the results of the query will be added to the Target Actor Array in the Ability Context. Note this takes 1 full frame to complete.*/
	UPROPERTY(EditAnywhere, Category = "Query|Misc", meta = (DisplayName = "Copy to Context"))
	bool m_CopyResultsToContext;

	/* If true, we won't check for already existing items when copying results to the context.*/
	UPROPERTY(EditAnywhere, Category = "Query|Misc", meta = (DisplayName = "Allow Duplicates", EditCondition=m_CopyResultsToContext))
	bool m_AllowDuplicateEntries;

	/* If true, we'll clear the Target Actor list before copying our context targets in. */
	UPROPERTY(EditAnywhere, Category = "Query|Misc", meta = (DisplayName = "Clear Existing Targets", EditCondition = m_CopyResultsToContext))
	bool m_ClearExistingTargets;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE