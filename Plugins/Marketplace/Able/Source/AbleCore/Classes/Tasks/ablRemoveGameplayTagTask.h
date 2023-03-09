// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"
#include "ablRemoveGameplayTagTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UCLASS()
class ABLECORE_API UAblRemoveGameplayTagTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblRemoveGameplayTagTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblRemoveGameplayTagTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Returns true if our Task lasts for only a single frame. */
	virtual bool IsSingleFrame() const { return true; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblRemoveGameplayTaskCategory", "Tags"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblRemoveGameplayTagTask", "Remove Gameplay Tag"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblRemoveGameplayTagDesc", "Removes the supplied Gameplay Tags on the task targets."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(239.0f / 255.0f, 153.0f / 255.0f, 24.0f / 255.0f); }

	/* How to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const override { return EVisibility::Collapsed; }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	/* The Tags to Remove. */
	UPROPERTY(EditAnywhere, Category = "Tags", meta = (DisplayName = "Tag List"))
	TArray<FGameplayTag> m_TagList;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE