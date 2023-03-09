// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AlphaBlend.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablPlayForcedFeedbackTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblPlayForcedFeedbackTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblPlayForcedFeedbackTaskScratchPad();
	virtual ~UAblPlayForcedFeedbackTaskScratchPad();

    UPROPERTY(transient)
    TArray<TWeakObjectPtr<class APlayerController>> Controllers;
};

UCLASS()
class ABLECORE_API UAblPlayForcedFeedbackTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblPlayForcedFeedbackTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblPlayForcedFeedbackTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
		
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;

	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return false; }
	
	/* Returns true if our Task needs its tick method called. */
	virtual bool NeedsTick() const override { return false; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_Client; }

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblPlayForcedFeedbackCategory", "Player"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblPlayForcedFeedback", "Play Force Feedback (Haptics)"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblPlayForcedFeedbackDesc", "Starts/stops a force feedback effect on the player controllers of the target pawns."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0, 0.69, 0.4f); } // Peach
#endif
protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Effect"))
    class UForceFeedbackEffect* ForceFeedbackEffect;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Start Tag Name"))
    FName StartTagName;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Looping"))
    bool bLooping;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Ignore Time Dilation"))
    bool bIgnoreTimeDilation;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Play while Paused"))
    bool bPlayWhilePaused;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Stop Tag Name"))
    FName StopTagName;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Stop On Task Exit"))
    bool StopOnTaskExit;
};

#undef LOCTEXT_NAMESPACE
