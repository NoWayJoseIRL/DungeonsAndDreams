// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityContext.h"
#include "Channeling/ablChannelingBase.h"
#include "GameplayTagContainer.h"
#include "Targeting/ablTargetingBase.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "ablAbility.generated.h"

#define LOCTEXT_NAMESPACE "AblAbility"

class UAblAbilityTask;
class UAnimationAsset;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EAblAbilityStartResult : uint8
{
	InvalidTarget UMETA(DisplayName = "Invalid Target"),
	FailedCustomCheck UMETA(DisplayName = "Failed Custom Check"),
	CooldownNotExpired UMETA(DisplayName = "Cooldown Not Expired"),
	CannotInterruptCurrentAbility UMETA(DisplayName = "Cannot Interrupt Current Ability"),
	NotAllowedAsPassive UMETA(DisplayName = "Not Allowed as Passive"),
	PassiveMaxStacksReached UMETA(DisplayName = "Passive Max Stacks Reached"),
	InternalSystemsError UMETA(DisplayName = "Internal Systems Error"),
	AsyncProcessing UMETA(DisplayName = "Async Processing"), // Used when an Async query has been queued up
	ForwardedToServer UMETA(DisplayName = "Forwarded to Server"),
	InvalidParameter UMETA(DisplayName = "Invalid Parameter"),
	Success UMETA(DisplayName = "Success")
};

UENUM()
enum class EAblAbilityPassiveBehavior : uint8
{
	CannotBePassive UMETA(DisplayName="Cannot be Passive"),
	RefreshDuration UMETA(DisplayName="Refresh Duration"),
	IncreaseStackCount UMETA(DisplayName="Increase Stack Count"),
	IncreaseAndRefresh UMETA(DisplayName="Increase Stack and Refresh Duration"),
};

UENUM(BlueprintType)
enum class EAblCallbackResult : uint8
{
    // The callback was handled successfully
    Complete UMETA(DisplayName = "Complete"),
    // The callback was ignored, so the task should keep processing if applicable
    KeepProcessing UMETA(DisplayName = "Keep Processing"),
    // The callback was ignored, so the task should keep processing if applicable
    IgnoreActors UMETA(DisplayName = "Ignore Actors"),
};

UENUM(BlueprintType)
enum class EAblInstancePolicy : uint8
{
	// Default behavior, use the immutable Class Default Object to save memory.
	Default UMETA(DisplayName = "Default"),
	// Allocate a new Instance of the Ability, this is writable. Client/Server get their own instances.
	NewInstance UMETA(DisplayName = "New Instance"),
	// Allocate a new Instance of the Ability, this is writable and replicated between client/server.
	NewInstanceReplicated UMETA(DisplayName = "New Instance(Replicated)"),
};

UENUM(BlueprintType)
enum class EAblClientExecutionPolicy : uint8
{
	// Default behavior, play this ability on both local (predicted), authoritative(server) and remote clients (from server).
	Default UMETA(DisplayName = "Default"),
	// Play this ability only on the Local client (predicted) and the server. Remote clients will ignore it.
	LocalAndAuthoritativeOnly UMETA(DisplayName = "Local And Authoritative"),
};

UCLASS(Abstract, Transient, Blueprintable)
class ABLECORE_API UAblAbilityScratchPad : public UObject
{
	GENERATED_BODY()
public:
	UAblAbilityScratchPad() { };
	virtual ~UAblAbilityScratchPad() { };
};

UCLASS(Blueprintable)
class ABLECORE_API UAblAbilityRuntimeParametersScratchPad : public UAblAbilityScratchPad
{
	GENERATED_BODY()
public:
	UAblAbilityRuntimeParametersScratchPad() { };
	virtual ~UAblAbilityRuntimeParametersScratchPad() { };

	/**
	* Set an Integer parameter on this Scratchpad using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	void SetIntParameter(FName Id, int Value);

	/**
	* Set an Float parameter on this Scratchpad using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	void SetFloatParameter(FName Id, float Value);

	/**
	* Set an String parameter on this Scratchpad using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	void SetStringParameter(FName Id, const FString& Value);

	/**
	* Set an UObject parameter on this Scratchpad using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	void SetUObjectParameter(FName Id, UObject* Value);

	/**
	* Returns an Integer parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or 0 if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	int GetIntParameter(FName Id) const;

	/**
	* Returns a Float parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or 0.0 if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	float GetFloatParameter(FName Id) const;

	/**
	* Returns a String parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or empty string if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	const FString& GetStringParameter(FName Id) const;

	/**
	* Returns a UObject parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or nullptr if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	UObject* GetUObjectParameter(FName Id) const;

	/**
	* Resets this Scratchpad to it's default state.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Scratchpad")
	void ResetScratchpad();

private:
	/* Various Scratchpad variables. Only allowed to be set */
	UPROPERTY(Transient)
	TMap<FName, int> m_IntParameters;

	UPROPERTY(Transient)
	TMap<FName, float> m_FloatParameters;

	UPROPERTY(Transient)
	TMap<FName, FString> m_StringParameters;

	UPROPERTY(Transient)
	TMap<FName, UObject*> m_UObjectParameters;

	/* ReadWrite Lock for Scratchpad Variables. */
	mutable FRWLock m_ScratchpadVariablesLock;
};

#if WITH_EDITORONLY_DATA
// Event for other objects that need to watch for changes in the Task.
DECLARE_MULTICAST_DELEGATE(FOnAblAbilityRequestEditorRefresh);
#endif

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSubclassOf<UAblAbility>, FGetAblAbility, const UAblAbilityContext*, Context, TSubclassOf<UAblAbility>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSubclassOf<AActor>, FGetAblActor, const UAblAbilityContext*, Context, TSubclassOf<AActor>, StaticValue);

UCLASS(Blueprintable, hidecategories=(Internal, Thumbnail), meta = (DisplayThumbnail = "true"))
class ABLECORE_API UAblAbility : public UObject
{
	GENERATED_BODY()

public:
	UAblAbility(const FObjectInitializer& ObjectInitializer);

	// UObject Overrides.
	void PostInitProperties() override;
	void PostLoad() override;
	virtual bool IsSupportedForNetworking() const override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual UWorld* GetWorld() const override;
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack) override;

	/* Called before the Ability is executed to allow any caching or other initialization. */
	void PreExecutionInit() const;

	/**
	* Check all prerequisites using the Ability Context to see if this Ability can be executed.
	* This will also check any targeting logic and populate the Context with that information.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The Ability Result, see EAblAbilityStartResult for all the options.
	*/
	virtual EAblAbilityStartResult CanAbilityExecute(UAblAbilityContext& Context) const;

	/**
	* Returns the name of the Ability in a print friendly format (without class identifiers).
	*
	* @return the name of the Ability
	*/
	UFUNCTION(BlueprintPure, Category="Able|Ability")
	FString GetDisplayName() const;

	/**
	* Returns the Channeling Conditions.
	*
	* @return Array of channeling conditions.
	*/
	FORCEINLINE const TArray<UAblChannelingBase*>& GetChannelConditions() const { return m_ChannelConditions; }
	
	/**
	* Returns the Ability Tasks, including inherited Tasks.
	*
	* @return Array of all Tasks for this Ability.
	*/
    const TArray<UAblAbilityTask*>& GetTasks() const;
	
	/**
	* Returns the Ability Name Hash.
	*
	* @return the Ability Name Hash.
	*/
	FORCEINLINE uint32 GetAbilityNameHash() const { return m_AbilityNameHash; }

	/**
	* Returns the realm this Ability belongs to.
	*
	* @return the realm this Ability belongs to.
	*/
	FORCEINLINE EAblAbilityTaskRealm GetAbilityRealm() const { return m_AbilityRealm.GetValue(); }

	/**
	* Returns the result used when Channeling Fails.
	*
	* @return the result used when Channeling Fails.
	*/
	FORCEINLINE EAblAbilityTaskResult GetChannelFailureResult() const { return m_FailedChannelResult.GetValue(); }

	/**
	* Returns any Task dependencies.
	*
	* @return Array of Task dependencies.
	*/
	FORCEINLINE const TArray<const UAblAbilityTask*>& GetAllTaskDependencies() const { return m_AllDependentTasks; }

	/**
	* Returns the Length, in seconds, of the Ability.
	*
	* @return the Length, in seconds, of the Ability.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE float GetLength() const { return m_Length; }

	/**
	* Returns the Base Play rate as set on the Ability.
	*
	* @return the Base Play rate, in seconds.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE float GetBasePlayRate() const { return m_PlayRate; }
	
	/**
	* Returns true if the Ability requires a Target.
	*
	* @return true if the Ability requires a Target.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE bool RequiresTarget() const { return m_RequiresTarget; }

	/**
	* Returns the estimated range of the Ability. Ignored unless "Requires Target" is set to true.
	*
	* @return the estimated range of the Ability.
	*/
    UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE float GetRange() const { return m_Targeting ? m_Targeting->GetRange() : 0.0f; }
	
	/**
	* Returns the calculated Cooldown for the Ability.
	*
	* @return the calculated Cooldown, in seconds, for the Ability.
	*/
	virtual float GetCooldown(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_Cooldown); }

	/* Returns the Base Cooldown, in seconds, of the Ability. */
	/**
	* Returns the Base Cooldown, in seconds, of the Ability.
	*
	* @return the Base Cooldown, in seconds, of the Ability.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE float GetBaseCooldown() const { return m_Cooldown; }

	/**
	* Returns whether or not our Cooldown can be cached.
	*
	* @return true if our Cooldown can be cached, and thus only calculated once.
	*/
	FORCEINLINE bool CanCacheCooldown() const { return m_CacheCooldown; }

	/**
	* Returns the Base Stacks value of the Ability.
	*
	* @return the Base Stacks value of the Ability.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE int32 GetBaseMaxStacks() const { return m_MaxStacks; }

	/**
	* Returns the Ability Tag Container.
	*
	* @return the Ability Tag Container.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE FGameplayTagContainer GetAbilityTagContainer() const { return m_TagContainer; }
    
	/**
	* Returns true if we have the supplied tag in our tag container.
	*
	* @param Tag The Gameplay Tag to check for.
	*
	* @return true if we have the supplied tag in our tag container.
	*/
    UFUNCTION(BlueprintPure, Category = "Able|Ability")
    bool HasTag(const FGameplayTag Tag) const;

	/**
	* Returns true if we have any tags from the passed in container in our own container.
	*
	* @param Container The Gameplay Tag Query to run against our container.
	*
	* @return true if we have any tags from the passed in container in our own container.
	*/
    UFUNCTION(BlueprintPure, Category = "Able|Ability")
    bool MatchesAnyTag(const FGameplayTagContainer Container) const;

	/**
	* Returns true if we have all the tags from the passed in container in our own container.
	*
	* @param Container The Gameplay Tag Query to run against our container.
	*
	* @return true if we have all the tags from the passed in container in our own container.
	*/
    UFUNCTION(BlueprintPure, Category = "Able|Ability")
    bool MatchesAllTags(const FGameplayTagContainer Container) const;
    
	/**
	* Returns true if the Ability passes a series of Gameplay Tags Queries.
	*
	* @param IncludesAny The tags to check if any of these tags are in our container.
	* @param IncludesAll The tags to check if all of these tags are in our container.
	* @param ExcludesAny The tags to check if any of these tags are not in our container.
	*
	* @return true if the our containers passed the series of Gameplay Tags queries.
	*/
    UFUNCTION(BlueprintPure, Category = "Able|Ability")
    bool CheckTags(const FGameplayTagContainer& IncludesAny, const FGameplayTagContainer& IncludesAll, const FGameplayTagContainer& ExcludesAny) const;

	/**
	* Returns true if the Ability is set to loop.
	*
	* @return true if the Ability is set to loop.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE bool IsLooping() const { return m_Loop; }

	/**
	* Returns the Max iterations allowed when looping this Ability.
	*
	* @return The max number of iterations to allow this Ability to make. Must be 0 or greater.
	*/
	virtual uint32 GetLoopMaxIterations(const UAblAbilityContext* Context) const { return (uint32)ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, (int32)m_LoopMaxIterations); }

	/**
	* Returns the Loop range, in seconds, of this Ability. X component is start time, Y component is end time.
	*
	* @return the Loop range, in seconds, of this Ability. X component is start time, Y component is end time.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FVector2D GetLoopRange() const { return FVector2D(m_LoopStart, m_LoopEnd); }

	/**
	* Returns true if this Ability is a passive Ability.
	*
	* @return true if this Ability is a passive Ability.
	*/
	UFUNCTION(BlueprintPure, Category="Able|Ability")
	FORCEINLINE bool IsPassive() const { return m_IsPassive; }


	/**
	* Returns true if this Ability is a passive Ability.
	*
	* @return true if this Ability is a passive Ability.
	*/
	UFUNCTION(BlueprintCallable, Category="Able|Ability")
	virtual bool GetIsPassive(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_IsPassive); }

	/**
	* Returns true if the duration should be reset any time a new stack is applied to the Ability.
	*
	* @return true if the duration should be reset any time a new stack is applied to the Ability.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE bool RefreshDurationOnNewStack() const { return m_RefreshDurationOnNewStack; }

	/**
	* Returns true if the duration is always refreshed, regardless if a new stack is added or fails. 
	*
	* @return true if the duration is always refreshed, regardless if a new stack is added or fails.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE bool AlwaysRefreshDuration() const { return m_AlwaysRefreshDuration; }

	/**
	* Returns true if, when refreshing duration, only the Loop range should be refreshed.
	*
	* @return true if, when refreshing duration, only the Loop range should be refreshed.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE bool RefreshLoopTimeOnly() const { return m_OnlyRefreshLoopTime; }

	/**
	* Returns true if, when refreshing duration, we should reset the loop count as well.
	*
	* @return true if, when refreshing duration, we should reset the loop count as well.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE bool ResetLoopCountOnRefresh() const { return m_ResetLoopCountOnRefresh; }

	/**
	* Returns true if we should decrement the stack count and restart the Ability as long as there is more than 0 stacks left.
	*
	* @return true if we should decrement the stack count and restart the Ability as long as there is more than 0 stacks left.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	FORCEINLINE bool GetDecrementAndRestartOnEnd() const { return m_DecrementAndRestartOnEnd; }

	/**
	* Returns if the Ability is channeled.
	*
	* @return true if channeled, false if not.
	*/
	UFUNCTION(BlueprintPure, Category="Able|Ability")
	FORCEINLINE bool IsChanneled() const { return m_IsChanneled; }

	/**
	* Returns the time, in seconds, a stack will decay automatically. 0.0 will prevent decay.
	*
	* @return the time, in seconds, a stack will decay automatically.
	*/
    UFUNCTION(BlueprintCallable, Category = "Able|Ability")
    virtual float GetDecayStackTime(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_DecayStackTime); }

	/**
	* Returns true if the Ability must pass all channel conditions to continue channeling.
	*
	* @return true if the Ability must pass all channel conditions to continue channeling.
	*/
	UFUNCTION(BlueprintPure, Category="Able|Ability")
	FORCEINLINE bool MustPassAllChannelConditions() const { return m_MustPassAllChannelConditions; }

	/**
	* Returns whether or not this Task must finish all tasks before being marked as done (e.g., it can ignore the end time if necessary).
	*
	* @return True if we need to must finish all tasks, false otherwise.
	*/
	UFUNCTION(BlueprintPure, Category="Able|Ability")
	FORCEINLINE bool MustFinishAllTasks() const { return m_FinishAllTasks; }

	/**
	* Called when this Ability is attempting to interrupt another ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param CurrentAbility The Ability we are attempting to interrupt.
	*
	* @return True if the Actor can be a Target, false if they should be removed.
	*/
	virtual bool CanInterruptAbility(const UAblAbilityContext* Context, const UAblAbility* CurrentAbility) const { return false; }

	/**
	* Called when this Ability is attempting to interrupt another ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param CurrentAbility The Ability we are attempting to interrupt.
	*
	* @return True if the Actor can be a Target, false if they should be removed.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Can Interrupt Ability", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	bool CanInterruptAbilityBP(const UAblAbilityContext* Context, const UAblAbility* CurrentAbility) const;
	
	/**
	* Called by the Custom Filter for each actor that is a candidate to be acted upon by this Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Filter Events you may have in your Timeline.
	* @param Actor The Actor we are testing as valid or not.
	*
	* @return True if the Actor can be a Target, false if they should be removed.
	*/
	virtual bool CustomFilterCondition(const UAblAbilityContext* Context, const FName& EventName, AActor* Actor) const { return true; }
	
	/**
	* Called by the Custom Filter for each actor that is a candidate to be acted upon by this Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Filter Events you may have in your Timeline.
	* @param Actor The Actor we are testing as valid or not.
	*
	* @return True if the Actor can be a Target, false if they should be removed.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Custom Filter Condition", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	bool CustomFilterConditionBP(const UAblAbilityContext* Context, const FName& EventName, AActor* Actor) const;

	/**
	* Called when determining the cooldown for this Ability.
	* If you have any special logic that increases/decreases the cooldown amount, you can do it here.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The amount of time, in seconds, to use for the cooldown. By default, returns GetBaseCooldown.
	*/
	virtual float CalculateCooldown(const UAblAbilityContext* Context) const;

	/**
	* Called when determining the cooldown for this Ability.
	* If you have any special logic that increases/decreases the cooldown amount, you can do it here.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The amount of time, in seconds, to use for the cooldown. By default, returns GetBaseCooldown.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Get Cooldown", meta = (DeprecatedFunction, DeprecationMessage = "This method is deprecated, please move any logic to the appropriate dynamic field method."))
	float CalculateCooldownBP(const UAblAbilityContext* Context) const;

	/**
	* Called when grabbing the Initial stack count for this Ability (if it's a passive).
	* If you have any special logic that increases/decreases the starting Stack amount, you can do it here.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The initial number of stacks to apply, must be greater than zero.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	virtual int32 GetInitialStacks(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_InitialStackCount); }

	/**
	* Called when increasing the Stack count for a Passive.
	* If you have any special logic that increases/decreases the amount, you can do it here.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The number of stacks to add, must be greater than zero.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	virtual int32 GetStackIncrement(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_StackIncrement); }

	/**
	* Called when decreasing the Stack count for a Passive.
	* If you have any special logic that increases/decreases the amount, you can do it here.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The number of stacks to add, must be greater than zero.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	virtual int32 GetStackDecrement(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_StackDecrement); }

	/**
	* Called when grabbing the Max stack count for this Ability (if it's a passive).
	* If you have any special logic that increases/decreases the Stack amount, you can do it here.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The maximum number of stacks to allow, must be greater than zero.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	virtual int32 GetMaxStacks(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_MaxStacks); }

	/**
	* Called when grabbing the Max stack count for this Ability (if it's a passive). 
	* If you have any special logic that increases/decreases the Stack amount, you can do it here.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The maximum number of stacks to allow, must be greater than zero.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Get Max Stacks", meta = (DeprecatedFunction, DeprecationMessage = "This method is deprecated, please move any logic to the appropriate dynamic field method."))
	int32 GetMaxStacksBP(const UAblAbilityContext* Context) const;

	/**
	* Called when checking if an Ability is valid to be started. You can use this event to execute
	* any custom logic beyond the normal targeting/channeling checks (e.g.; resources, time of day, etc).
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True if the ability should be allowed to start, false if not.
	*/
	virtual bool CustomCanAbilityExecute(const UAblAbilityContext* Context) const { return true; }

	/**
	* Called when checking if an Ability is valid to be started. You can use this event to execute
	* any custom logic beyond the normal targeting/channeling checks (e.g.; resources, time of day, etc).
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True if the ability should be allowed to start, false if not.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName="Can Ability Execute", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	bool CustomCanAbilityExecuteBP(const UAblAbilityContext* Context) const;

	/**
	* Called by the Custom Targeting of an Ability to apply whatever additional custom target logic you wish to do.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param FoundTargets The Array of Actors to used as Targets for this Ability.
	*
	* @return none
	*/
	virtual void CustomTargetingFindTargets(const UAblAbilityContext* Context, TArray<AActor*>& FoundTargets) const { };

	/**
	* Called by the Custom Targeting of an Ability to apply whatever additional custom target logic you wish to do.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param FoundTargets The Array of Actors to used as Targets for this Ability.
	*
	* @return none
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Able|Ability", DisplayName = "Find Targets", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	void CustomTargetingFindTargetsBP(const UAblAbilityContext* Context, TArray<AActor*>& FoundTargets) const;

	/**
	* Called by the Looping logic of an Ability to apply whatever additional check(s) you wish to do.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True if should allow another loop iteration, false if we should break the looping.
	*/
	virtual bool CustomCanLoopExecute(const UAblAbilityContext* Context) const { return true; }

	/**
	* Called by the Looping logic of an Ability to apply whatever additional check(s) you wish to do.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True if should allow another loop iteration, false if we should break the looping.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Can Loop Execute", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	bool CustomCanLoopExecuteBP(const UAblAbilityContext* Context) const;

	/**
	* Called by the Custom Channeling Condition to apply whatever channeling check you wish to do.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Channeling Events you may have in your Timeline.
	*
	* @return "Passed" if check succeeded, or "Failed" if it did not. Ignore is used for instances when the Server/Client can't test a condition (e.g., Input on the Server).
	*/
	virtual EAblConditionResults CheckCustomChannelConditional(const UAblAbilityContext* Context, const FName& EventName) const { return EAblConditionResults::ACR_Passed; }

	/**
	* Called by the Custom Channeling Condition to apply whatever channeling check you wish to do.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Channeling Events you may have in your Timeline.
	*
	* @return "Passed" if check succeeded, or "Failed" if it did not. Ignore is used for instances when the Server/Client can't test a condition (e.g., Input on the Server).
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Custom Channel Conditional", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	EAblConditionResults CheckCustomChannelConditionalBP(const UAblAbilityContext* Context, const FName& EventName) const;

	/**
	* Called by the Apply Damage Task to calculate how much damage to apply to an Actor.
	* NOTE: This method should always be thread safe, so only read data from actors - never write/change values of an actor.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Damage Events you may have in your Timeline.
	* @param BaseDamage The flat damage value provided by the Task properties.
	* @param Actor The Actor we are going to damage.
	*
	* @return The final damage value to apply to the Actor.
	*/
	virtual float CalculateDamageForActor(const UAblAbilityContext* Context, const FName& EventName, float BaseDamage, AActor* Actor, FDamageEvent damageEvent) const { return BaseDamage; }

	/**
	* Called by the Apply Damage Task to calculate how much damage to apply to an Actor.
	* NOTE: This method should always be thread safe, so only read data from actors - never write/change values of an actor.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Damage Events you may have in your Timeline.
	* @param BaseDamage The flat damage value provided by the Task properties.
	* @param Actor The Actor we are going to damage.
	*
	* @return The final damage value to apply to the Actor.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Calculate Damage For Actor", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	float CalculateDamageForActorBP(const UAblAbilityContext* Context, const FName& EventName, float BaseDamage, AActor* Actor, FDamageEvent damageEvent) const;

	/**
	* Called by the Branch Ability Task to verify we are allowed to branch to this new Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param BranchAbility The Ability we want to branch to.
	*
	* @return True if we should allow the branch, false otherwise.
	*/
	virtual bool CustomCanBranchTo(const UAblAbilityContext* Context, const UAblAbility* BranchAbility) const { return true; }

	/**
	* Called by the Branch Ability Task to verify we are allowed to branch to this new Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param BranchAbility The Ability we want to branch to.
	*
	* @return True if we should allow the branch, false otherwise.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Can Branch To", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	bool CustomCanBranchToBP(const UAblAbilityContext* Context, const UAblAbility* BranchAbility) const;

	/**
	* Callback for when the Ability successful starts, after passing all required prerequisites.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	virtual void OnAbilityStart(const UAblAbilityContext* Context) const { };

	/**
	* Callback for when the Ability successful starts, after passing all required prerequisites.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName="On Ability Start", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	void OnAbilityStartBP(const UAblAbilityContext* Context) const;

	/**
	* Callback for when the Ability successful ends. This is not called if the Ability is branched or interrupted.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	virtual void OnAbilityEnd(const UAblAbilityContext* Context) const { };

	/**
	* Callback for when the Ability successful ends. This is not called if the Ability is branched or interrupted.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Ability End", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	void OnAbilityEndBP(const UAblAbilityContext* Context) const;

	/**
	* Callback for when Branching to a new Ability
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	virtual void OnAbilityInterrupt(const UAblAbilityContext* Context) const { };

	/**
	* Callback for when this Ability is Interrupted.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Ability Interrupt", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	void OnAbilityInterruptBP(const UAblAbilityContext* Context) const;

	/**
	* Callback for when branching to a new Ability
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	virtual void OnAbilityBranch(const UAblAbilityContext* Context) const { };

	/**
	* Callback for when branching to a new Ability
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Ability Branch", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	void OnAbilityBranchBP(const UAblAbilityContext* Context) const;

	/**
	* Called by the Collision Query Task to allow any specific logic you wish to do with the Overlap results.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Collision Event you may have in your Timeline.
	* @param HitEntities Array of results from the Overlap Query
	*
	* @return The Callback Result, used by the Overlap Watcher Task to stop future overlaps or continue.
	*/
    virtual EAblCallbackResult OnCollisionEvent(const UAblAbilityContext* Context, const FName& EventName, const TArray<struct FAblQueryResult>& HitEntities) const;

	/**
	* Called by the Collision Query Task to allow any specific logic you wish to do with the Overlap results.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Collision Event you may have in your Timeline.
	* @param HitEntities Array of results from the Overlap Query
	*
	* @return The Callback Result, used by the Overlap Watcher Task to stop future overlaps or continue.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Collision Event", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
    EAblCallbackResult OnCollisionEventBP(const UAblAbilityContext* Context, const FName& EventName, const TArray<struct FAblQueryResult>& HitEntities) const;

	/**
	* Called by the Raycast Query Task to allow any specific logic you wish to do with the Raycast results.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Raycast Event you may have in your Timeline.
	* @param HitResults Array of results from the Raycast
	*
	* @return The Callback Result, this is currently ignored for Raycasts. Set it to "Complete" and ignore it for now.
	*/
    virtual EAblCallbackResult OnRaycastEvent(const UAblAbilityContext* Context, const FName& EventName, const TArray<FHitResult>& HitResults) const;

	/**
	* Called by the Raycast Query Task to allow any specific logic you wish to do with the Raycast results.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Raycast Event you may have in your Timeline.
	* @param HitResults Array of results from the Raycast
	*
	* @return The Callback Result, this is currently ignored for Raycasts. Set it to "Complete" and ignore it for now.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Raycast Event", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
    EAblCallbackResult OnRaycastEventBP(const UAblAbilityContext* Context, const FName& EventName, const TArray<FHitResult>& HitResults) const;

	/**
	* Called by the Custom Event Task; this callback can be used as a generic entry into your Ability to do whatever logic you wish.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Event you may have in your Timeline.
	*
	* @return none
	*/
	virtual void OnCustomEvent(const UAblAbilityContext* Context, const FName& EventName) const { };

	/**
	* Called by the Custom Event Task; this callback can be used as a generic entry into your Ability Blueprint to do whatever logic you wish.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Event you may have in your Timeline.
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Custom Event", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	void OnCustomEventBP(const UAblAbilityContext* Context, const FName& EventName) const;

	/**
	* Called by the Spawn Actor Task; this callback can be used to set any fields (location, velocity, etc) on the newly spawned Actor.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Spawn Actor Event you may have in your Timeline.
	* @param SpawnedActor The Actor spawned by the Task.
	* @param SpawnIndex The Spawn Index of this Actor, if spawning multiple actors (0 for 1st, 1 for 2nd, etc).
	*
	* @return none
	*/
	virtual void OnSpawnedActorEvent(const UAblAbilityContext* Context, const FName& EventName, AActor* SpawnedActor, int SpawnIndex) const { };

	/**
	* Called by the Spawn Actor Task; this callback can be used to set any fields (location, velocity, etc) on the newly spawned Actor.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Spawn Actor Event you may have in your Timeline.
	* @param SpawnedActor The Actor spawned by the Task.
	* @param SpawnIndex The Spawn Index of this Actor, if spawning multiple actors (0 for 1st, 1 for 2nd, etc).
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Spawned Actor", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	void OnSpawnedActorEventBP(const UAblAbilityContext* Context, const FName& EventName, AActor* SpawnedActor, int SpawnIndex) const;

	/**
	* Called when determining the Play Rate for this Ability. 
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The Play Rate to used (1.0 for default, 0.5 for 2x, 2.0 for 1/2 speed, etc).
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability", DisplayName = "Get Play Rate")
	virtual float GetPlayRate(const UAblAbilityContext* Context) const { return ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_PlayRate); }

	/**
	* Called by the Cancel Ability Task to do any custom logic to determine whether or not to cancel the passed in Ability on the Task Target.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param Ability The Ability we want to cancel.
	* @param EventName A simple identifier to distinguish this Event from other possible Cancel Ability Event you may have in your Timeline.
	*
	* @return True to cancel the Ability, false to ignore it.
	*/
	virtual bool ShouldCancelAbility(const UAblAbilityContext* Context, const UAblAbility* Ability, const FName& EventName) const { return false; }

	/**
	* Called by the Cancel Ability Task to do any custom logic to determine whether or not to cancel the passed in Ability on the Task Target.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param Ability The Ability we want to cancel.
	* @param EventName A simple identifier to distinguish this Event from other possible Cancel Ability Event you may have in your Timeline.
	*
	* @return True to cancel the Ability, false to ignore it.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Should Cancel Ability", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	bool ShouldCancelAbilityBP(const UAblAbilityContext* Context, const UAblAbility* Ability, const FName& EventName) const;

	/**
	* Callback for a Custom Event that can be used to block dependent Task execution until some provided criteria is met.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Condition Event you may have in your Timeline.
	*
	* @return Whether or not the condition is satisfied and should allow any dependent Tasks to execute.
	*/
	virtual bool CheckCustomConditionEvent(const UAblAbilityContext* Context, const FName& EventName) const { return true; }

	/**
	* Callback for a Custom Event that can be used to block dependent Task execution until some provided criteria is met.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param EventName A simple identifier to distinguish this Event from other possible Custom Condition Event you may have in your Timeline.
	*
	* @return Whether or not the condition is satisfied and should allow any dependent Tasks to execute.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Check Custom Condition", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	bool CheckCustomConditionEventBP(const UAblAbilityContext* Context, const FName& EventName) const;

	/**
	* Callback that occurs when the Location Target Type is used.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The location to assign as the Target.
	*/
	virtual FVector GetTargetLocation(const UAblAbilityContext* Context) const;

	/**
	* Callback that occurs when the Location Target Type is used. 
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The location to assign as the Target.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Get Target Location", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	FVector GetTargetLocationBP(const UAblAbilityContext* Context) const;

	/**
	* Callback that occurs when a stack is added from an executing Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
    virtual void OnAbilityStackAdded(const UAblAbilityContext* Context) const;

	/**
	* Callback that occurs when a stack is added from an executing Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
    UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Ability Stack Added")
    void OnAbilityStackAddedBP(const UAblAbilityContext* Context) const;

	/**
	* Callback that occurs when a stack is removed from an executing Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	virtual void OnAbilityStackRemoved(const UAblAbilityContext* Context) const;

	/**
	* Callback that occurs when a stack is removed from an executing Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
    UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "On Ability Stack Removed")
    void OnAbilityStackRemovedBP(const UAblAbilityContext* Context) const;

	/**
	* Returns the Targeting logic of this Ability, if any.
	*
	* @return The Targeting Logic instance.
	*/
    FORCEINLINE const UAblTargetingBase* GetTargeting() const { return m_Targeting; }

	/**
	* Returns the instancing policy of the Ability.
	*
	* @return The Instance Policy (Default, New Instance, Replicated)
	*/
	FORCEINLINE EAblInstancePolicy GetInstancePolicy() const { return m_InstancePolicy; }

	/**
	* Returns the client policy of the Ability.
	*
	* @return The Client Policy (Default, LocalOnly, AuthoritativeOnly, LocalAndAuthoritative, RemoteAndAuthoritative)
	*/
	FORCEINLINE EAblClientExecutionPolicy GetClientPolicy() const { return m_ClientPolicy; }

	/**
	* Returns the class, if any, to use as the Ability Scratchpad.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The class of the Ability Scratchpad (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad)
	*/
	virtual TSubclassOf<UAblAbilityScratchPad> GetAbilityScratchPadClass(const UAblAbilityContext* Context) const;
	
	/**
	* Returns the class, if any, to use as the Ability Scratchpad. 
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The class of the Ability Scratchpad (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad)
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Get Ability ScratchPad Class")
	TSubclassOf<UAblAbilityScratchPad> GetAbilityScratchPadClassBP(const UAblAbilityContext* Context) const;

	/**
	* This method is responsible for initializing the Ability Scratchpad back to it's default state (if allowing Scratchpads to be re-used).
	*
	* @param ScratchPad The Ability Scratchpad (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad)
	*
	* @return None
	*/
	virtual void ResetAbilityScratchPad(UAblAbilityScratchPad* ScratchPad) const;

	/**
	* This method is responsible for initializing the Ability Scratchpad back to it's default state (if allowing Scratchpads to be re-used).
	*
	* @param ScratchPad The Ability Scratchpad (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad)
	*
	* @return None
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Reset Ability ScratchPad")
	void ResetAbilityScratchPadBP(UAblAbilityScratchPad* ScratchPad) const;

	/**
	* Called when the Client requests an Ability Cancel, but the Server hasn't canceled that Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True to allow the cancel, false to ignore it.
	*/
	virtual bool CanClientCancelAbility(const UAblAbilityContext* Context) const { return true; }

	/**
	* Called when the Client requests an Ability Cancel, but the Server hasn't canceled that Ability. 
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True to allow the cancel, false to ignore it.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Can Client Cancel Ability")
	bool CanClientCancelAbilityBP(const UAblAbilityContext* Context) const;


	/**
	* Called by the Play Animation Task / Target Locations to figure out which SkeletalMesh to use for any socket or animation calls.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param Actor The Actor to pull the Skeletal Mesh Component from.
	* @param EventName The event name defined in the Play Animation Task / Target Location (used to identify it from other Tasks / Locations in the Ability). In the case of a Target Location, the event name is the socket name.
	*
	* @return the USkeletalMeshComponent to use, or null - which means Able will attempt to use the one found internally.
	*/
	virtual USkeletalMeshComponent* GetSkeletalMeshComponentForActor(const UAblAbilityContext* Context, AActor* Actor, const FName& EventName) const;

	/**
	* Called by the Play Animation Task / Target Locations to figure out which SkeletalMesh to use for any socket or animation calls.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param Actor The Actor to pull the Skeletal Mesh Component from.
	* @param EventName The event name defined in the Play Animation Task / Target Location (used to identify it from other Tasks / Locations in the Ability). In the case of a Target Location, the event name is the socket name. 
	*
	* @return the USkeletalMeshComponent to use, or null - which means Able will attempt to use the one found internally.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Ability", DisplayName = "Get Skeletal Mesh Component for Actor")
	USkeletalMeshComponent* GetSkeletalMeshComponentForActorBP(const UAblAbilityContext* Context, AActor* Actor, const FName& EventName) const;

#if WITH_EDITOR
	/* Adds a Task to the Ability. */
	void AddTask(UAblAbilityTask& Task);
	
	/* Removes a Task from the Ability. */
	void RemoveTask(UAblAbilityTask& Task);
	
	/* Removes a Task from the Ability. */
	void RemoveTask(const UAblAbilityTask& Task);
	
	/* Removes the Task at the provided index from the Ability. */
	void RemoveTaskAtIndex(int32 Index);

	/* Remove any null Tasks. */
	void SanitizeTasks();

	/* Sorts all Tasks. */
	void SortTasks();

	/* Make sure all dependencies are valid, remove stale dependencies. */
	void ValidateDependencies();

	/* This is only used in the case where we inherit from an Ability that already contains task. */
	void MarkTasksAsPublic(); 

	/* Sets the Length of the Ability. */
	FORCEINLINE void SetLength(float Length) { m_Length = Length; }

	// UObject override. 
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	/* Only returns our local Tasks. */
	TArray<UAblAbilityTask*>& GetMutableTasks() { return m_Tasks; }

    EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	/* Make copies of our Inherited Tasks. */
	void CopyInheritedTasks(const UAblAbility& ParentObject);

	void EditorRefreshBroadcast() 
	{
		if (RequestEditorRefresh.IsBound()) 
		{ 
			RequestEditorRefresh.Broadcast(); 
		}
	}

	/* Fix up any bad flags we may have, returns true if the object was modified and should be marked dirty. */
	bool FixUpObjectFlags(); 
#endif

#if WITH_EDITORONLY_DATA
	/* If the user has manually chosen a thumbnail image, use that. */
	UPROPERTY()
	class UTexture2D* ThumbnailImage;
	
	/** Information for thumbnail rendering */
	UPROPERTY(VisibleAnywhere, Instanced, Category = Thumbnail)
	class UThumbnailInfo* ThumbnailInfo;

	/* Event used to catch any of our Tasks modifying their properties. */
	void OnReferencedTaskPropertyModified(UAblAbilityTask& Task, struct FPropertyChangedEvent& PropertyChangedEvent);

	/* Delegate to request an Editor refresh.*/
	FOnAblAbilityRequestEditorRefresh RequestEditorRefresh;

	/* Check for any outdated / deprecated tasks. */
	bool HasInvalidTasks() const;

	/* Check if we expect compact data, but don't currently have any. */
	bool NeedsCompactData() const;

	/* Save all Compact Data. */
	void SaveCompactData(); 
	
	/* Load / Recreate Tasks through Compact Data. */
	void LoadCompactData();

	/* In Editor builds, it's possible for Arrays to be out of sync given how Blueprints copy the values (if the template has more entries than the CDO, the extra entries are left null). Check and fix that here. */
	void ValidateDataAgainstTemplate(const UAblAbility* Template);

	UPROPERTY()
	TArray<FAblCompactTaskData> CompactData;

	/* Draws Debug information in the Ability Editor to help visualize the Targeting shapes. */
	UPROPERTY(EditDefaultsOnly, Transient, Category = "Targeting", meta=(DisplayName="Draw Targeting Shapes (Editor)"))
	bool DrawTargetingShapes;

	UPROPERTY()
	bool IsCollapsed;
#endif

	/* Helper method to bind Dynamic Properties. */
	void BindDynamicProperties();

private:
	/* Helper method to build our Task dependency list. */
	void BuildDependencyList() const;

	/* For Ability Dynamic Delegates. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* The Length, in seconds, this ability takes to successfully execute. Tasks that end after this value (if any)
	 * will simply be ended early. This could cause unwanted behavior, so try to keep all Task times within the overall 
	 * time of the ability. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (DisplayName = "Length", ClampMin = 0.0))
	float m_Length;

	/* The time before this ability can be used again. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (DisplayName = "Cooldown", ClampMin = 0.0, AblBindableProperty, AblDefaultBinding = "OnCalculateCooldownBP"))
	float m_Cooldown;

	UPROPERTY()
	FGetAblFloat m_CooldownDelegate;

	/* If true, the cooldown will only be calculated during Ability start and then stored. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (DisplayName = "Cache Cooldown", ClampMin = 0.0))
	bool m_CacheCooldown;

	/* If true, the Ability will go on indefinitely until all tasks are completed. This is only required if you have a task of indeterminate length. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (DisplayName = "Finish All Tasks", ClampMin = 0.0))
	bool m_FinishAllTasks;

	/* The rate at which we play the Ability. 1.0 is normal, 2.0 is twice as fast, 0.5 is half as fast, etc. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (DisplayName = "Play Rate", ClampMin = 0.0, AblBindableProperty, AblDefaultBinding = "GetPlayRateBP"))
	float m_PlayRate;

	UPROPERTY()
	FGetAblFloat m_PlayRateDelegate;

	/* Whether this ability is eligible to be a passive or not. Passives cannot be executed as Actives and vice-versa.*/
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Is Passive", AblBindableProperty))
	bool m_IsPassive;

	UPROPERTY()
	FGetAblBool m_IsPassiveDelegate;

	/* The Initial number of stacks of this ability. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Initial Stacks", ClampMin = 1, EditCondition = m_IsPassive, AblBindableProperty))
	int32 m_InitialStackCount;

	UPROPERTY()
	FGetAblInt m_InitialStackCountDelegate;

	/* How much to increment the Stack count by each time the passive is reapplied and requests to increase stack. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Stack Increment", ClampMin = 1, EditCondition = m_IsPassive, AblBindableProperty))
	int32 m_StackIncrement;

	UPROPERTY()
	FGetAblInt m_StackIncrementDelegate;

	/* How much to decrement the Stack count by each time the passive is restarted/decayed. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Stack Decrement", ClampMin = 1, EditCondition = m_IsPassive, AblBindableProperty))
	int32 m_StackDecrement;

	UPROPERTY()
	FGetAblInt m_StackDecrementDelegate;

	/* The Maximum number of stacks of this ability. 0 = Infinite */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Max Stacks", ClampMin = 0, EditCondition=m_IsPassive, AblBindableProperty, AblDefaultBinding = "GetMaxStacksBP"))
	int32 m_MaxStacks;

	UPROPERTY()
	FGetAblInt m_MaxStacksDelegate;

	/* If true, we will only set the time for this ability if a new stack is successfully applied. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Refresh Duration On New Stack", EditCondition = m_IsPassive))
	bool m_RefreshDurationOnNewStack;

	/* If true, the time will be reset for this ability, regardless of if a new stack was applied - or we were already at Max stacks. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Always Refresh Duration", EditCondition = m_IsPassive))
	bool m_AlwaysRefreshDuration;

	/* If true, we will only reset the current loop time, instead of the ability time. You can use this if you have infinite looping abilities
	*  and you don't want certain tasks that happen before the loop to occur each time the passive is refreshed. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Only Refresh Loop Time", EditCondition = m_IsPassive))
	bool m_OnlyRefreshLoopTime; 

	/* If true, we will reset the current loop count when we refresh the Passives duration or loop time. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Reset Loop Count on Refresh", EditCondition = m_IsPassive))
	bool m_ResetLoopCountOnRefresh;

	/* If true, we will restart the Ability and decrement the stack count when the Ability ends. At 0 Stacks the Ability will not restart. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (DisplayName = "Decrement and Restart on End", EditCondition = m_IsPassive))
	bool m_DecrementAndRestartOnEnd;

	/* If true, and the loop passes the OnCustomCanLoopExecute event (if there is one), the ability will continue looping
	   the time segment specified until either the OnCustomCanLoopExecute returns false or the Max Iterations count is hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Loop", meta = (DisplayName = "Is Looping"))
	bool m_Loop;

	/* The start time of the Looping segment.*/
	UPROPERTY(EditDefaultsOnly, Category = "Loop", meta = (DisplayName = "Loop Start", ClampMin = 0.0, EditCondition=m_Loop))
	float m_LoopStart;

	/* The end time of the Looping segment. */
	UPROPERTY(EditDefaultsOnly, Category = "Loop", meta = (DisplayName = "Loop End", ClampMin = 0.0, EditCondition = m_Loop))
	float m_LoopEnd;

	/* The number of iterations to perform the loop. 0 = Infinite*/
	UPROPERTY(EditDefaultsOnly, Category = "Loop", meta = (DisplayName = "Max Iterations", ClampMin = 0, EditCondition = m_Loop, AblBindableProperty))
	int32 m_LoopMaxIterations;

	UPROPERTY()
	FGetAblInt m_LoopMaxIterationsDelegate;

	/* If true, this ability will immediately fail to run if it cannot find a target using the targeting logic. */
	UPROPERTY(EditDefaultsOnly, Category = "Targeting", meta = (DisplayName = "Requires Target"))
	bool m_RequiresTarget;

	/* The targeting logic to use when attempting to find a target. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Targeting", meta = (DisplayName = "Target Logic"))
	UAblTargetingBase* m_Targeting;

	/* Time value that allows for passive abilities to automatically decay. 0.0 = No decay */
    UPROPERTY(EditDefaultsOnly, Category = "Stack Decay", meta = (DisplayName = "Stack Decay Time", ClampMin = 0, EditCondition = "m_IsPassive", AblBindableProperty))
    float m_DecayStackTime;

    UPROPERTY()
    FGetAblFloat m_DecayStackTimeDelegate;

	/* Is this Ability a Channel Ability? Channeled Abilities have 1 or more Channel Conditionals that are checked each frame
	   if the condition passes, the Ability is allowed to continue execution. If it fails, the Ability is treated according to the provided Failed Channel Result.
	   Passive Abilities cannot be channeled.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Channeling", meta = (DisplayName = "Is Channeled"))
	bool m_IsChanneled;

	/* The conditions to check each frame to allow our channel to complete, or not. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Channeling", meta = (DisplayName = "Channel Conditions", EditCondition = "m_IsChanneled"))
	TArray<UAblChannelingBase*> m_ChannelConditions;

	/* If true, all conditions must succeed to continue channeling the Ability. */
	UPROPERTY(EditDefaultsOnly, Category = "Channeling", meta = (DisplayName = "Must Pass All Conditions", EditCondition = "m_IsChanneled"))
	bool m_MustPassAllChannelConditions;

	/* What result to pass to the Ability when the channel condition fails. */
	UPROPERTY(EditDefaultsOnly, Category = "Channeling", meta = (DisplayName="Failed Channel Result", EditCondition = "m_IsChanneled"))
	TEnumAsByte<EAblAbilityTaskResult> m_FailedChannelResult;

	/* Tag container for any Tags you wish to assign to this ability. */
	UPROPERTY(EditDefaultsOnly, Category = "Tags", meta = (DisplayName = "Tag Container"))
	FGameplayTagContainer m_TagContainer;

	/* Our Tasks */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Internal")
	TArray<UAblAbilityTask*> m_Tasks;

	/* The policy that handles how this Ability is instantiated.*/
	UPROPERTY(EditDefaultsOnly, Category = "Misc", meta = (DisplayName="Instance Policy"))
	EAblInstancePolicy m_InstancePolicy;

	/* The policy that handles how client execution (prediction or otherwise) is handled for this Ability. */
	UPROPERTY(EditDefaultsOnly, Category = "Misc", meta = (DisplayName="Client Policy"))
	EAblClientExecutionPolicy m_ClientPolicy;

	// Various run-time parameters.

	/* CRC Hash of our Ability Name. */
	UPROPERTY(Transient)
	uint32 m_AbilityNameHash;

	/* Realm of our Ability - calculated based on our Tasks, used in early-outs.*/
	UPROPERTY(Transient)
	TEnumAsByte<EAblAbilityTaskRealm> m_AbilityRealm;

	/* An Array of unique Tasks Dependencies. If one Task depends on another, the Task it depends on will be in this list. This is cached to save time during execution. */
	/* The reason these two fields are mutable is due to the Custom Task. We can't call BP events during PostLoad, which happens when a Custom Task has a dependency. So,
	   instead, we call PreExecutionInit to populate these fields. However, we don't want to break the const contract with the result of the systems, so these guys just get
	   to be special and use the dreaded mutable keyword so we can write to them, even while const. */
	UPROPERTY(Transient)
	mutable TArray<const UAblAbilityTask*> m_AllDependentTasks;

	/* Whether we need to update our dependencies or not. */
	UPROPERTY(Transient)
	mutable bool m_DependenciesDirty;
};

#undef LOCTEXT_NAMESPACE