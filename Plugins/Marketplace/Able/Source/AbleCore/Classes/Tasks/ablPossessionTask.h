// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablPossessionTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblPossessionTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblPossessionTaskScratchPad();
	virtual ~UAblPossessionTaskScratchPad();

	/* The controller we've possessed. */
	UPROPERTY(transient)
	TWeakObjectPtr<APlayerController> PossessorController;
};

UCLASS()
class ABLECORE_API UAblPossessionTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblPossessionTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblPossessionTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;
	
	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return !m_UnPossessOnEnd; }

	/* Returns the realm our Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_Server; }

	/* Creates the Scratchpad for our Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual  TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblPossessionTaskCategory", "Misc"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblPossessionTask", "Possess"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblPossessionTaskDesc", "Causes the player to possess the target."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(204.0f / 255.0f, 169.0f / 255.0f, 45.0f / 255.0f); }
#endif
protected:
	/* The Context Target that will be possessing the target. */
	UPROPERTY(EditAnywhere, Category = "Possession", meta = (DisplayName = "Possessor"))
	TEnumAsByte<EAblAbilityTargetType> m_Possessor;

	/* The Context Target that will be possessed. */
	UPROPERTY(EditAnywhere, Category="Possession", meta=(DisplayName="Possession Target"))
	TEnumAsByte<EAblAbilityTargetType> m_PossessionTarget;

	/* If true, the Possession is undone at the end of the Task. */
	UPROPERTY(EditAnywhere, Category = "Possession", meta = (DisplayName = "UnPossess On End"))
	bool m_UnPossessOnEnd;
};

#undef LOCTEXT_NAMESPACE