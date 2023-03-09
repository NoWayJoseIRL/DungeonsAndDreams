// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbility.h"

#include "ablAbilityContext.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "ablAbilityContext.h"
#include "ablAbilityInstance.h"
#include "ablAbilityComponent.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

class UAblAbility;
class UAbleSettings;
class FAsyncAbilityCooldownUpdaterTask;
struct FAnimNode_AbilityAnimPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityStartBP, const UAblAbilityContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityEndBP, const UAblAbilityContext*, Context, EAblAbilityTaskResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityInterruptBP, const UAblAbilityContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityBranchedBP, const UAblAbilityContext*, Context);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAbilityStart, const UAblAbilityContext& /*Context*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAbilityEnd, const UAblAbilityContext& /*Context*/, EAblAbilityTaskResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAbilityInterrupt, const UAblAbilityContext& /*Context*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAbilityBranched, const UAblAbilityContext& /*Context*/);

/* Helper struct to keep track of Cooldowns. */
USTRUCT(BlueprintType)
struct FAblAbilityCooldown
{
	GENERATED_USTRUCT_BODY()
public:
	FAblAbilityCooldown();
	FAblAbilityCooldown(const UAblAbility& InAbility, const UAblAbilityContext& InContext);

	/* The Ability tied to this Cooldown. */
	const UAblAbility* GetAbility() const { return Ability; }

	/* Returns a value between 0 - 1.0 with how much time is left on the cooldown.*/
	float getTimeRatio() const { return FMath::Min(CurrentTime / CooldownTime, 1.0f);	}
	
	/* Returns if the Cooldown is complete or not. */
	bool IsComplete() const { return CurrentTime > CooldownTime; }
	
	/* Updates the Cooldown. */
	void Update(float DeltaTime);

	/* Returns the Calculated Cooldown. */
	float GetCooldownTime() const { return CooldownTime; }

	/* Sets the Cooldown. */
	void SetCooldownTime(float time) { CooldownTime = time; }
protected:
	/* The Ability for this Cooldown.*/
	UPROPERTY(BlueprintReadOnly, Category = "Cooldown")
	const UAblAbility* Ability;

	/* The Context for this Cooldown.*/
	UPROPERTY(BlueprintReadOnly, Category = "Cooldown")
	const UAblAbilityContext* Context;
	
	/* Current Time on the Cooldown.*/
	UPROPERTY(BlueprintReadOnly, Category = "Cooldown")
	float CurrentTime;

	/* Total Cooldown time. */
	UPROPERTY(BlueprintReadOnly, Category = "Cooldown")
	float CooldownTime;
};

USTRUCT()
struct FAblPendingCancelContext
{
	GENERATED_USTRUCT_BODY()
public:
	FAblPendingCancelContext() : AbilityNameHash(0U), ResultToUse(EAblAbilityTaskResult::Successful) {}
	FAblPendingCancelContext(uint32 InAbilityNameHash, EAblAbilityTaskResult InResultToUse)
		: AbilityNameHash(InAbilityNameHash),
		ResultToUse(InResultToUse)
	{

	}

	uint32 GetNameHash() const { return AbilityNameHash; }
	EAblAbilityTaskResult GetResult() const { return ResultToUse; }
	bool IsValid() const { return AbilityNameHash != 0U; }

private:
	uint32 AbilityNameHash;

	EAblAbilityTaskResult ResultToUse;
};

UCLASS(ClassGroup = Able, hidecategories = (Internal, Activation, Collision), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Ability Component", ShortToolTip = "A component for playing active and passive abilities."))
class ABLECORE_API UAblAbilityComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()
public:
	UAblAbilityComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UActorComponent Overrides
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginDestroy() override;
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;
	////

	/* Returns whether or not this is a networked component/game or not. */
	bool IsNetworked() const;

	/* Returns if this owner is locally controlled. */
	bool IsOwnerLocallyControlled() const;

	/* Returns if this component is authoritative (only applies to networked games). */
	bool IsAuthoritative() const;

	bool AbilityClientPolicyAllowsExecution(const UAblAbility* Ability, bool IsPrediction = false) const;

	/**
	* Attempts to Branch to the provided Ability Context.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return the result from the attempted branch.
	*/
	UFUNCTION(BlueprintCallable, Category="Able|Ability")
	EAblAbilityStartResult BranchAbility(UAblAbilityContext* Context);

	/**
	* Runs the Ability Context through the activate logic and returns the result, does not actually activate the Ability.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return the predicated start result for the given Context.
	*/
	UFUNCTION(BlueprintCallable, Category="Able|Ability")
	EAblAbilityStartResult CanActivateAbility(UAblAbilityContext* Context) const;
	
	/**
	* Activates the ability (does call into CanActivateAbility to make sure the call is valid).
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return the Ability Start Result.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	EAblAbilityStartResult ActivateAbility(UAblAbilityContext* Context);

	/**
	* Returns whether or not the component has an active Ability being played. 
	*
	* @return true if we are currently playing an Ability, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool IsPlayingAbility() const { return m_ActiveAbilityInstance.IsValid(); }

	/**
	* If you've set up your Animation Blueprint to use the AbilityAnimPlayer Graph Node,
	* this method will tell you if you have a Play Animation Task requesting you to use transition to that node.
	*
	* @return true if we have a queued up Ability Animation, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool HasAbilityAnimation() const;

	/**
	* Returns whether or not an Ability is marked as being on Cooldown.
	*
	* @param Ability The Ability to check for cooldown.
	*
	* @return true if that Ability still is on cooldown, false if not.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool IsAbilityOnCooldown(const UAblAbility* Ability) const;

	/**
	* Returns true if the passive Ability passed in is currently active, or not. 
	*
	* @param Ability The Ability to check for.
	*
	* @return true if that Ability still is running, false if not (or not marked as Passive).
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool IsPassiveActive(const UAblAbility* Ability) const;

	/* If you need the active Ability, use this method. The one below requires a const_cast because BPs can't take Const pointers. */
	const UAblAbility* GetConstActiveAbility() const;

	/**
	* Returns the current Active Ability (if there is one).
	*
	* @return The current Active Ability, or null if there isn't one.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	UAblAbility* GetActiveAbility() const;

    /* Executes a custom event on the currently active ability context. */
    void ExecuteCustomEventForActiveAbility(const UAblAbility* Ability, const FName& EventName);
    
	/**
	* Cancels the passed in Ability, if it's currently being played. Accepts both active and passive Abilities.
	*
	* @param Ability The Ability to check for.
	* @param ResultToUse Which result to apply to the Ability when we cancel it (Interrupted, Successful, Branched).
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	void CancelAbility(const UAblAbility* Ability, EAblAbilityTaskResult ResultToUse);

	/**
	* Returns the total number of active Passive Abilities on this component.
	*
	* @return the total number of active Passive Abilities running on this component.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	int32 GetTotalNumberOfPassives() const { return m_PassiveAbilityInstances.Num(); }

	/**
	* Returns the specific stack count for the passed in passive ability, or 0 if not found.
	*
	* @param Ability The Ability to check stacks for.
	*
	* @return the stack count for that Ability, or 0 if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	int32 GetCurrentStackCountForPassiveAbility(const UAblAbility* Ability) const;

	/**
	* Populates the passed in array with all the Passive Abilities currently active on this component.
	*
	* @param OutPassives The array to fill with any running Passive Abilities.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	void GetCurrentPassiveAbilities(TArray<UAblAbility*>& OutPassives) const;

	/**
	* Sets the Stack Count of the provided passive, if it exists. Optionally resets the duration of the passive. If the Stack Count is 0, the Ability is canceled with the provided result.
	*
	* @param Ability The Ability to set the stacks for.
	* @param NewStackCount What to set the Stack count to.
	* @param ResetDuration If we should reset the Ability duration with this new stack value.
	* @param ResultToUseOnCancel If we are setting the stack to 0, then use this result as the cancel result.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	void SetPassiveStackCount(const UAblAbility* Ability, int32 NewStackCount, bool ResetDuration, EAblAbilityTaskResult ResultToUseOnCancel);

    /**
	* Returns a value between 0.0 - 1.0 which represents how far along the cooldown is.
	* You can multiply this value by the Cooldown total to get the actual cooldown value in seconds.
	*
	* @param Ability The Ability to grab the Cooldown ratio for.
	*
	* @return The Cooldown as a ratio of 0.0 - 1.0.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	float GetAbilityCooldownRatio(const UAblAbility* Ability) const;

	/**
	* Returns the Cooldown that was calculated for this Ability when it was executed.
	*
	* @param Ability The Ability to grab the calculated Cooldown for.
	*
	* @return The calculated Cooldown value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	float GetAbilityCooldownTotal(const UAblAbility* Ability) const;

	/**
	* Removes the Cooldown for a given ability.
	*
	* @param Ability The Ability to remove the Cooldown for.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	void RemoveCooldown(const UAblAbility* Ability);

	/**
	* Sets the Cooldown for an Ability. If the Ability isn't currently on cooldown, it will add it. The context is only used when adding a new cooldown.
	*
	* @param Ability The Ability to set the Cooldown for.
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param Time The time, in seconds, to set this Cooldown for.
	*
	* @return The Cooldown as a ratio of 0.0 - 1.0.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	void SetCooldown(const UAblAbility* Ability, UAblAbilityContext* Context, float time);

	/**
	* Returns the current time of the Ability, if executing. You can also get this from the Context if you cached it.
	*
	* @param Ability The Ability to grab the current time from.
	*
	* @return the current time of the Ability, if executing.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	float GetAbilityCurrentTime(const UAblAbility* Ability) const;

	/**
	* Returns the current time ratio of the Ability, if executing. You can also get this from the Context if you cached it.
	*
	* @param Ability The Ability to grab the Time ratio for.
	*
	* @return The current Time ratio, between 0.0 and 1.0.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	float GetAbilityCurrentTimeRatio(const UAblAbility* Ability) const;

	/* Sets the Branch context which is instantiated next tick. */
	void QueueContext(UAblAbilityContext* Context, EAblAbilityTaskResult ResultToUse);

	/* Returns the Gameplay Tag Container. */
	const FGameplayTagContainer& GetGameplayTagContainer() const { return m_TagContainer; }
	
	/* Returns all Cooldowns, mutable. */
	TMap<uint32, FAblAbilityCooldown>& GetMutableCooldowns() { return m_ActiveCooldowns; }
	
	/* Returns all Cooldowns, non-mutable. */
	const TMap<uint32, FAblAbilityCooldown>& GetCooldowns() const {	return m_ActiveCooldowns; }

	/* Finds the appropriate ability and adds the provided array to the Context Target Actors. AllowDuplicates will prevent adding any duplicates to the Target array (or not), Clear Targets will clear the current targets before adding the new ones. */
	void AddAdditionTargetsToContext(const TWeakObjectPtr<const UAblAbilityContext>& Context, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool AllowDuplicates = false, bool ClearTargets = false);

	/* Finds the appropriate ability and modifies the context with the provided values. Not replication safe unless you are 100% sure the client and server will modify things in the same way. */
	void ModifyContext(const TWeakObjectPtr<const UAblAbilityContext>& Context, AActor* Instigator, AActor* Owner, const FVector& TargetLocation, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool ClearTargets = false);

#if WITH_EDITOR
	/* Plays the provided Ability. */
	void PlayAbilityFromEditor(const UAblAbility* Ability, AActor* ForceTarget = nullptr);

	/* Returns the current Ability play time. */
	float GetCurrentAbilityTime() const;

	/* Sets the current Ability play time. */
	void SetAbilityTime(float NewTime);
#endif
	/**
	* Adds a Gameplay Tag to our tag container.
	*
	* @param Tag The Gameplay Tag to add. 
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	void AddTag(const FGameplayTag Tag);

	/**
	* Removes a Gameplay Tag from our tag container.
	*
	* @param Tag The Gameplay Tag to remove.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	void RemoveTag(const FGameplayTag Tag);

	/**
	* Returns true if we have the supplied tag in our tag container.
	*
	* @param Tag The Gameplay Tag to check for.
	* @param includeExecutingAbilitys If true, run this check on all executing Abilities as well.
	*
	* @return true if we have the supplied tag in our tag container, or the executing Abilities.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool HasTag(const FGameplayTag Tag, bool includeExecutingAbilities = true) const;

	/**
	* Returns true if we have any tags from the passed in container in our own container.
	*
	* @param Container The Gameplay Tag container to compare our own container against.
	* @param includeExecutingAbilities If true, compare the container against our executing Ability containers as well.
	*
	* @return true if we have any tags from the passed in container in our own container, or the executing Abilities.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool MatchesAnyTag(const FGameplayTagContainer Container, bool includeExecutingAbilities = true) const;

	/**
	* Returns true if we have all the tags from the passed in container in our own container.
	*
	* @param Container The Gameplay Tag Container to compare our own container against.
	* @param includeExecutingAbilities If true, compare the container against our executing Ability containers as well.
	*
	* @return true if we have all the tags from the passed in container in our own container, or the executing Abilities.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool MatchesAllTags(const FGameplayTagContainer Container, bool includeExecutingAbilities = true) const;

	/**
	* Returns true if we match the supplied query with our GameplayTag container.
	*
	* @param Container The Gameplay Tag Query to run against our container.
	* @param includeExecutingAbilities If true, run the query against our executing Ability containers as well.
	*
	* @return true if we have have a match in our own container, or the executing Abilities.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	bool MatchesQuery(const FGameplayTagQuery Query, bool includeExecutingAbilities = true) const;

	/**
	* Returns true if the Ability checks pass a series of Gameplay Tags Queries.
	*
	* @param IncludesAny The tags to check if any of these tags are in our container, or the executing Abilities.
	* @param IncludesAll The tags to check if all of these tags are in our container, or the executing Abilities.
	* @param ExcludesAny The tags to check if any of these tags are not in our container, or the executing Abilities.
	* @param includeExecutingAbilities If true, run these checks against our executing Ability containers as well.
	*
	* @return true if the our container, or the executing Abilities, checks passed the series of Gameplay Tags queries.
	*/
    UFUNCTION(BlueprintCallable, Category = "Able|Ability")
    bool CheckTags(const FGameplayTagContainer& IncludesAny, const FGameplayTagContainer& IncludesAll, const FGameplayTagContainer& ExcludesAny, bool includeExecutingAbilities = true) const;

	/**
	* Returns a copy of the Gameplay Tag container.
	*
	* @return A copy of our Gameplay Tag container.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability", DisplayName = "GetGameplayTagContainer")
	FGameplayTagContainer GetGameplayTagContainerBP() const { return m_TagContainer; }
	
	// IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const { TagContainer = m_TagContainer; }

	// C++ Delegate for Ability Start.
	FOnAbilityStart& GetOnAbilityStart() { return m_AbilityStartDelegate; }

	// C++ Delegate for Ability End.
	FOnAbilityEnd& GetOnAbilityEnd() { return m_AbilityEndDelegate; }

	// C++ Delegate for Ability Interrupt.
	FOnAbilityInterrupt& GetOnAbilityInterrupt() { return m_AbilityInterruptDelegate; }

	// C++ Delegate for Ability Branch.
	FOnAbilityBranched& GetOnAbilityBranched() { return m_AbilityBranchedDelegate; }

	// Ability Animation Node Helper.
	void SetAbilityAnimationNode(const FAnimNode_AbilityAnimPlayer* Node);

	// Grabs the appropriate prediction key. 
	uint16 GetPredictionKey(); 

	void AddLocallyPredictedAbility(const FAblAbilityNetworkContext& Context);

	// Returns whether or not this Ability was locally predicted.
	bool WasLocallyPredicted(const FAblAbilityNetworkContext& Context);

	// Blueprint Assignable event that fires every time an Ability has been started. Parameter is the Ability Context that is starting.
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "OnAbilityStart"))
	FOnAbilityStartBP AbilityStartBPDelegate;

	// Blueprint Assignable event that fires every time an Ability has ended. Parameters are the Ability Context that has ended and the result of the Ability.
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "OnAbilityEnd"))
	FOnAbilityEndBP AbilityEndBPDelegate;

	// Blueprint Assignable event that fires every time an Ability has been interrupted. Parameter is the Ability Context that was interrupted.
	// Note there is no difference between this method and OnAbilityEnd with the Interrupted result. In fact, both are called. It's simply a convenience method.
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "OnAbilityInterrupt"))
	FOnAbilityInterruptBP AbilityInterruptBPDelegate;

	// Blueprint Assignable event that fires every time an Ability has been branched. Parameter is the Ability Context that was branched.
	// Note there is no difference between this method and OnAbilityEnd with the Branched result. In fact, both are called. It's simply a convenience method.
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "OnAbilityBranched"))
	FOnAbilityBranchedBP AbilityBranchBPDelegate;

protected:
	// Server Methods
	
	/**
	* INTERNAL - Attempts to activates the Ability using the provided Context.
	*
	* @param Context AbilityNetworkContext
	*
	* @return none
	*/
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerActivateAbility(const FAblAbilityNetworkContext& Context);
	
	/**
	* INTERNAL - Attempts to Branch the Ability using the provided Context.
	*
	* @param Context AbilityNetworkContext
	*
	* @return none
	*/
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBranchAbility(const FAblAbilityNetworkContext& Context);

	/**
	* INTERNAL - Attempts to Cancel the provided Ability with the result passed in.
	*
	* @param Context AbilityNetworkContext
	* @param ResultToUse The cancel result to use.
	*
	* @return none
	*/
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCancelAbility(uint32 AbilityNameHash, EAblAbilityTaskResult ResultToUse);

	/**
	* INTERNAL - Sent from the Server when delta compression wouldn't catch a change.
	*
	* @param Context AbilityNetworkContext
	*
	* @return none
	*/
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void ClientForceAbility(const FAblAbilityNetworkContext& Context);

	////

	// Client Replication Notifications

	/* Called when the Ability is changed on the Server. */
	UFUNCTION()
	void OnServerActiveAbilityChanged();
	
	/* Called when the number of Passives (or some aspect of the passive) is changed on the Server. */
	UFUNCTION()
	void OnServerPassiveAbilitiesChanged();

	UFUNCTION()
	void OnServerPredictiveKeyChanged();
	////

	/* Checks if this Component still needs to Tick each frame. */
	void CheckNeedsTick();

	/* Returns true if the Component is in the middle of an update. */
	bool IsProcessingUpdate() const { return m_IsProcessingUpdate; }

	/* Internal/Shared Logic between Server and Client for starting an Ability.  */
	EAblAbilityStartResult InternalStartAbility(UAblAbilityContext* Context, bool ServerActivatedAbility = false);
	
	/* Internal/Shared Logic between Server and Client for canceling an Ability. */
	void InternalCancelAbility(const UAblAbility* Ability, EAblAbilityTaskResult ResultToUse);

	/* Adds a Cooldown for the Provided Ability. */
	void AddCooldownForAbility(const UAblAbility& Ability, const UAblAbilityContext& Context);
	
	/* Internal/Shared Logic between Server and Client for updating all Abilities. */
	void InternalUpdateAbility(FAblAbilityInstance* AbilityInstance, float DeltaTime);

	/* Attempts to Activate the provided Context and returns the result, does not actually activate the Ability. */
	EAblAbilityStartResult CanActivatePassiveAbility(UAblAbilityContext* Context) const;

	/* Attempts to Activate the provided Context and returns the result. */
	EAblAbilityStartResult ActivatePassiveAbility(UAblAbilityContext* Context, bool ServerActivatedAbility = false);
	
	/* Cancels the current Active Ability using the provided result. */
	void CancelActiveAbility(EAblAbilityTaskResult ResultToUse);

	// Friend class to help call this method via Async task graph.
	friend class FAsyncAbilityCooldownUpdaterTask;

	/* Updates all Cooldowns using the provided DeltaTime. */
	void UpdateCooldowns(float DeltaTime);

	/* Helper method to deal with all the pending contexts we may have. */
	void HandlePendingContexts();

	/* Process any pending cancel requests we may have. */
	void HandlePendingCancels();

	/* Handle any Instance logic for this context. */
	void HandleInstanceLogic(UAblAbilityContext* Context);

	/* Handle any Instance clean up. */
	void HandleInstanceCleanUp(const UAblAbility& Ability);

	/* Check our running Abilities against our Server variables. Only executed on remote clients. */
	void ValidateRemoteRunningAbilities();
	////

	// Network Helper Methods

	/* Returns true if the provided Ability is allowed to run in this realm. */
	bool IsAbilityRealmAllowed(const UAblAbility& Ability) const;

	/* Updates the server representation of current passive Abilities (to be replicated to the client). */
	void UpdateServerPassiveAbilities();

	/* Updates the server representation of the current active Ability (to be replicated to the client). */
	void UpdateServerActiveAbility();
	////

    void GetCombinedGameplayTags(FGameplayTagContainer& CombinedTags, bool includeRunningAbilities) const;
protected:
	/* Our Active Ability Instance. */
	UPROPERTY(Transient)
	FAblAbilityInstance m_ActiveAbilityInstance;

	/* Our Active Ability Instance Result. */
	UPROPERTY(Transient)
	TEnumAsByte<EAblAbilityTaskResult> m_ActiveAbilityResult;

	/* Our Passive Ability Instances */
	UPROPERTY(Transient)
	TArray<FAblAbilityInstance> m_PassiveAbilityInstances;

    UPROPERTY(Transient)
    bool m_PassivesDirty = false;

	/* Cached Settings Object for log reporting. */
	UPROPERTY(Transient)
	TWeakObjectPtr<const UAbleSettings> m_Settings;
	
	/* Active Cooldowns. */
	UPROPERTY(Transient)
	TMap<uint32, FAblAbilityCooldown> m_ActiveCooldowns;

	/* Pending Context */
	UPROPERTY(Transient)
	TArray<UAblAbilityContext*> m_PendingContext;

	/* Pending Cancels */
	UPROPERTY(Transient)
	TArray<FAblPendingCancelContext> m_PendingCancels;

	/* How to treat the current Ability before switching to the new Context (success/normal, branch, interrupt). */
	UPROPERTY(Transient)
	TArray<TEnumAsByte<EAblAbilityTaskResult>> m_PendingResult;

	/* Contexts used in Async targeting, processed when they are completed. */
	UPROPERTY(Transient)
	TArray<UAblAbilityContext*> m_AsyncContexts; 

	/* Our Gameplay Tag Container */
	UPROPERTY(Transient)
	FGameplayTagContainer m_TagContainer;

	/* These tags are automatically added to the Ability Component Tag container when the game starts.*/
	UPROPERTY(EditDefaultsOnly, Category = "Able|Tags", meta = (DisplayName = "Auto Apply Tags"))
	FGameplayTagContainer m_AutoApplyTags;

	/* Boolean to track our update processing. */
	UPROPERTY(Transient)
	bool m_IsProcessingUpdate;

	UPROPERTY(Transient)
	TArray<UAblAbility*> m_CreatedAbilityInstances;

	static const uint16 ABLE_ABILITY_PREDICTION_RING_SIZE = 16;

	UPROPERTY(Transient)
	TArray<FAblAbilityNetworkContext> m_LocallyPredictedAbilities;

	uint16 m_ClientPredictionKey;

	// C++ delegates

	FOnAbilityStart m_AbilityStartDelegate;

	FOnAbilityEnd m_AbilityEndDelegate;

	FOnAbilityInterrupt m_AbilityInterruptDelegate;

	FOnAbilityBranched m_AbilityBranchedDelegate;

	const FAnimNode_AbilityAnimPlayer* m_AbilityAnimationNode;

	FCriticalSection m_AbilityAnimNodeCS;

	// Network Fields
	// The Active Ability being played on the server.
	UPROPERTY(Transient, ReplicatedUsing = OnServerActiveAbilityChanged)
	FAblAbilityNetworkContext m_ServerActive;

	// The Active Passive Abilities being played on the server.
	UPROPERTY(Transient, ReplicatedUsing = OnServerPassiveAbilitiesChanged)
	TArray<FAblAbilityNetworkContext> m_ServerPassiveAbilities;

	UPROPERTY(Transient, ReplicatedUsing = OnServerPredictiveKeyChanged)
	uint16 m_ServerPredictionKey;
	///// 

};

#undef LOCTEXT_NAMESPACE
