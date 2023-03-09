// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityTypes.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "WorldCollision.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "ablAbilityContext.generated.h"

class UAblAbility;
class UAblAbilityComponent;
class UAblAbilityUtilitySubsystem;
class UAblAbilityScratchPad;
class UAbleSettings;

#define LOCTEXT_NAMESPACE "AblAbilityContext"

/* Struct used in Collision Queries. */
USTRUCT(BlueprintType)
struct ABLECORE_API FAblQueryResult
{
	GENERATED_USTRUCT_BODY();
public:
	FAblQueryResult() {};
	FAblQueryResult(const FOverlapResult& OverlapResult)
		: PrimitiveComponent(OverlapResult.GetComponent()),
		Actor(OverlapResult.GetActor())
	{

	}
	FAblQueryResult(const FHitResult& HitResult)
		:PrimitiveComponent(HitResult.GetComponent()),
		Actor(HitResult.GetActor())
	{

	}

	FAblQueryResult(UPrimitiveComponent* InPrimitiveComponent, AActor* InActor)
		: PrimitiveComponent(InPrimitiveComponent),
		Actor(InActor)
	{

	}

	bool operator==(const FAblQueryResult& RHS) const
	{
		return PrimitiveComponent == RHS.PrimitiveComponent && Actor == RHS.Actor;
	}

	/* Returns the Location of the Result. */
	FVector GetLocation() const;

	/* Returns the Transform of the Result. */
	void GetTransform(FTransform& OutTransform) const;

	/* Primitive Component from our Query result.*/
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Able|QueryResult", meta = (DisplayName = "Primitive Component"))
	TWeakObjectPtr<UPrimitiveComponent> PrimitiveComponent;

	/* Actor from our Query Result. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Able|QueryResult", meta = (DisplayName = "Actor"))
	TWeakObjectPtr<AActor> Actor;
};

class UAblAbilityContext;

/* Slightly more compact version of our normal Ability Context, for transfer across the wire. */
USTRUCT()
struct ABLECORE_API FAblAbilityNetworkContext
{
	GENERATED_USTRUCT_BODY();
public:
	FAblAbilityNetworkContext();
	FAblAbilityNetworkContext(const UAblAbilityContext& Context);
	FAblAbilityNetworkContext(const UAblAbilityContext& Context, EAblAbilityTaskResult Result);

	/* Resets this struct.*/
	void Reset();

	/* Returns true if this struct has valid properties. */
	bool IsValid() const;

	/* Sets the Stacks of this Context. */
	void SetCurrentStacks(int8 CurrentStacks) { m_CurrentStacks = CurrentStacks; }

	/* Returns the Ability contained by this Context.*/
	const TWeakObjectPtr<const UAblAbility>& GetAbility() const { return m_Ability; }
	
	/* Returns the Ability Component contained by this Context. */
	const TWeakObjectPtr<UAblAbilityComponent>& GetAbilityComponent() const { return m_AbilityComponent; }
	
	/* Returns the Owner contained by the Context. */
	const TWeakObjectPtr<AActor>& GetOwner() const { return m_Owner; }
	
	/* Returns the Instigator contained by the Context. */
	const TWeakObjectPtr<AActor>& GetInstigator() const { return m_Instigator; }

	/* Returns all Target actors contained by the Context. */
	const TArray<TWeakObjectPtr<AActor>>& GetTargetActors() const { return m_TargetActors; }

	/* Returns the Timestamp on when this Ability Context was created.*/
	float GetTimeStamp() const { return m_TimeStamp; }

	/* Returns the Current stack count of this Context. */
	int8 GetCurrentStack() const { return m_CurrentStacks; }

	/* Returns the Result of the Ability (if coming from the server). */
	EAblAbilityTaskResult GetResult() const { return m_Result.GetValue(); }

	/* Returns a Target location in the scene. */
	const FVector& GetTargetLocation() const { return m_TargetLocation; }

	/* Returns the Prediction Key for this Context. */
	uint16 GetPredictionKey() const { return m_PredictionKey; }

	/* Parameter Accessors. */
	const TMap<FName, int>& GetIntParameters() const { return m_IntParameters; }
	const TMap<FName, float>& GetFloatParameters() const { return m_FloatParameters; }
	const TMap<FName, FString>& GetStringParameters() const { return m_StringParameters; }
	const TMap<FName, UObject*>& GetUObjectParameters() const { return m_UObjectParameters; }
	const TMap<FName, FVector>& GetVectorParameters() const { return m_VectorParameters; }

	static FAblAbilityNetworkContext UpdateNetworkContext(const UAblAbilityContext& UpdatedContext, const FAblAbilityNetworkContext& Source);
private:
	/* The Ability for this Context. */
	UPROPERTY()
	TWeakObjectPtr<const UAblAbility> m_Ability;

	/* The Ability Component for this Context. */
	UPROPERTY()
	TWeakObjectPtr<UAblAbilityComponent> m_AbilityComponent;

	/* The Owner Actor of this Context. */
	UPROPERTY()
	TWeakObjectPtr<AActor> m_Owner;

	/* The Instigator Actor of this Context. */
	UPROPERTY()
	TWeakObjectPtr<AActor> m_Instigator;

	/* The Target Actors of this Context. */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> m_TargetActors;

	/* The Current Ability Stacks of this Context. */
	UPROPERTY()
	int8 m_CurrentStacks;

	/* The Timestamp of this context. */
	UPROPERTY()
	float m_TimeStamp;

	UPROPERTY()
	TEnumAsByte<EAblAbilityTaskResult> m_Result;

	UPROPERTY()
	FVector m_TargetLocation;

	UPROPERTY()
	uint16 m_PredictionKey;

	UPROPERTY(Transient, NotReplicated)
	TMap<FName, int> m_IntParameters;

	UPROPERTY(Transient, NotReplicated)
	TMap<FName, float> m_FloatParameters;

	UPROPERTY(Transient, NotReplicated)
	TMap<FName, FString> m_StringParameters;

	UPROPERTY(Transient, NotReplicated)
	TMap<FName, UObject*> m_UObjectParameters;

	UPROPERTY(Transient, NotReplicated)
	TMap<FName, FVector> m_VectorParameters;
};

class AbleRWScopeLock
{
public:
	AbleRWScopeLock(FRWLock& lock, bool write = false): m_write(write), m_lock(&lock)
	{
		if (m_write)
		{
			m_lock->WriteLock();
		}
		else
		{
			m_lock->ReadLock();
		}
	}
	~AbleRWScopeLock()
	{
		if (m_write)
		{
			m_lock->WriteUnlock();
		}
		else
		{
			m_lock->ReadUnlock();
		}
	}
private:
	bool m_write;
	FRWLock* m_lock;
};

#define ABLE_RWLOCK_SCOPE_READ(x) AbleRWScopeLock(x, false);
#define ABLE_RWLOCK_SCOPE_WRITE(x) AbleRWScopeLock(x, true);

/* Ability Context, contains all the information needed during an Ability's execution. */
UCLASS(BlueprintType)
class ABLECORE_API UAblAbilityContext : public UObject
{
	GENERATED_BODY()
public:
	UAblAbilityContext(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityContext();

	/* Uses the provided arguments to create an Ability Context. */
	static UAblAbilityContext* MakeContext(const UAblAbility* Ability, UAblAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator);

	/* Uses the provided arguments to create an Ability Context. */
	static UAblAbilityContext* MakeContext(const UAblAbility* Ability, UAblAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator, FVector Location);
	
	/* Creates an Ability Context from a Networked Ability Context. */
	static UAblAbilityContext* MakeContext(const FAblAbilityNetworkContext& NetworkContext);

	/* Allocates all required Ability / Task Scratchpads. */
	void AllocateScratchPads();

	/* Releases Ability / Task Scratchpads. */
	void ReleaseScratchPads();

	/* Updates the time of this Context. */
	void UpdateTime(float DeltaTime);

	/* Returns the Scratchpad for the provided Task (if it has one). */
	class UAblAbilityTaskScratchPad* GetScratchPadForTask(const class UAblAbilityTask* Task) const;

	/* Returns Target Actor array, mutable. */
	TArray<TWeakObjectPtr<AActor>>& GetMutableTargetActors() { return m_TargetActors; }

	/* Sets the Instigator. Note: This isn't replicated. Use the normal MakeContext flow if you want replication. */
	void SetInstigator(AActor* Instigator) { m_Instigator = Instigator; }

	/* Sets the Owner. Note: This isn't replicated. Use the normal MakeContext flow if you want replication. */
	void SetOwner(AActor* Owner) { m_Owner = Owner; }

	/**
	* Returns the Ability Component executing this Context.
	*
	* @return the Ability Component executing this Context.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	UAblAbilityComponent* GetSelfAbilityComponent() const;

	/**
	* Returns the Actor (Ability Component Owner) executing this Context.
	*
	* @return the Actor (Ability Component Owner) executing this Context.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	AActor* GetSelfActor() const;

	/**
	* Returns the Owner Context Target Actor, if it exists. This is set when creating the context.
	*
	* @return the Owner Context Target Actor.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	AActor* GetOwner() const { return m_Owner.Get(); }

	/**
	* Returns the Instigator Context Target Actor, if it exists. This is set when creating the context.
	*
	* @return the Instigator Context Target Actor.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	AActor* GetInstigator() const { return m_Instigator.Get(); }

	/**
	* Returns an array of all Target Context Target Actors, if they exist.
	*
	* @return Array of all Target Context Target Actors
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	const TArray<AActor*> GetTargetActors() const;

	/**
	* Clear all entries from the Target Actors.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void ClearTargetActors() { m_TargetActors.Empty(); }

	/**
	* Returns the Stack count of this Ability.
	*
	* @return the Stack count of this Ability.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	int32 GetCurrentStackCount() const;

	/**
	* Returns the Loop iteration of this Ability, starting at 0.
	*
	* @return the Loop iteration of this Ability, starting at 0.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	int32 GetCurrentLoopIteration() const;

	/**
	* Sets the Stack Count of this Ability.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void SetStackCount(int32 Stack);

	/**
	* Sets the Loop Iteration of this Ability. Does not reset the current loop.
	*
	* @return none
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void SetLoopIteration(int32 Loop);

	/**
	* Returns the current time of this Ability.
	*
	* @return the current time of this Ability.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	float GetCurrentTime() const { return m_CurrentTime; }

	/**
	* Returns the current time of this Ability as a ratio between 0.0 and 1.0.
	*
	* @return the current time of this Ability as a ratio between 0.0 and 1.0.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	float GetCurrentTimeRatio() const;

	/* Returns the last Delta time used to update this Ability. */
	/**
	* Returns the last Delta time (time between frames) used to update this Ability.
	*
	* @return the last Delta time (time between frames) used to update this Ability.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	float GetLastDeltaTime() const { return m_LastDelta; }

	/**
	* Returns the start location of this Ability.
	*
	* @return the start location of this Ability.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	FVector GetAbilityStartLocation() const { return m_AbilityActorStartLocation; }

	/**
	* Returns the Ability Scratchpad allocated for this context, if requested.
	*
	* @return the Ability Scratchpad (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad)
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	UAblAbilityScratchPad* GetAbilityScratchPad() const { return m_AbilityScratchPad; }

	/**
	* Set an Integer parameter on this Context using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void SetIntParameter(FName Id, int Value);

	/**
	* Set an Float parameter on this Context using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void SetFloatParameter(FName Id, float Value);
	
	/**
	* Set an String parameter on this Context using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void SetStringParameter(FName Id, const FString& Value);

	/**
	* Set an UObject parameter on this Context using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void SetUObjectParameter(FName Id, UObject* Value);

	/**
	* Set an Vector parameter on this Context using an FName Identifier. Parameters are not replicated across client/server.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	void SetVectorParameter(FName Id, FVector Value);

	/**
	* Returns an Integer parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or 0 if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	int GetIntParameter(FName Id) const;

	/**
	* Returns a Float parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or 0.0 if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	float GetFloatParameter(FName Id) const;

	/**
	* Returns a String parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or empty string if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	const FString& GetStringParameter(FName Id) const;

	/**
	* Returns a UObject parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or nullptr if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	UObject* GetUObjectParameter(FName Id) const;

	/**
	* Returns a Vector parameter assigned to the provided FName Identifier.
	*
	* @return the parameter value, or nullptr if not found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	FVector GetVectorParameter(FName Id) const;

	/**
	* Returns the Ability contained in this Context.
	*
	* @return the Ability.
	*/
	const UAblAbility* GetAbility() const { return m_Ability; }

	/**
	* Returns the Ability contained in this Context.
	*
	* @return the Ability.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context", DisplayName = "GetAbility")
	UAblAbility* GetAbilityBP() const { return const_cast<UAblAbility*>(m_Ability); }

	/* Returns Target Actors Array directly (blueprint version requires a copy). */
	const TArray<TWeakObjectPtr<AActor>>& GetTargetActorsWeakPtr() const { return m_TargetActors; }

	/* Returns true if the Ability has found any targets. */
	bool HasAnyTargets() const { return m_TargetActors.Num() > 0 || m_TargetLocation.SizeSquared() > KINDA_SMALL_NUMBER; }

	/* Sets the Current Time of this Ability. */
	void SetCurrentTime(float Time) { m_CurrentTime = Time; }

	// Async Targeting Support
	/* Returns true if the context contains an Async handle for targeting. */
	bool HasValidAsyncHandle() const { return m_AsyncHandle._Handle != 0; }
	
	/* Sets the Async Handle.*/
	void SetAsyncHandle(FTraceHandle& InHandle) { m_AsyncHandle = InHandle; }
	
	/* Returns the Async Handle.*/
	const FTraceHandle& GetAsyncHandle() const { return m_AsyncHandle; }

	/* Sets the Async Targeting Transform. */
	void SetAsyncQueryTransform(const FTransform& Transform) { m_AsyncQueryTransform = Transform; }
	
	/* Returns the Async Targeting Transform. */
	const FTransform& GetAsyncQueryTransform() const { return m_AsyncQueryTransform; }
	//////

    /* Set the Origin Location. */
    void SetAbilityActorStartLocation(const FVector& Location) { m_AbilityActorStartLocation = Location; }

	/**
	* Sets the Target Location contained in this Context.
	* Note: This is only sent across the wire if this value is set BEFORE calling ActivateAbility and passing in this context.
	* 
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context", DisplayName = "SetTargetLocation")
	void SetTargetLocation(const FVector& Location) { m_TargetLocation = Location; }
	
	/**
	* Returns the Target Location contained in this Context.
	*
	* @return the Target Location.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context", DisplayName = "GetTargetLocation")
	const FVector& GetTargetLocation() const { return m_TargetLocation; }

	/* Sets the Ability on the Context. Be very careful when using this. */
	void SetAbility(const UAblAbility* Ability) { m_Ability = Ability; }

	/* Returns the Prediction Key */
	uint16 GetPredictionKey() const { return m_PredictionKey; }

	/* Returns the Able Utility Subsystem. */
	UAblAbilityUtilitySubsystem* GetUtilitySubsystem() const;

	/* Parameter Accessors. */
	const TMap<FName, int>& GetIntParameters() const { return m_IntParameters; }
	const TMap<FName, float>& GetFloatParameters() const { return m_FloatParameters; }
	const TMap<FName, FString>& GetStringParameters() const { return m_StringParameters; }
	const TMap<FName, UObject*>& GetUObjectParameters() const { return m_UObjectParameters; }
	const TMap<FName, FVector>& GetVectorParameters() const { return m_VectorParameters; }

	TMap<FName, int>& GetMutableIntParameters() { return m_IntParameters; }
	TMap<FName, float>& GetMutableFloatParameters() { return m_FloatParameters; }
	TMap<FName, FString>& GetMutableStringParameters() { return m_StringParameters; }
	TMap<FName, UObject*>& GetMutableUObjectParameters() { return m_UObjectParameters; }
	TMap<FName, FVector>& GetMutableVectorParameters() { return m_VectorParameters; }
	/* Resets the Context to it's default state, and returns it to the pool if pooling is enabled.*/
	void Reset();
protected:

    /* A Target Location. */
    UPROPERTY(Transient)
    FVector m_AbilityActorStartLocation;

	/* The Ability being executed. */
	UPROPERTY(Transient)
	const UAblAbility* m_Ability;

	/* The Ability Component running this Ability. */
	UPROPERTY(Transient)
	UAblAbilityComponent* m_AbilityComponent;

	/* The Stack Count. */
	UPROPERTY(Transient)
	int32 m_StackCount;

	/* The Loop Count. */
	UPROPERTY(Transient)
	int32 m_LoopIteration;

	/* The Current Time of the Ability. */
	UPROPERTY(Transient)
	float m_CurrentTime;

	/* The Last delta value used to update the Ability. */
	UPROPERTY(Transient)
	float m_LastDelta;

	/* The "Owner" of this ability (may or may not be the same as the AbilityComponent owner).*/
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> m_Owner; 
	
	/* The Actor that caused this Ability to occur (if there is one).*/
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> m_Instigator;

	/* The Actor Targets of the Ability. */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> m_TargetActors;

	/* Map of Task Unique IDs to ScratchPads. */
	UPROPERTY(Transient)
	TMap<uint32, class UAblAbilityTaskScratchPad*> m_TaskScratchPadMap;

	/* The Ability ScratchPad, if allocated. */
	UPROPERTY(Transient)
	UAblAbilityScratchPad* m_AbilityScratchPad;

	/* Used if our targeting call uses Async instead of Sync queries. */
	FTraceHandle m_AsyncHandle;

	/* Cached Transform used for our Async transform in case we need to do extra processing once our results come in. Currently only Cone check uses this. */
	UPROPERTY(Transient)
	FTransform m_AsyncQueryTransform;

	/* A Target Location. */
	UPROPERTY(Transient)
	FVector m_TargetLocation;

	/* Prediction Key */
	UPROPERTY(Transient)
	uint16 m_PredictionKey;

	/* Various context variables. Only allowed to be set */
	UPROPERTY(Transient)
	TMap<FName, int> m_IntParameters;

	UPROPERTY(Transient)
	TMap<FName, float> m_FloatParameters;

	UPROPERTY(Transient)
	TMap<FName, FString> m_StringParameters;

	UPROPERTY(Transient)
	TMap<FName, UObject*> m_UObjectParameters;

	UPROPERTY(Transient)
	TMap<FName, FVector> m_VectorParameters;

	/* ReadWrite Lock for Context Variables. */
	mutable FRWLock m_ContextVariablesLock;
};

#undef LOCTEXT_NAMESPACE