// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityInstance.h"
#include "ablAbilityComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/EngineTypes.h"

class FAblAbilityUtilities
{
public:
	static const TArray<struct FKey> GetKeysForInputAction(const FName& InputAction);
	static UBlackboardComponent* GetBlackboard(AActor* Target);
};

// Various one off helper classes.
// These are mostly Predicates, or Async Tasks structs.
class FAsyncAbilityInstanceUpdaterTask
{
public:
	// We make a copy of the Current time since it is written to during the sync update.
	FAsyncAbilityInstanceUpdaterTask(FAblAbilityInstance* InAbilityInstance, float InCurrentTime, float InDeltaTime)
		: m_AbilityInstance(InAbilityInstance),
		m_CurrentTime(InCurrentTime),
		m_DeltaTime(InDeltaTime)
	{ }

	/* Run the Async Update. */
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (m_AbilityInstance->IsValid())
		{
			m_AbilityInstance->AsyncUpdate(m_CurrentTime, m_DeltaTime);
		}
	}

	/* Returns the mode of this Task. */
	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::FireAndForget; }
	
	/* Returns the desired thread of this Task. */
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }

	/* Returns the Profiler Stat ID of this Task. */
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncAbilityInstanceUpdaterTask, STATGROUP_TaskGraphTasks);
	}

private:
	/* Ability Instance to Update. */
	FAblAbilityInstance* m_AbilityInstance;
	
	/* Current Ability Time. */
	float m_CurrentTime;

	/* Delta Time*/
	float m_DeltaTime;
};

class FAsyncAbilityCooldownUpdaterTask
{
public:
	// We make a copy of the Current time since it is written to during the sync update.
	FAsyncAbilityCooldownUpdaterTask(UAblAbilityComponent* InComponent, float InDeltaTime)
		: m_AbilityComponent(InComponent),
		m_DeltaTime(InDeltaTime)
	{ }

	/* Update all Cooldowns. */
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (m_AbilityComponent.IsValid())
		{
			m_AbilityComponent->UpdateCooldowns(m_DeltaTime);
		}
	}

	/* Returns the mode of this Task. */
	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::FireAndForget; }
	
	/* Returns the desired thread of this Task. */
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }
	
	/* Returns the Profiler Stat ID for this Task. */
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncAbilityInstanceUpdaterTask, STATGROUP_TaskGraphTasks);
	}

private:
	/* The Ability Component to update Cooldowns on. */
	TWeakObjectPtr<UAblAbilityComponent> m_AbilityComponent;

	/* The Delta Time to apply. */
	float m_DeltaTime;
};

struct FAblFindAbilityInstanceByHash
{
	FAblFindAbilityInstanceByHash(uint32 InAbilityHash)
		: AbilityHash(InAbilityHash)
	{

	}

	/* The Hash to find. */
	uint32 AbilityHash;

	bool operator()(const FAblAbilityInstance& A) const
	{
		return A.GetAbility().GetAbilityNameHash() == AbilityHash;
	}
};

struct FAblFindAbilityNetworkContextByHash
{
	FAblFindAbilityNetworkContextByHash(uint32 InAbilityHash)
		: AbilityHash(InAbilityHash)
	{

	}

	/* The Hash to find. */
	uint32 AbilityHash;

	bool operator()(const FAblAbilityNetworkContext& A) const
	{
		return A.GetAbility()->GetAbilityNameHash() == AbilityHash;
	}
};

struct FAblAbilityInstanceWhiteList
{
	FAblAbilityInstanceWhiteList(const TArray<uint32>& InAbilityHashWhiteList, bool InCallFinish = true)
		: AbilityHashWhiteList(InAbilityHashWhiteList), CallFinish(InCallFinish)
	{

	}

	/* The Hashs of all Abilities on our Whitelist. */
	TArray<uint32> AbilityHashWhiteList;

	/* True if Finish Ability should be called on all entities not in the whitelist before removing them.*/
	bool CallFinish;

	bool operator()(FAblAbilityInstance& A) const
	{
		check(A.IsValid());
		bool Remove = !AbilityHashWhiteList.Contains(A.GetAbility().GetAbilityNameHash());
		if (Remove && CallFinish)
		{
			A.FinishAbility();
		}
		return Remove;
	}
};

struct FAblAbilityNetworkContextWhiteList
{
	FAblAbilityNetworkContextWhiteList(const TArray<uint32>& InAbilityHashWhiteList)
		: AbilityHashWhiteList(InAbilityHashWhiteList)
	{

	}

	/* Ability Hash Whitelist*/
	TArray<uint32> AbilityHashWhiteList;

	bool operator()(const FAblAbilityNetworkContext& A) const
	{
		return !AbilityHashWhiteList.Contains(A.GetAbility()->GetAbilityNameHash());
	}
};

struct FAblAbilityResultSortByDistance
{
	FAblAbilityResultSortByDistance(const FVector& InSourceLocation, bool InUse2DDistance, bool InSortAscending)
		: SourceLocation(InSourceLocation),
		Use2DDistance(InUse2DDistance),
		SortAscending(InSortAscending)
	{

	}

	/* The Location to use in our Sort comparision. */
	FVector SourceLocation;

	/* True if we should use 2D Distance instead of 3D distance. */
	bool Use2DDistance;

	/* Whether to sort the results in ascending or descending. */
	bool SortAscending;

	bool operator()(const FAblQueryResult& A, const FAblQueryResult& B) const
	{
		float DistanceA = Use2DDistance ? FVector::DistSquaredXY(SourceLocation, A.GetLocation()) : FVector::DistSquared(SourceLocation, A.GetLocation());
		float DistanceB = Use2DDistance ? FVector::DistSquaredXY(SourceLocation, B.GetLocation()) : FVector::DistSquared(SourceLocation, B.GetLocation());

		return SortAscending ? DistanceA < DistanceB : DistanceA > DistanceB;
	}
};

struct FAbleLogHelper
{
	/* Returns the provided Result as a human readable string. */
	ABLECORE_API static const FString GetResultEnumAsString(EAblAbilityStartResult Result);
	ABLECORE_API static const FString GetTaskResultEnumAsString(EAblAbilityTaskResult Result);
	ABLECORE_API static const FString GetTargetTargetEnumAsString(EAblAbilityTargetType Result);
	ABLECORE_API static const FString GetCollisionResponseEnumAsString(ECollisionResponse Response);
	ABLECORE_API static const FString GetCollisionChannelEnumAsString(ECollisionChannel Channel);
    ABLECORE_API static const FString GetWorldName(const UWorld* World);
};