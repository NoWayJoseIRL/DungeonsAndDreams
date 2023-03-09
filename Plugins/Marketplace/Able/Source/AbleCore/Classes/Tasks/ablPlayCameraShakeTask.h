// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AlphaBlend.h"
#include "Camera/CameraShakeBase.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"
#include "ablAbilityTypes.h"

#include "ablPlayCameraShakeTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblPlayerCameraShakeTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblPlayerCameraShakeTaskScratchPad();
	virtual ~UAblPlayerCameraShakeTaskScratchPad();

    UPROPERTY(transient)
    TArray<TWeakObjectPtr<class APlayerController>> Controllers;

	UPROPERTY(transient)
	TArray<TSubclassOf<UCameraShakeBase>> ShakeClasses;
};

UCLASS()
class ABLECORE_API UAblPlayerCameraShakeTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblPlayerCameraShakeTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblPlayerCameraShakeTask();

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

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAblAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblPlayerCameraShakeCategory", "Player"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblPlayerCameraShake", "Play Camera Shake"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblPlayerCameraShakeDesc", "Starts/stops a camera shake on the player controllers of the target pawns."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0, 0.69, 0.4f); } // Peach
#endif
protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake", meta = (DisplayName = "Shake", AblBindableProperty))
    TSubclassOf<UCameraShakeBase> Shake;

	UPROPERTY()
	FGetCameraShakeClass ShakeDelegate;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake", meta = (DisplayName = "Shake Scale", AblBindableProperty))
    float ShakeScale;

	UPROPERTY()
	FGetAblFloat ShakeScaleDelegate;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake", meta = (DisplayName = "Shake World Space", AblBindableProperty))
    bool ShakeInWorldSpace;

	UPROPERTY()
	FGetAblBool ShakeInWorldSpaceDelegate;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake", meta = (DisplayName = "Stop Immediately on Exit", AblBindableProperty))
    EAblPlayCameraShakeStopMode StopMode;

	UPROPERTY()
	FGetCameraStopMode StopModeDelegate;
};

#undef LOCTEXT_NAMESPACE
