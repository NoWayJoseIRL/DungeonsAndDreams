// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablPlayAnimationTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

class UAnimationAsset;
class UAblAbilityComponent;
class UAblAbilityContext;

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblPlayAnimationTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblPlayAnimationTaskScratchPad();
	virtual ~UAblPlayAnimationTaskScratchPad();

	/* The Ability Components of all the actors we affected. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UAblAbilityComponent>> AbilityComponents;

	/* The Skeletal Mesh Components of all the actors we affected (Single Node only). */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<USkeletalMeshComponent>> SingleNodeSkeletalComponents;
};

UENUM()
enum EAblPlayAnimationTaskAnimMode
{
	SingleNode UMETA(DisplayName="Single Node"),
	AbilityAnimationNode UMETA(DisplayName = "Ability Animation Node"),
	DynamicMontage UMETA(DisplayName = "Dynamic Montage")
};

UCLASS()
class ABLECORE_API UAblPlayAnimationTask : public UAblAbilityTask
{
	GENERATED_BODY()

public:
	UAblPlayAnimationTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblPlayAnimationTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;

	/* Check if our Task is finished. */
	virtual bool IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Returns the End time of our Task. */
	virtual float GetEndTime() const override;

	/* Returns which realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_PlayOnServer ? EAblAbilityTaskRealm::ATR_ClientAndServer : EAblAbilityTaskRealm::ATR_Client; }

	/* Creates the Scratchpad for our Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Called when our Time has been set. */
	virtual void OnAbilityTimeSet(const TWeakObjectPtr<const UAblAbilityContext>& Context) override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAblAbility* Ability ) override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblPlayAnimationTaskCategory", "Animation"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblPlayAnimationTask", "Play Animation"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the Rich Text description, with mark ups. */
	virtual FText GetRichTextTaskSummary() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblPlayAnimationTaskDesc", "Plays an Animation asset (currently only Animation Montage and Animation Segments are supported)."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(177.0f / 255.0f, 44.0f / 255.0f, 87.0f / 255.0f); } // Light Blue
	
	/* Returns the estimated runtime cost of our Task. */
	virtual float GetEstimatedTaskCost() const override { return ABLTASK_EST_SYNC; }

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const override { return m_ManuallySpecifyAnimationLength ? EVisibility::Visible : EVisibility::Hidden; } // Hidden = Read only (it's still using space in the layout, so might as well display it).

    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;
#endif
	/* Returns the Animation Asset. */
	FORCEINLINE const UAnimationAsset* GetAnimationAsset() const { return m_AnimationAsset; }

    /* Returns the Ability State Name. */
    FORCEINLINE const FName& GetAnimationMontageSectionName() const { return m_AnimationMontageSection; }

	/* Sets the Animation Asset. */
	void SetAnimationAsset(UAnimationAsset* Animation);

	/* Returns the Animation Mode. */
	FORCEINLINE EAblPlayAnimationTaskAnimMode GetAnimationMode() const { return m_AnimationMode.GetValue(); }
	
	/* Returns the State Machine Name. */
	FORCEINLINE const FName& GetStateMachineName() const { return m_StateMachineName; }
	
	/* Returns the Ability State Name. */
	FORCEINLINE const FName& GetAbilityStateName() const { return m_AbilityStateName; }

protected:
	/* Helper method to clean up code a bit. This method does the actual PlayAnimation/Montage_Play/etc call.*/
	void PlayAnimation(const TWeakObjectPtr<const UAblAbilityContext>& Context, const UAnimationAsset* AnimationAsset, const FName& MontageSection, AActor& TargetActor, UAblPlayAnimationTaskScratchPad& ScratchPad, USkeletalMeshComponent& SkeletalMeshComponent, float PlayRate) const;

	/* Helper method to find the AbilityAnimGraph Node, if it exists. */
	struct FAnimNode_AbilityAnimPlayer* GetAbilityAnimGraphNode(const TWeakObjectPtr<const UAblAbilityContext>& Context, USkeletalMeshComponent* MeshComponent) const;

	// The Animation to play.
    UPROPERTY(EditAnywhere, Category="Animation", meta = (DisplayName = "Animation", AblBindableProperty, AblDefaultBinding = "OnGetAnimationAssetBP", AllowedClasses = "AnimMontage,AnimSequence"))
	UAnimationAsset* m_AnimationAsset;

	UPROPERTY()
	FGetAblAnimation m_AnimationAssetDelegate;

	/* The animation montage section to jump to if using Dynamic Montages. */
    UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Montage Section", AblBindableProperty))
    FName m_AnimationMontageSection;

	UPROPERTY()
	FGetAblName m_AnimationMontageSectionDelegate;

	/* If set, Jump to this montage section when the Task ends, if using Dynamic Montages. Set it to Name_None to disable this feature.*/
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Montage Section on End", AblBindableProperty ))
	FName m_OnEndAnimationMontageSection;

	UPROPERTY()
	FGetAblName m_OnEndAnimationMontageSectionDelegate;

	/* What mode to use for this task. 
	*  Single Node - Plays the Animation as a Single Node Animation outside of the Animation Blueprint (if there is one).
	*  Ability Animation Node - Plays the Animation using the Ability Animation Node within an Animation State Machine.
	*  Dynamic Montage - Plays the Animation as a Dynamic Montage.
	*/
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Animation Mode", EditCondition = "m_AnimationAsset!=nullptr"))
	TEnumAsByte<EAblPlayAnimationTaskAnimMode> m_AnimationMode;

	/* The name of the State Machine we should look for our Ability State in.*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "State Machine Name", AblBindableProperty, EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FName m_StateMachineName;

	UPROPERTY()
	FGetAblName m_StateMachineNameDelegate;

	/* The name of the State that contains the Ability Animation Player Node*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "Ability State Name", AblBindableProperty, EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FName m_AbilityStateName;

	UPROPERTY()
	FGetAblName m_AbilityStateNameDelegate;

	/* If the node is already playing an animation, this is the blend used transition to this animation.*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "Blend In", EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FAlphaBlend m_BlendIn;

	/* If the node doesn't have any more animations to play, this is the blend used transition out of this animation. NOTE: Currently unused.*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "Blend Out", EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FAlphaBlend m_BlendOut;

	// Whether or not to loop the animation.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Loop", EditCondition="m_AnimationAsset!=nullptr"))
	bool m_Loop;

	// Blend times to use when playing the animation as a dynamic montage.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Play Blend", AblBindableProperty, EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::DynamicMontage"))
	FAblBlendTimes m_DynamicMontageBlend;

	UPROPERTY()
	FGetAblBlendTimes m_DynamicMontageBlendDelegate;

	// Slot Name to use for the Sequence when playing as a dynamic montage.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Slot Name", AblBindableProperty, EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::DynamicMontage"))
	FName m_SlotName;

	UPROPERTY()
	FGetAblName m_SlotNameDelegate;

	// Used when attempting to play a Slot Animation Asset as a Dynamic Montage, or playing a Montage Asset (including as a Single Node Animation). Offsets the starting time of the animation.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Time To Start At", AblBindableProperty, EditCondition = "m_AnimationMode != EAblPlayAnimationTaskAnimMode::AbilityAnimationNode"))
	float m_TimeToStartMontageAt;

	UPROPERTY()
	FGetAblFloat m_TimeToStartMontageAtDelegate;

	// Used when attempting to play a Slot Animation Asset as a Dynamic Montage. Offsets the Blend out time. 
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Blend Out Trigger Time", AblBindableProperty, EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::DynamicMontage"))
	float m_BlendOutTriggerTime;

	UPROPERTY()
	FGetAblFloat m_BlendOutTriggerTimeDelegate;

	// Used when attempting to play a Slot Animation Asset as a Dynamic Montage. Loops the Animation N times.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Number Of Loops", AblBindableProperty, EditCondition = "m_AnimationMode == EAblPlayAnimationTaskAnimMode::DynamicMontage"))
	int32 m_NumberOfLoops;

	UPROPERTY()
	FGetAblInt m_NumberOfLoopsDelegate;

	// Used when attempting to play a Montage Asset (including as a Single Node Animation). Stops all existing montages.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Stop All Montages", AblBindableProperty, EditCondition = "m_AnimationMode != EAblPlayAnimationTaskAnimMode::AbilityAnimationNode"))
	bool m_StopAllMontages;

	UPROPERTY()
	FGetAblBool m_StopAllMontagesDelegate;

	// Animation Play Rate
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Play Rate", AblBindableProperty, EditCondition = "m_AnimationAsset!=nullptr"))
	float m_PlayRate;

	UPROPERTY()
	FGetAblFloat m_PlayRateDelegate;

	// If true, we scale our Animation Play Rate by what our Ability Play Rate is. So, if your Ability Play Rate is 2.0, the animation play rate is multiplied by that same value.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Scale With Ability Play Rate", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ScaleWithAbilityPlayRate;

	// If true, stop the current playing animation mid blend if the owning Ability is interrupted. Only applies when Animation Mode is set to Ability Animation Node.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Stop on Interrupt", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_StopAnimationOnInterrupt;

	// If true, any queued up (using the Ability Animation Node) animations will be removed as well. 
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Clear Queued Animation On Interrupt", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ClearQueuedAnimationOnInterrupt;

	// If true, Able will try to reset you into whatever Animation State you were in when the Task started (e.g., Single Instance vs Animation Blueprint).
	// You can disable this if you see it causing issues, as this does reset the animation instance time when it's called.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Reset Animation State On End", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ResetAnimationStateOnEnd;

	// If true, you can tell Able how long this task (and thus the animation) should play. Play rate still modifies this length. 
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Manually Specified Animation Length", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ManuallySpecifyAnimationLength;

	/* If true, we'll treat a manually specified length as an interrupt - so normal rules for stopping, clearing the queue, etc apply. */
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Manual Length Is Interrupt", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ManualLengthIsInterrupt;

	/* A String identifier you can use to identify this specific task in the Ability blueprint, when GetSkeletalMeshComponentForActor is called on the Ability. */
	UPROPERTY(EditAnywhere, Category = "Animation|Dynamic", meta = (DisplayName = "Event Name"))
	FName m_EventName;
	
	/* If true, in a networked game, the animation will be played on the server. 
	*  You should only use this if you have collision queries that rely on bone positions
	*  or animation velocities.
	*/
	UPROPERTY(EditAnywhere, Category = "Network", meta=(DisplayName="Play On Server", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_PlayOnServer;
};

#undef LOCTEXT_NAMESPACE
