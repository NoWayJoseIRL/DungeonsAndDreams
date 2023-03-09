// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ablTargetingBase.h"
#include "Tasks/ablPlayParticleEffectParams.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"
#include "Niagara/Classes/NiagaraSystem.h"
#include "Niagara/Public/NiagaraComponent.h"

#include "ablPlayParticleEffectTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

class UParticleSystem;
class UParticleSystemComponent;
class UFXSystemComponent;

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblPlayParticleEffectTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblPlayParticleEffectTaskScratchPad();
	virtual ~UAblPlayParticleEffectTaskScratchPad();

	/* All the Particle effects we've spawned. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UParticleSystemComponent>> SpawnedEffects;

	/* All the Niagara Particle effects we've spawned. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UNiagaraComponent>> SpawnedNiagaraEffects;
};

UCLASS()
class ABLECORE_API UAblPlayParticleEffectTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblPlayParticleEffectTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblPlayParticleEffectTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* Update our Task. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;

	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const override;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const override { return true; }
	
	/* Returns true if the Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return !m_DestroyAtEnd; }

	/* Returns true if our Task requires a Tick update.*/
	virtual bool NeedsTick() const override;
	
	/* Returns the realm of our Task. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_Client; }

	/* Creates the Scratchpad for our Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID of our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAblAbility* Ability ) override;

	/* Get our Parameter values. */
	const TArray<UAblParticleEffectParam*>& GetParams() const { return m_Parameters; }

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblPlayParticleEffectTaskCategory", "Effects"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblPlayParticleEffectTask", "Play Particle Effect"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblPlayParticleEffectTaskDesc", "Plays a particle effect, can be attached to a bone."); }
	
	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(149.0f / 255.0f, 61.0f / 255.0f, 73.0f / 255.0f); }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Fix up our flags. */
	virtual bool FixUpObjectFlags() override;
#endif

protected:
	void ApplyParticleEffectParameter(const UAblParticleEffectParam* Parameter, const TWeakObjectPtr<const UAblAbilityContext>& Context, UFXSystemComponent* EffectSystem, TWeakObjectPtr<UParticleSystemComponent>& CascadeComponent) const;

	/* Particle Effect to play (has priority over a Niagara system if both are specified). */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (AblBindableProperty, DisplayName="Effect Template"))
	UParticleSystem* m_EffectTemplate;

	UPROPERTY()
	FGetAblParticleSystem m_EffectTemplateDelegate;

	/* Niagara Effect to play (does NOT have priority over a Particle System if both are specified). */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (AblBindableProperty, DisplayName = "Niagara Template"))
	UNiagaraSystem* m_NiagaraEffectTemplate;

	UPROPERTY()
	FGetAblNiagaraSystem m_NiagaraEffectTemplateDelegate;

	UPROPERTY(EditAnywhere, Category = "Particle", meta=(DisplayName = "Location", AblBindableProperty))
	FAblAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAblTargetLocation m_LocationDelegate;
    
	/* If true, the particle effect will follow the transform of the socket. */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Attach To Socket"))
	bool m_AttachToSocket;

	/* Uniform scale applied to the particle effect. */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Scale", AblBindableProperty))
	float m_Scale;

	UPROPERTY()
	FGetAblFloat m_ScaleDelegate;

    /* If non zero, used to calculate a scaling factor based on the target actor radius. */
    UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Dynamic Scale", AblBindableProperty))
    float m_DynamicScaleSize;

	UPROPERTY()
	FGetAblFloat m_DynamicScaleSizeDelegate;

	/* Whether or not we destroy the effect at the end of the task. */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Destroy on End"))
	bool m_DestroyAtEnd;

	/* Context Driven Parameters to set on the Particle instance.*/
	UPROPERTY(EditAnywhere, Instanced, Category = "Particle", meta = (DisplayName = "Instance Parameters"))
	TArray<UAblParticleEffectParam*> m_Parameters;
};

#undef LOCTEXT_NAMESPACE
