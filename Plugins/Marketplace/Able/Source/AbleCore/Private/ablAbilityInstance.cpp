// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ablAbilityInstance.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/ScopeLock.h"

struct FAblAbilityTaskIsDonePredicate
{
	FAblAbilityTaskIsDonePredicate(float InCurrentTime)
		: CurrentTime(InCurrentTime)
	{

	}

	float CurrentTime;

	bool operator()(const UAblAbilityTask* A) const
	{
		return CurrentTime >= A->GetEndTime();
	}
};

FAblAbilityInstance::FAblAbilityInstance()
: m_DecayTime(0.0f),
m_AsyncTasks(),
m_ActiveAsyncTasks(),
m_FinishedAyncTasks(),
m_SyncTasks(),
m_ActiveSyncTasks(),
m_FinishedSyncTasks(),
m_Ability(nullptr),
m_Context(nullptr),
m_ClearTargets(false),
m_AdditionalTargets(),
m_RequestedInstigator(),
m_RequestedOwner(),
m_RequestedTargetLocation(FVector::ZeroVector)
{

}

void FAblAbilityInstance::Initialize(UAblAbilityContext& AbilityContext)
{
	AbilityContext.SetLoopIteration(0);
	m_Ability = AbilityContext.GetAbility();
	m_Ability->PreExecutionInit();
	m_Context = &AbilityContext;

	ENetMode NetMode = NM_Standalone;
	if (AbilityContext.GetSelfActor())
	{
		NetMode = AbilityContext.GetSelfActor()->GetNetMode();

        AbilityContext.SetAbilityActorStartLocation(AbilityContext.GetSelfActor()->GetActorLocation());
	}

	// Sort our Tasks into Sync/Async queues.
	const TArray<UAblAbilityTask*> & Tasks = m_Ability->GetTasks();
	for (UAblAbilityTask* Task : Tasks)
	{
        if (!Task || Task->IsDisabled(m_Context))
        {
            continue;
        }

		if (Task->IsValidForNetMode(NetMode))
		{
			if (Task->IsAsyncFriendly())
			{
				m_AsyncTasks.Add(Task);
			}
			else
			{
				m_SyncTasks.Add(Task);
			}
		}
	}

	m_FinishedAyncTasks.Reserve(m_AsyncTasks.Num());
	m_FinishedSyncTasks.Reserve(m_SyncTasks.Num());

	// Populate our map of Tasks we need to keep track of due to other Tasks being dependent on them.
	const TArray<const UAblAbilityTask*>& TaskDependencies = m_Ability->GetAllTaskDependencies();
	for (const UAblAbilityTask* TaskDependency : TaskDependencies)
	{
        if (TaskDependency == nullptr)
            continue;

		if (!m_TaskDependencyMap.Contains(TaskDependency))
		{
			m_TaskDependencyMap.Add(TaskDependency, false);
		}
	}

	// Set our initial stacks.
	SetStackCount(FMath::Max(m_Ability->GetInitialStacks(m_Context), 0));

	// Tell our Delegates
	if (UAblAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityStart().Broadcast(*m_Context);

		// BP
		AbilityComponent->AbilityStartBPDelegate.Broadcast(m_Context);
	}

	// Call our OnAbilityStart
	m_Ability->OnAbilityStartBP(m_Context);
}

bool FAblAbilityInstance::PreUpdate()
{
	// Copy over any targets that should be added to our context.
	check(m_Context != nullptr);

	if (m_ClearTargets)
	{
		m_Context->GetMutableTargetActors().Empty();
		m_ClearTargets = false;
	}

	if (m_AdditionalTargets.Num())
	{
		m_Context->GetMutableTargetActors().Append(m_AdditionalTargets);
		m_AdditionalTargets.Empty();
	}

	if (m_RequestedInstigator.IsValid())
	{
		m_Context->SetInstigator(m_RequestedInstigator.Get());
		m_RequestedInstigator.Reset();
	}

	if (m_RequestedOwner.IsValid())
	{
		m_Context->SetOwner(m_RequestedOwner.Get());
		m_RequestedOwner.Reset();
	}

	if (m_RequestedTargetLocation.SizeSquared() > 0.0f)
	{
		m_Context->SetTargetLocation(m_RequestedTargetLocation);
		m_RequestedTargetLocation = FVector::ZeroVector;
	}

	if (!m_Context->GetSelfAbilityComponent()->IsAuthoritative() && 
	    m_Context->GetSelfAbilityComponent()->IsOwnerLocallyControlled())
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(m_Context->GetSelfActor()) )
		{
			UCharacterMovementComponent* OwnerCharacterMovement = Cast<UCharacterMovementComponent>(OwnerCharacter->GetMovementComponent());
			if (OwnerCharacterMovement)
			{
				OwnerCharacterMovement->FlushServerMoves();
			}
		}
	}

	return true;
}

uint32 FAblAbilityInstance::GetAbilityNameHash() const
{
	check(m_Ability != nullptr);
	return m_Ability->GetAbilityNameHash();
}

bool FAblAbilityInstance::HasAsyncTasks() const
{
	return m_AsyncTasks.Num() > 0 || m_ActiveAsyncTasks.Num() > 0;
}

void FAblAbilityInstance::ResetForNextIteration()
{
	// Stop any active tasks.
	for (int32 i = 0; i < m_ActiveAsyncTasks.Num(); )
	{
		if (m_ActiveAsyncTasks[i]->GetResetForIterations())
		{
			m_ActiveAsyncTasks[i]->OnTaskEnd(m_Context, EAblAbilityTaskResult::Successful);
			m_ActiveAsyncTasks.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}

	for (int32 i = 0; i < m_ActiveSyncTasks.Num(); )
	{
		if (m_ActiveSyncTasks[i]->GetResetForIterations())
		{
			m_ActiveSyncTasks[i]->OnTaskEnd(m_Context, EAblAbilityTaskResult::Successful);
			m_ActiveSyncTasks.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}

	m_Context->SetLoopIteration(m_Context->GetCurrentLoopIteration() + 1);
	if (m_Ability->IsLooping())
	{
		// This is the start, end time of the Loop. X = Start, Y = End
		const FVector2D& LoopRange = m_Ability->GetLoopRange();
		if (m_Context->GetCurrentTime() > LoopRange.Y)
		{
			// Reset our time to the start of the loop range, and we'll keep going.
			m_Context->SetCurrentTime(LoopRange.X);

			m_FinishedAyncTasks.RemoveAll([&](const UAblAbilityTask* Task)
			{
				return Task->GetStartTime() >= LoopRange.X && Task->GetEndTime() <= LoopRange.Y;
			});

			m_FinishedSyncTasks.RemoveAll([&](const UAblAbilityTask* Task)
			{
				return Task->GetStartTime() >= LoopRange.X && Task->GetEndTime() <= LoopRange.Y;
			});

			// Sort our Tasks into Sync/Async queues (again, people apparently want to dynamically turn on/off tasks).
			m_AsyncTasks.Empty(m_AsyncTasks.Num());
			m_SyncTasks.Empty(m_SyncTasks.Num());

			const TArray<UAblAbilityTask*>& Tasks = m_Ability->GetTasks();
			for (UAblAbilityTask* Task : Tasks)
			{
				if (!Task || Task->IsDisabled(m_Context))
				{
					continue;
				}

				ENetMode NetMode = NM_Standalone;
				if (m_Context->GetSelfActor())
				{
					NetMode = m_Context->GetSelfActor()->GetNetMode();
				}

				if (Task->IsValidForNetMode(NetMode))
				{
					if (Task->IsAsyncFriendly() && !m_ActiveAsyncTasks.Contains(Task))
					{
						m_AsyncTasks.Add(Task);
					}
					else if (!m_ActiveSyncTasks.Contains(Task))
					{
						m_SyncTasks.Add(Task);
					}
				}
			}
		}
	}
	else
	{
		m_Context->SetCurrentTime(0.0f);
	}

	if (GetStackCount() != 0
		&& m_Ability->GetDecrementAndRestartOnEnd())
	{
		int32 stackDecrement = m_Ability->GetStackDecrement(m_Context);
		int32 newStackValue = FMath::Max(GetStackCount() - stackDecrement, 0);

		m_Context->GetSelfAbilityComponent()->SetPassiveStackCount(m_Context->GetAbility(), newStackValue, true, EAblAbilityTaskResult::Successful);
	}

	m_FinishedAyncTasks.Empty();
	m_FinishedSyncTasks.Empty();

}

bool FAblAbilityInstance::IsIterationDone() const
{
	if (GetCurrentTime() > m_Ability->GetLength())
	{
		if (m_Ability->MustFinishAllTasks())
		{
			return m_ActiveSyncTasks.Num() == 0 && m_ActiveAsyncTasks.Num() == 0;
		}

		return true;
	}
	else if (m_Ability->IsLooping() && m_Context->GetCurrentLoopIteration() != 0)
	{
		if (GetCurrentTime() > m_Ability->GetLoopRange().Y) // If we're looping, check our range.
		{
			if (m_Ability->MustFinishAllTasks())
			{
				return m_ActiveSyncTasks.Num() == 0 && m_ActiveAsyncTasks.Num() == 0;
			}

			return true;
		}
	}

	return false;
}

bool FAblAbilityInstance::IsDone() const
{
	check(m_Ability != nullptr);
	if (m_Ability->IsLooping() && m_Ability->CustomCanLoopExecuteBP(m_Context))
	{
		const uint32 MaxIterations = m_Ability->GetLoopMaxIterations(m_Context);
		// Zero = infinite.
		return MaxIterations != 0 && (uint32)m_Context->GetCurrentLoopIteration() >= MaxIterations;
	}

	if (m_Ability->GetDecrementAndRestartOnEnd() && m_Ability->GetMaxStacksBP(m_Context) != 0)
	{
		return GetStackCount() == 0;
	}

	return true;
}

bool FAblAbilityInstance::IsChanneled() const
{
	return m_Ability != nullptr && m_Ability->IsChanneled();
}

int FAblAbilityInstance::CheckForDecay(UAblAbilityComponent* AbilityComponent)
{
    if (m_Ability != nullptr)
    {
        double decayStackTime = (double)m_Ability->GetDecayStackTime(&GetContext());
		double remainder = 0.0f;
        if (decayStackTime > 0.0f)
        {
            int cancelStackCount = (int)UKismetMathLibrary::FMod64((double)m_DecayTime, (double)decayStackTime, remainder);
            if (cancelStackCount > 0)
            {
                int newStackCount = FMath::Max(GetStackCount() - cancelStackCount, 0);
                AbilityComponent->SetPassiveStackCount(&GetAbility(), newStackCount, false, EAblAbilityTaskResult::Decayed);
                return true;
            }
        }
    }
    return false;
}

EAblConditionResults FAblAbilityInstance::CheckChannelConditions() const
{
	check(m_Ability != nullptr);

#if WITH_EDITOR
    if (UWorld* World = m_Context->GetWorld())
    {
        if (World->WorldType == EWorldType::Type::EditorPreview)
            return EAblConditionResults::ACR_Passed;
    }
#endif
	EAblConditionResults ConditionResult = EAblConditionResults::ACR_Failed;
	for (const UAblChannelingBase* Condition : m_Ability->GetChannelConditions())
	{
		ConditionResult = Condition->GetConditionResult(*m_Context);

		if (ConditionResult == EAblConditionResults::ACR_Passed && !m_Ability->MustPassAllChannelConditions())
		{
			// One condition passed.
			return EAblConditionResults::ACR_Passed;
		}
		else if (ConditionResult == EAblConditionResults::ACR_Failed && m_Ability->MustPassAllChannelConditions())
		{
			return EAblConditionResults::ACR_Failed;
		}
	}

	return ConditionResult;
}

bool FAblAbilityInstance::RequiresServerNotificationOfChannelFailure() const
{
	for (const UAblChannelingBase* Condition : m_Ability->GetChannelConditions())
	{
		if (Condition && Condition->RequiresServerNotificationOfFailure())
		{
			return true;
		}
	}

	return false;
}

EAblAbilityTaskResult FAblAbilityInstance::GetChannelFailureResult() const
{
	check(m_Ability != nullptr);
	return m_Ability->GetChannelFailureResult();
}

void FAblAbilityInstance::ResetTime(bool ToLoopStart /*= false*/)
{
	check(m_Ability != nullptr);
	if (ToLoopStart && m_Ability->IsLooping())
	{
		// Reset to the start of our loop.
		m_Context->SetCurrentTime(m_Ability->GetLoopRange().X);
	}
	else
	{
		m_Context->SetCurrentTime(0.0f);
	}
}

void FAblAbilityInstance::AsyncUpdate(float CurrentTime, float DeltaTime)
{
	InternalUpdateTasks(m_AsyncTasks, m_ActiveAsyncTasks, m_FinishedAyncTasks, CurrentTime, DeltaTime);
}

void FAblAbilityInstance::SyncUpdate(float DeltaTime)
{
	const float CurrentTime = m_Context->GetCurrentTime();
	const float AdjustedTime = CurrentTime + DeltaTime;

	InternalUpdateTasks(m_SyncTasks, m_ActiveSyncTasks, m_FinishedSyncTasks, CurrentTime, DeltaTime);

	m_Context->UpdateTime(DeltaTime);

    m_DecayTime += DeltaTime;
}

void FAblAbilityInstance::SetStackCount(int32 TotalStacks)
{
	check(m_Context != nullptr);
	m_Context->SetStackCount(TotalStacks);
}

int32 FAblAbilityInstance::GetStackCount() const
{
	check(m_Context != nullptr);
	return m_Context->GetCurrentStackCount();
}

void FAblAbilityInstance::AddAdditionalTargets(const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool AllowDuplicates /*= false*/, bool ClearTargets /*= false*/)
{
	check(m_Context != nullptr);

	const TArray<TWeakObjectPtr<AActor>>& CurrentTargets = m_Context->GetTargetActorsWeakPtr();

	if (AllowDuplicates)
	{
		FScopeLock Lock(&m_AddTargetCS);
		// Simple Append.
		m_AdditionalTargets.Append(AdditionalTargets);
	}
	else
	{
		//Worst Case is O(n^2) which sucks, but hopefully our list is small.
		TArray<TWeakObjectPtr<AActor>> UniqueEntries;

		for (const TWeakObjectPtr<AActor>& Target : AdditionalTargets)
		{
			if (!CurrentTargets.Contains(Target) || ClearTargets)
			{
				UniqueEntries.AddUnique(Target);
			}
		}

		FScopeLock Lock(&m_AddTargetCS);
		m_AdditionalTargets.Append(UniqueEntries);
	}

	m_ClearTargets = ClearTargets;
}

void FAblAbilityInstance::ModifyContext(AActor* Instigator, AActor* Owner, const FVector& TargetLocation, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool ClearTargets /*= false*/)
{
	FScopeLock Lock(&m_AddTargetCS); // Just re-use this.
	m_AdditionalTargets.Append(AdditionalTargets);
	m_RequestedInstigator = Instigator;
	m_RequestedOwner = Owner;
	m_RequestedTargetLocation = TargetLocation;
	m_ClearTargets = ClearTargets;
}

void FAblAbilityInstance::StopAbility()
{
	InternalStopRunningTasks(EAblAbilityTaskResult::Successful);

	// Tell our Delegates
	if (UAblAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAblAbilityTaskResult::Successful);

		// BP
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAblAbilityTaskResult::Successful);
	}

	// Call our Ability method
	m_Ability->OnAbilityEndBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

void FAblAbilityInstance::FinishAbility()
{
	InternalStopRunningTasks(EAblAbilityTaskResult::Successful);

	// Tell our Delegates
	if (UAblAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAblAbilityTaskResult::Successful);

		// BP
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAblAbilityTaskResult::Successful);
	}

	// Call our Ability method.
	m_Ability->OnAbilityEndBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

void FAblAbilityInstance::InterruptAbility()
{
	InternalStopRunningTasks(EAblAbilityTaskResult::Interrupted);

	// Tell our Delegates
	if (UAblAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityInterrupt().Broadcast(*m_Context);
        AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAblAbilityTaskResult::Interrupted);

		// BP
		AbilityComponent->AbilityInterruptBPDelegate.Broadcast(m_Context);
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAblAbilityTaskResult::Interrupted);
	}

	// Call our Ability method.
	m_Ability->OnAbilityInterruptBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

void FAblAbilityInstance::BranchAbility()
{
	InternalStopRunningTasks(EAblAbilityTaskResult::Branched);

	// Tell our Delegates
	if (UAblAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++ 
		AbilityComponent->GetOnAbilityBranched().Broadcast(*m_Context);
		AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAblAbilityTaskResult::Branched);

		// BP
		AbilityComponent->AbilityBranchBPDelegate.Broadcast(m_Context);
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAblAbilityTaskResult::Branched);
	}

	// Call our Ability method.
	m_Ability->OnAbilityBranchBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

UAblAbilityContext& FAblAbilityInstance::GetMutableContext()
{
	verify(m_Context != nullptr)
	return *m_Context;
}

const UAblAbilityContext& FAblAbilityInstance::GetContext() const
{
	verify(m_Context != nullptr);
	return *m_Context;
}

const UAblAbility& FAblAbilityInstance::GetAbility() const
{
	verify(m_Ability != nullptr);
	return *m_Ability;
}

float FAblAbilityInstance::GetPlayRate() const
{
	return GetAbility().GetPlayRate(m_Context);
}

void FAblAbilityInstance::SetCurrentTime(float NewTime)
{
	m_Context->SetCurrentTime(NewTime);

	TArray<UAblAbilityTask*> NewTasks;
	TArray<UAblAbilityTask*> CurrentTasks;

	const TArray<UAblAbilityTask*> & Tasks = m_Ability->GetTasks();
	for (UAblAbilityTask* Task : Tasks)
	{
		if (Task && Task->CanStart(m_Context, NewTime, 0.0f ) && !Task->IsDisabled(m_Context))
		{
			if (Task->IsAsyncFriendly() ? m_ActiveAsyncTasks.Contains(Task) : m_ActiveSyncTasks.Contains(Task))
			{
				CurrentTasks.Add(Task);
			}
			else
			{
				NewTasks.Add(Task);
			}
		}
	}

	// Now go through and Terminate any tasks that should no longer be running.
	for (int i = 0; i < m_ActiveSyncTasks.Num();)
	{
		if (!CurrentTasks.Contains(m_ActiveSyncTasks[i]))
		{
			m_ActiveSyncTasks[i]->OnTaskEnd(m_Context, Successful);
			m_ActiveSyncTasks.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}

	for (int i = 0; i < m_ActiveAsyncTasks.Num();)
	{
		if (!CurrentTasks.Contains(m_ActiveAsyncTasks[i]))
		{
			m_ActiveAsyncTasks[i]->OnTaskEnd(m_Context, Successful);
			m_ActiveAsyncTasks.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}

	// Start our New Tasks.
	for (UAblAbilityTask* Task : NewTasks)
	{
		Task->OnTaskStart(m_Context);

		if (Task->IsAsyncFriendly())
		{
			m_ActiveAsyncTasks.Add(Task);
		}
		else
		{
			m_ActiveSyncTasks.Add(Task);
		}
	}

	// Tell our Current Tasks to go ahead and do any logic they need to if the time was modified out from under them.
	for (UAblAbilityTask* Task : CurrentTasks)
	{
		Task->OnAbilityTimeSet(m_Context);
	}
}

void FAblAbilityInstance::SetCurrentIteration(uint32 NewIteration)
{
	m_Context->SetLoopIteration(NewIteration);
}

void FAblAbilityInstance::Reset()
{
	// Defaulting to false on the shrink for the arrays. These array entry values are so small, that having them constantly go in/out of allocation could be pretty gross for fragmentation.
	// Just keep the memory for now, and if it becomes an issue (not sure why it would), then just remove the false and let the memory get released.
	m_DecayTime = 0.0f;
	m_AsyncTasks.Empty(false);
	m_ActiveAsyncTasks.Empty(false);
	m_FinishedAyncTasks.Empty(false);
	m_SyncTasks.Empty(false);
	m_ActiveSyncTasks.Empty(false);
	m_FinishedSyncTasks.Empty(false);
	m_Ability = nullptr;
	m_Context = nullptr;
	m_ClearTargets = false;
	m_AdditionalTargets.Empty(false);
	m_TaskDependencyMap.Empty(false);
	m_RequestedInstigator.Reset();
	m_RequestedOwner.Reset();
	m_RequestedTargetLocation = FVector::ZeroVector;
}

void FAblAbilityInstance::InternalUpdateTasks(TArray<const UAblAbilityTask*>& InTasks, TArray<const UAblAbilityTask*>& InActiveTasks, TArray<const UAblAbilityTask*>& InFinishedTasks, float CurrentTime, float DeltaTime)
{
	const float AdjustedTime = CurrentTime + DeltaTime;
	const bool IsLooping = m_Ability->IsLooping() || m_Ability->GetDecrementAndRestartOnEnd();
	const FVector2D LoopTimeRange = m_Ability->GetLoopRange();

	// Just for readability...
	bool IsStarting = false;

	// if we need to clean up any active tasks.
	bool NeedArrayCleanUp = false;

	TArray<const UAblAbilityTask*> NewlyStartedTasks;
	TArray<const UAblAbilityTask*> FinishedDependentTasks;

	// First go through our Tasks and see if they need to be started.
	for (int i = 0; i < InTasks.Num(); )
	{
		const UAblAbilityTask* Task = InTasks[i];

		IsStarting = Task->CanStart(m_Context, CurrentTime, DeltaTime) && !Task->IsDisabled(m_Context);

		if (IsLooping)
		{
			// If we're looping (And thus not cleaning up tasks as we complete them), make sure we haven't already finished this task.
			IsStarting = IsStarting && !InFinishedTasks.Contains(Task);
		}

		// Check any dependencies.
		if (IsStarting && Task->HasDependencies())
		{
			for (const UAblAbilityTask* DependantTask : Task->GetTaskDependencies())
			{
				if (!DependantTask)
				{
					continue;
				}

				// This *should* always be true, but in the editor sometimes the list isn't properly updated.
				// This is by far the assert I hit the most during iteration, so there's obviously a case I'm missing but I'd rather make this code a bit
				// more adaptive as it was written before I added the finished lists for both processes.
				if (!m_TaskDependencyMap.Contains(DependantTask))
				{
					FScopeLock DependencyMapLock(&m_DependencyMapCS);

					m_TaskDependencyMap.Add(DependantTask);
					m_TaskDependencyMap[DependantTask] = !DependantTask->IsAsyncFriendly() ? m_FinishedSyncTasks.Contains(DependantTask) : m_FinishedAyncTasks.Contains(DependantTask);
				}

				if (!m_TaskDependencyMap[DependantTask])
				{
					// A dependent Task is still executing, we can't start this frame.
					IsStarting = false;
					break;
				}
				else
				{
					// Our dependency is done, but we have some context targets that are pending. These could be needed, so delay for a frame.
					IsStarting = m_AdditionalTargets.Num() == 0;
				}
			}
		}

		if (IsStarting && !InActiveTasks.Contains(Task))
		{
			FScopeCycleCounter TaskScope(Task->GetStatId());

			// New Task to start.
			Task->OnTaskStart(m_Context);
			if (Task->IsSingleFrame())
			{
				// We can go ahead and end this task and forget about it.
				Task->OnTaskEnd(m_Context, EAblAbilityTaskResult::Successful);

				if (m_TaskDependencyMap.Contains(Task))
				{
					FScopeLock DependencyMapLock(&m_DependencyMapCS);
					m_TaskDependencyMap[Task] = true;
				}

				InFinishedTasks.Add(Task);
			}
			else
			{
				NewlyStartedTasks.Add(Task);
			}

			if (!IsLooping || Task->GetStartTime() < LoopTimeRange.X)
			{
				// If we aren't looping, or our task starts before our loop range, we can remove items from this list as we start tasks to cut down on future iterations.
				InTasks.RemoveAt(i, 1, false);
				continue;
			}
		}
		else if (AdjustedTime < Task->GetStartTime())
		{
			// Since our array is sorted by time. It's safe to early out.
			break;
		}

		++i;
	}

	InTasks.Shrink();

	// Update our actives.
	bool TaskCompleted = false;
	const UAblAbilityTask* ActiveTask = nullptr;
	for (int i = 0; i < InActiveTasks.Num(); )
	{
		ActiveTask = InActiveTasks[i];

		FScopeCycleCounter ActiveTaskScope(ActiveTask->GetStatId());

		TaskCompleted = ActiveTask->IsDone(m_Context);
		if (!TaskCompleted && ActiveTask->NeedsTick())
		{
			ActiveTask->OnTaskTick(m_Context, DeltaTime);
		}
		else if (TaskCompleted)
		{
			ActiveTask->OnTaskEnd(m_Context, EAblAbilityTaskResult::Successful);

			if (m_TaskDependencyMap.Contains(ActiveTask))
			{
				FScopeLock DependencyMapLock(&m_DependencyMapCS);
				m_TaskDependencyMap[ActiveTask] = true;
			}

			if (IsLooping)
			{
				InFinishedTasks.Add(ActiveTask);
			}

			InActiveTasks.RemoveAt(i, 1, false);
			continue;
		}

		++i;
	}

	// Move our newly started tasks over.
	InActiveTasks.Append(NewlyStartedTasks);

	InActiveTasks.Shrink();
}

void FAblAbilityInstance::InternalStopRunningTasks(EAblAbilityTaskResult Reason, bool ResetForLoop)
{
	if (ResetForLoop)
	{
		// We only want to remove Tasks that fall within our Loop time range.
		const FVector2D LoopRange = m_Ability->GetLoopRange();

		const UAblAbilityTask* CurrentTask = nullptr;

		for (int32 i = 0; i < m_ActiveAsyncTasks.Num();)
		{
			CurrentTask = m_ActiveAsyncTasks[i];
			if (CurrentTask->GetStartTime() >= LoopRange.X && CurrentTask->GetEndTime() <= LoopRange.Y)
			{
				CurrentTask->OnTaskEnd(m_Context, Reason);
				m_ActiveAsyncTasks.RemoveAt(i, 1, false);
				continue;
			}
			++i;
		}

		m_ActiveAsyncTasks.Shrink();

		for (int32 i = 0; i < m_ActiveSyncTasks.Num();)
		{
			CurrentTask = m_ActiveSyncTasks[i];
			if (CurrentTask->GetStartTime() >= LoopRange.X && CurrentTask->GetEndTime() <= LoopRange.Y)
			{
				CurrentTask->OnTaskEnd(m_Context, Reason);
				m_ActiveSyncTasks.RemoveAt(i, 1, false);
				continue;
			}
			++i;
		}

		m_ActiveSyncTasks.Shrink();

	}
	else
	{
		for (const UAblAbilityTask* Task : m_ActiveAsyncTasks)
		{
			Task->OnTaskEnd(m_Context, Reason);
		}
		m_ActiveAsyncTasks.Empty();

		for (const UAblAbilityTask* Task : m_ActiveSyncTasks)
		{
			Task->OnTaskEnd(m_Context, Reason);
		}
		m_ActiveSyncTasks.Empty();
	}

}

void FAblAbilityInstance::ResetTaskDependencyStatus()
{
	TMap<const UAblAbilityTask*, bool>::TIterator ResetIt = m_TaskDependencyMap.CreateIterator();
	const UAblAbilityTask* Task;
	for (; ResetIt; ++ResetIt)
	{
		Task = ResetIt.Key();
		if (!m_Ability->IsLooping())
		{
			ResetIt.Value() = false;
		}
		else
		{
			// Only reset if our task falls within our Loop range.
			if (Task->GetStartTime() > m_Ability->GetLoopRange().X)
			{
				ResetIt.Value() = false;
			}
		}
	}
}
