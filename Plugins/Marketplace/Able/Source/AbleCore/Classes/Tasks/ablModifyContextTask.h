// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablModifyContextTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UCLASS()
class ABLECORE_API UAblModifyContextTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblModifyContextTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblModifyContextTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Returns true if our Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return true; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_ClientAndServer; }

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAblAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblModifyContextCategory", "Context"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblModifyContextTask", "Modify Context"); }

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblModifyContextTaskDesc", "Allows the user to modify the Ability Context at runtime. This can cause a desync in a network environment, so be careful. Takes one frame to complete."); }

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(195.0f / 255.0f, 50.0f / 255.0f, 105.0f / 255.0f); }
#endif

protected:
	/* Target Location to set within the Context.  */
	UPROPERTY(EditAnywhere, Category = "Context", meta = (DisplayName = "Target Location", AblBindableProperty))
	FVector m_TargetLocation;

	UPROPERTY()
	FGetAblVector m_TargetLocationDelegate;

	/* If true, we'll call the Ability's FindCustomTargets method and add those to the Target Actors in the Context. */
	UPROPERTY(EditAnywhere, Category = "Context", meta = (DisplayName = "Additional Targets", AblBindableProperty))
	bool m_AdditionalTargets;

	UPROPERTY()
	FGetAblBool m_AdditionalTargetsDelegate;

	/* If true, we'll clear the current Target Actors first. */
	UPROPERTY(EditAnywhere, Category = "Context", meta = (DisplayName = "Clear Current Targets", AblBindableProperty))
	bool m_ClearCurrentTargets;

	UPROPERTY()
	FGetAblBool m_ClearCurrentTargetsDelegate;
};

#undef LOCTEXT_NAMESPACE