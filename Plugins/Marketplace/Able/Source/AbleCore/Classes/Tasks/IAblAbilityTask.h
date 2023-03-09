// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityTypes.h"
#include "Misc/Attribute.h"
#include "Serialization/Archive.h"
#include "Targeting/ablTargetingBase.h"
#include "Tasks/ablTaskWeights.inl"
#include "UObject/ObjectMacros.h"

#if WITH_EDITOR
#include "Runtime/SlateCore/Public/Layout/Visibility.h"
#endif

#include "IAblAbilityTask.generated.h"

class UAblAbilityContext;
class UAblAbilityComponent;
class UAblAbility;

#define LOCTEXT_NAMESPACE "AbleCore"


#if WITH_EDITORONLY_DATA
// Event for other objects that need to watch for changes in the Task.
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAblAbilityTaskPropertyModified, class UAblAbilityTask& /*Task*/, struct FPropertyChangedEvent& /*PropertyChangedEvent*/);
#endif

USTRUCT()
struct FAblCompactTaskData
{
	GENERATED_BODY()
public:
	FAblCompactTaskData() {};
	virtual ~FAblCompactTaskData() {};

	UPROPERTY()
	TSoftClassPtr<UAblAbilityTask> TaskClass;

	UPROPERTY()
	TArray<TSoftObjectPtr<const UAblAbilityTask>> Dependencies;

	UPROPERTY()
	TArray<uint8> DataBlob;
};

/* Tasks are meant to be stateless, as you could have multiple actors referencing the same
 * ability, which are then executing the same tasks. To avoid copying the entire ability(or even just the tasks)
 * a Task can create a custom type of scratch pad and it'll automatically be instantiated and passed to the task so you can
 * keep any state changes you need there. 
 */

UCLASS(Abstract, Transient)
class ABLECORE_API UAblAbilityTaskScratchPad : public UObject
{
	GENERATED_BODY()
public:
	UAblAbilityTaskScratchPad() { };
	virtual ~UAblAbilityTaskScratchPad() { };
};

/* Tasks execute a single bit of logic. They are the building blocks of an ability.*/
UCLASS(Abstract, EditInlineNew, HideCategories="Internal")
class ABLECORE_API  UAblAbilityTask : public UObject
{
	GENERATED_BODY()

protected:
	UAblAbilityTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTask();

public:
	/* UObject override to fix up any properties. */
	virtual void PostInitProperties() override;
    void PostLoad() override;
	virtual UWorld* GetWorld() const override;
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack) override;

	/* Called to determine is a Task can start. Default behavior is to simply check if our context time is > our start time. */
	virtual bool CanStart(const TWeakObjectPtr<const UAblAbilityContext>& Context, float CurrentTime, float DeltaTime) const;

	/* Called as soon as the task is started. Do any per-run initialization here.*/
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	/* Called per tick if NeedsTick returns true. Not called on the 1st frame (OnTaskStart is called instead).*/
	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const;

	/* Called to determine if a Task can end. Default behavior is to see if our context time is > than our end time. */
	virtual bool IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	/* Called when a task is ending. The result tells us why the task is ending.*/
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const;

	/* Return true if the task will be ticked each frame during its execution.*/
	virtual bool NeedsTick() const { return !IsSingleFrame(); }

	/* Returns the Duration of the Task.*/
	UFUNCTION(BlueprintPure, Category = "Able|Task")
	float GetDuration() const {	return GetEndTime() - GetStartTime();	}

	UFUNCTION(BlueprintCallable, Category = "Able|Task")
	float GetCurrentDurationAsRatio(const UAblAbilityContext* Context) const;
	
	/* Return the Start time of the Task.*/
	FORCEINLINE virtual float GetStartTime() const { return m_StartTime; }
	
	/* Returns the End time of the Task.*/
	FORCEINLINE virtual float GetEndTime() const { return IsSingleFrame() ? GetStartTime() + KINDA_SMALL_NUMBER : m_EndTime; }
	
	/* Returns whether this Task only occurs during a Single Frame, or not. */
	FORCEINLINE virtual bool  IsSingleFrame() const { return false; }

	/* Returns whether this Task can be executed Asynchronously or not. */
	FORCEINLINE virtual bool  IsAsyncFriendly() const { return false; }

	/* Returns the Realm (Client/Server/Both) that this Task is allowed to execute on. */
	FORCEINLINE virtual EAblAbilityTaskRealm GetTaskRealm() const { return EAblAbilityTaskRealm::ATR_Client; }

	/* Returns whether this Task has any dependencies or not. */
	FORCEINLINE bool HasDependencies() const { return m_Dependencies.Num() != 0; }

	/* Returns any Tasks we are dependent on. */
	FORCEINLINE const TArray<const UAblAbilityTask*>& GetTaskDependencies() const { return m_Dependencies; }

	/* Returns whether to run in verbose mode or not. */
	FORCEINLINE bool IsVerbose() const { return m_Verbose; }

	/* Returns whether execute the Task or not. */
	FORCEINLINE bool IsAlwaysDisabled() const { return m_Disabled; }
	
    /* Returns whether execute the Task or not, possibly based on runtime logic. */
    bool IsDisabled(const TWeakObjectPtr<UAblAbilityContext>& Context) const;
    
	/* Returns whether to run in verbose mode or not. */
	FORCEINLINE const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }
	
	/* Called when a Task is about to begin execution. Used to allocate any run-specific memory requirements. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const { return nullptr; }

	/* Returns the StatId for this Task, used by the Profiler. */
	virtual TStatId GetStatId() const { checkNoEntry(); return TStatId(); }

	bool IsValidForNetMode(ENetMode NetMode) const;

	/* Call back for when the time has changed unexpectedly (normally due to timeline scrubbing in the editor). */
	virtual void OnAbilityTimeSet(const TWeakObjectPtr<const UAblAbilityContext>& Context) { };

	FName GetDynamicDelegateName( const FString& PropertyName ) const;

	virtual void BindDynamicDelegates( UAblAbility* Ability );

	/* If this Task should be inherited by derived Abilities */
    bool IsInheritable() const { return m_Inheritable; }

	/* Returns a modifiable version of our Task dependencies. */
	TArray<const UAblAbilityTask*>& GetMutableTaskDependencies() { return m_Dependencies; }

	/* Returns if we should reset with Iterations or not. */
	bool GetResetForIterations() const { return m_ResetForIteration; }

#if WITH_EDITOR
	/* Returns the Category for this task. Supports Subcategories using the | character (e.g. Main Character|Sub Category|etc). */
	virtual FText GetTaskCategory() const { return LOCTEXT("AblAbilityTaskCategory", "Misc"); }

	/* Returns the Tasks name. */
	virtual FText GetTaskName() const { return LOCTEXT("AblAbilityTask", "Unknown Task"); }

	/* Returns a more descriptive, dynamically constructed Task name. */
	virtual FText GetDescriptiveTaskName() const { return GetTaskName(); }

	/* Returns a description of what the Task does. */
	virtual FText GetTaskDescription() const { return FText::GetEmpty(); }

	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;

	/* The color to use to represent this Task in the Editor. */
	virtual FLinearColor GetTaskColor() const { return FLinearColor::White;  }

	/* If our Task color is not black, then the user picked a value. */
	bool HasUserDefinedTaskColor() const { return m_OverrideTaskColor; }

	/* Our User defined Task Color.*/
	const FLinearColor& GetUserDefinedTaskColor() const { return m_TaskColor; }
	
	/* Estimated Task Cost is used to give a rough estimate on how "expensive" the task, and thus the ability itself, is.
	 * The value is between 0.0 - 1.0 where 0.0 is "free" and 1.0 is computationally burdensome.
	 * Things to consider when decided on an estimate:
	 * - Does the task require Ticking? 
	 *    - Is it Async friendly? 
	 * - Does it fire a blueprint event? 
	 * - Does it do any type of collision query (is it a simple sphere query, or a more expensive raycast)? 
	 * 
	 * In the end, this is just a very rough guess to help your content creators. As always, profiling tools are your best bet for
	 * truly hunting down performance hot spots.
	 *
	 * Feel free to use some of the defines above as an easy way to estimate.*/
	virtual float GetEstimatedTaskCost() const;

	/* Sets the Start time of the Task. */
	void SetStartTime(float Time) { m_StartTime = Time; }

	/* Sets the End time of the Task. */
	void SetEndTime(float Time) { m_EndTime = Time; }

	/* The End Time of our Task can have various behaviors within the editor depending on our options. 
	  - Hidden = End Time is marked as Read Only in the Editor.
	  - Collapsed = End Time is not shown in the Editor.
	  - Visible = End Time is visible and editable in the Editor.*/
	virtual EVisibility ShowEndTime() const { return IsSingleFrame() ? EVisibility::Hidden : EVisibility::Visible; }

	/* Whether or not to allow the User to edit the Realm (Client/Server/Both) that a Task can execute in.*/
	virtual bool CanEditTaskRealm() const { return false; }

	/* UObject Override to fix up any Properties. */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	/* Adds a Task dependency for this Task.*/
	void AddDependency(const UAblAbilityTask* Task);

	/* Removes a Task dependency for this Task.*/
	void RemoveDependency(const UAblAbilityTask* Task);

	/* Clears all dependencies. */
	void ClearDependencies() { m_Dependencies.Empty(); }

	/* Fixes any Object flags, used mainly for classes that reference objects outside of their packages. */
	virtual bool FixUpObjectFlags();

#if WITH_EDITORONLY_DATA
	/* Returns the On Task Property Modified delegate. */
	FOnAblAbilityTaskPropertyModified& GetOnTaskPropertyModified() { return m_OnTaskPropertyModified; }

	/* Explicit property copy call. */
	void CopyProperties(UAblAbilityTask& source);

	/* Whether or not our Task has compact data, used to re-instantiate itself if BP reinstancing fails for whatever reason.*/
	virtual bool HasCompactData() const { return false; }

	/* Returns the compact data for this task. */
	void GetCompactData(FAblCompactTaskData& output);

	/* Reinstantiates properties using the provided compact data. */
	void LoadCompactData(FAblCompactTaskData& input);

	/* Sets our Task to be locked on the Task Timeline. */
	void SetLocked(bool locked) { m_Locked = locked; }

	/* Returns if our Task is locked or not. */
	bool IsLocked() const { return m_Locked; }
#endif

	/* Data Validation Tests */
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const {}
#endif
protected:
	/* Populates the OutActorArray with all Context Targets relevant to this Task. */
	void GetActorsForTask(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<TWeakObjectPtr<AActor>>& OutActorArray) const;
	
	/* Returns only a Single Actor from a given Context Target, if Target Actors is used - only the first actor in the list is selected. */
	AActor* GetSingleActorFromTargetType(const TWeakObjectPtr<const UAblAbilityContext>& Context, EAblAbilityTargetType TargetType) const;

	/* Returns true (or false) if this Task is allowed to execute on this Actor based on the Task realm and Actor authority. */
	bool IsTaskValidForActor(const AActor* Actor) const;

#if !(UE_BUILD_SHIPPING)
	void PrintVerbose(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FString& Output) const;
#endif

	/* When the Task starts. */
	UPROPERTY(EditInstanceOnly, Category = "Timing", meta=(DisplayName = "Start Time", ClampMin = 0.0))
	float m_StartTime;

	/* When the Task ends. */
	UPROPERTY(EditInstanceOnly, Category = "Timing", meta=(DisplayName = "End Time", ClampMin = 0.001))
	float m_EndTime;

    /* If true, this task will be inherited by child abilities. */
    UPROPERTY(EditInstanceOnly, Category = "Inheritance", meta = (DisplayName = "Inheritable"))
    bool m_Inheritable;

	/* Who you want this Task to affect.*/
	UPROPERTY(EditInstanceOnly, Category = "Targets", meta=(DisplayName = "Targets"))
	TArray<TEnumAsByte<EAblAbilityTargetType>> m_TaskTargets;

	/* Any Tasks that this Task requires to be completed before beginning. */
	UPROPERTY(EditInstanceOnly, Category = "Internal")
	TArray<const UAblAbilityTask*> m_Dependencies;

	/* If true, this task will print out various debug information as it executes. This is automatically disabled in shipping builds. */
	UPROPERTY(EditInstanceOnly, Category = "Debug", meta=(DisplayName = "Verbose"))
	bool m_Verbose;

    /* If true, This task will be ignored completely. Can be binded to check at runtime. */
    UPROPERTY(EditInstanceOnly, Category = "Debug", meta = (AblBindableProperty, DisplayName = "Disabled"))
    bool m_Disabled;

	UPROPERTY()
	FGetAblBool m_DisabledDelegate;

	/* What color to apply to the Task while in the Ability Editor. */
	UPROPERTY(EditInstanceOnly, Category = "Debug", meta = (DisplayName = "Override Task Color"))
	bool m_OverrideTaskColor;

	/* What color to apply to the Task while in the Ability Editor. */
	UPROPERTY(EditInstanceOnly, Category = "Debug", meta = (DisplayName = "Task Color", EditCondition=m_OverrideTaskColor))
	FLinearColor m_TaskColor;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta=(DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;

	/* If true, any time we start a new Iteration of our own Ability (due to looping for example), this Task will be stopped and restarted. */
	UPROPERTY(EditInstanceOnly, Category = "Looping", meta = (DisplayName = "Reset For Iteration"))
	bool m_ResetForIteration;

#if WITH_EDITORONLY_DATA
	/* Delegate for Task Properties being modified. */
	FOnAblAbilityTaskPropertyModified m_OnTaskPropertyModified;

	UPROPERTY()
	bool m_Locked;
#endif
};

#undef LOCTEXT_NAMESPACE