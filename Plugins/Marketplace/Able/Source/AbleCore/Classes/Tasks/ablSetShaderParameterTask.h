// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AlphaBlend.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablSetShaderParameterTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

class UAblSetParameterValue;

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblSetShaderParameterTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblSetShaderParameterTaskScratchPad();
	virtual ~UAblSetShaderParameterTaskScratchPad();

	/* All the Dynamic Materials we've affected. */
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	/* The previous value of any parameters we've set. */
	UPROPERTY()
	TArray<UAblSetParameterValue*> PreviousValues;

	/* Blend In Time. */
	UPROPERTY()
	FAlphaBlend BlendIn;

	/* Blend Out Time. */
	UPROPERTY()
	FAlphaBlend BlendOut;
};

UCLASS()
class ABLECORE_API UAblSetShaderParameterTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblSetShaderParameterTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblSetShaderParameterTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* Task On Tick. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const override;
	
	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return false; } 	// I'd love to run this Async given the blending, etc. But creating the MID isn't thread safe. :(
	
	/* Returns true if our Task lasts for a single frame. */
	virtual bool IsSingleFrame() const { return m_BlendIn.IsComplete() && !m_RestoreValueOnEnd; } // Only tick if we have a blend to do, or we have to restore our value at the end.
	
	/* Returns true if our Task needs its Tick method called. */
	virtual bool NeedsTick() const override { return !IsSingleFrame(); }
	
	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_Client; }

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler stat ID of our Task. */
	virtual TStatId GetStatId() const override;

	/* Get our Value to set.*/
	const UAblSetParameterValue* GetParam() const { return m_Value; }
#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblSetShaderParameterCategory", "Material"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblSetShaderParameterTask", "Set Material Parameter"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblSetShaderParameterTaskDesc", "Sets a dynamic parameter on the Target's material instance."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(82.0f / 255.0f, 137.0f / 255.0f, 237.0f / 255.0f); }

	// UObject Overrides. 
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	/* Fix up our flags. */
	virtual bool FixUpObjectFlags() override;
#endif

private:
	/* Helper Method to cache current shader parameter values. */
	UAblSetParameterValue* CacheShaderValue(UAblSetShaderParameterTaskScratchPad* ScratchPad, UMaterialInstanceDynamic* DynMaterial) const;
	
	/* Helper method to set Shader parameters. */
	void InternalSetShaderValue(const TWeakObjectPtr<const UAblAbilityContext>& Context, UMaterialInstanceDynamic* DynMaterial, UAblSetParameterValue* Value, UAblSetParameterValue* PreviousValue, float BlendAlpha) const;
	
	/* Helper method, returns true if the Material has the parameter this Task is looking for. */
	bool CheckMaterialHasParameter(UMaterialInterface* Material) const;

	/* Bind our Dynamic Delegates. */
	virtual void BindDynamicDelegates(UAblAbility* Ability) override;
protected:
	/* The name of our Shader Parameter. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Name"))
	FName m_ParameterName;

	/* The Shader Parameter value to set. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Parameter", meta=(DisplayName="Value"))
	UAblSetParameterValue* m_Value;

	/* When setting the Shader parameter, this is the blend we will use as we transition over. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Blend In"))
	FAlphaBlend m_BlendIn;

	/* If true, restore the value of the variable when we started the task. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Restore Value On End"))
	bool m_RestoreValueOnEnd;

	/* If restoring the value at the end, this blend is used when transition back to the original value. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Blend Out", EditCondition ="m_RestoreValueOnEnd"))
	FAlphaBlend m_BlendOut;
};

#undef LOCTEXT_NAMESPACE