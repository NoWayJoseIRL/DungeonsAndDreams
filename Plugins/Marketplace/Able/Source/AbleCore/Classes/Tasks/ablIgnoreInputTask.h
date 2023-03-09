// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AlphaBlend.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablIgnoreInputTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblIgnoreInputTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblIgnoreInputTaskScratchPad();
	virtual ~UAblIgnoreInputTaskScratchPad();

    UPROPERTY(transient)
    TArray<TWeakObjectPtr<class APawn>> Pawns;
};

UCLASS()
class ABLECORE_API UAblIgnoreInputTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblIgnoreInputTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblIgnoreInputTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
		
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;

	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return !m_MoveInput && !m_LookInput; }
	
	/* Returns true if our Task needs its tick method called. */
	virtual bool NeedsTick() const override { return false; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_ClientAndServer; } // Client for Auth client, Server for AIs/Proxies.

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblIgnoreInputTaskCategory", "Movement"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblIgnoreInput", "Ignore Input"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblIgnoreInputTaskDesc", "Ignores move/look input while the task is active."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0, 0.69, 0.4f); } // Peach

	// UObject Overrides
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
protected:
    UPROPERTY(EditAnywhere, Category = "Ignore Input", meta = (DisplayName = "Move Input"))
    bool m_MoveInput = true;

	UPROPERTY(EditAnywhere, Category = "Ignore Input", meta = (DisplayName = "Look Input"))
	bool m_LookInput = false;

    UPROPERTY(EditAnywhere, Category = "Ignore Input", meta = (DisplayName = "Input"))
    bool m_Input = false;
};

#undef LOCTEXT_NAMESPACE
