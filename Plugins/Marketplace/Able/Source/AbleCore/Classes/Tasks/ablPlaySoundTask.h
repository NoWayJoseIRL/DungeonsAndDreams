// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ablTargetingBase.h"
#include "Components/AudioComponent.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablPlaySoundTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblPlaySoundTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblPlaySoundTaskScratchPad();
	virtual ~UAblPlaySoundTaskScratchPad();

	/* All the sounds we created. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UAudioComponent>> AttachedSounds;
};

UCLASS()
class ABLECORE_API UAblPlaySoundTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblPlaySoundTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblPlaySoundTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const override;

	/* Returns if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return true; }
	
	/* Returns true if our Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return !m_DestroyOnEnd; }
	
	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_Client; }

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;
	
	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAblAbility* Ability ) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblPlaySoundTaskCategory", "Audio"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblPlaySoundTask", "Play Sound"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblPlaySoundTaskDesc", "Plays a sound at the given location, can be attached to socket, or played as a 2D (rather than 3D) sound."); }

	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(135.0f / 255.0f, 50.0f / 255.0f, 105.0f / 255.0f); }
#endif

protected:
	/* The Sound to play. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Sound", AblBindableProperty))
	USoundBase* m_Sound;

	UPROPERTY()
	FGetAblSound m_SoundDelegate;

	/* Plays the Sound as a 2D sound */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Play as 2D"))
	bool m_2DSound;

	/* The time, within the sound file, to begin playing the Sound from. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Sound Start Time", AblBindableProperty))
	float m_SoundStartTime;

	UPROPERTY()
	FGetAblFloat m_SoundStartTimeDelegate;

	/* Volume modifier to apply with this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Volume Modifier", AblBindableProperty))
	float m_VolumeModifier;

	UPROPERTY()
	FGetAblFloat m_VolumeModifierDelegate;

	/* Pitch modifier to apply with this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Pitch Modifier", AblBindableProperty))
	float m_PitchModifier;

	UPROPERTY()
	FGetAblFloat m_PitchModifierDelegate;

	/* Attenuation settings for this sound.*/
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Attenuation", AblBindableProperty))
	USoundAttenuation* m_Attenuation;

	UPROPERTY()
	FGetAblAttenuation m_AttenuationDelegate;

	/* Concurrency settings for this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Concurrency", AblBindableProperty))
	USoundConcurrency* m_Concurrency;

	UPROPERTY()
	FGetAblConcurrency m_ConcurrencyDelegate;

	/* Location for this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Location", AblBindableProperty))
	FAblAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAblTargetLocation m_LocationDelegate;

	/* Attach the sound to a socket. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Attach to Socket"))
	bool m_AttachToSocket;

	/* Stop the sound when the task ends. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Destroy on End"))
	bool m_DestroyOnEnd;

	/* Stop the sound when the attached Actor is destroyed. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Destroy on Actor Destroy"))
	bool m_DestroyOnActorDestroy;

	/* If the sound is being destroyed early, how long, in seconds, to fade out so we don't have any hard audio stops. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Fade Out Duration"))
	float m_DestroyFadeOutDuration;

	/* Allow the sound to clean up itself once done playing, this can happen outside of the Task and should be left ON by default. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Allow Auto Destroy"))
	bool m_AllowAutoDestroy;
};

#undef LOCTEXT_NAMESPACE