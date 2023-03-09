// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ablAbility.h"
#include "AbleCorePrivate.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Misc/Crc.h"
#include "Tasks/ablPlayAnimationTask.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectSaveContext.h"

#define LOCTEXT_NAMESPACE "AblAbility"

UAblAbility::UAblAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_Length(1.0f),
	m_Cooldown(0.0f),
	m_CacheCooldown(true),
	m_FinishAllTasks(false),
	m_PlayRate(1.0f),
	m_IsPassive(false),
	m_InitialStackCount(1),
	m_StackIncrement(1),
	m_StackDecrement(1),
	m_MaxStacks(1),
	m_RefreshDurationOnNewStack(true),
	m_AlwaysRefreshDuration(false),
	m_OnlyRefreshLoopTime(false),
	m_ResetLoopCountOnRefresh(false),
	m_DecrementAndRestartOnEnd(false),
	m_Loop(false),
	m_LoopStart(0.0f),
	m_LoopEnd(1.0f),
	m_LoopMaxIterations(0U),
	m_RequiresTarget(false),
	m_Targeting(nullptr),
    m_DecayStackTime(0.0f),
	m_IsChanneled(false),
	m_ChannelConditions(),
	m_MustPassAllChannelConditions(true),
	m_FailedChannelResult(EAblAbilityTaskResult::Interrupted),
	m_Tasks(),
	m_InstancePolicy(EAblInstancePolicy::Default),
	m_ClientPolicy(EAblClientExecutionPolicy::Default),
	m_AbilityNameHash(0U),
	m_AbilityRealm(0),
	m_DependenciesDirty(true)
{
#if WITH_EDITORONLY_DATA
	ThumbnailImage = nullptr;
	ThumbnailInfo = nullptr;
	DrawTargetingShapes = false;
	IsCollapsed = false;
#endif
}

void UAblAbility::PostInitProperties()
{
	Super::PostInitProperties();
}

void UAblAbility::PreSave(FObjectPreSaveContext SaveContext)
{
#if WITH_EDITOR
	SanitizeTasks();
	SortTasks();
#endif
	Super::PreSave(SaveContext);
}

void UAblAbility::PostLoad()
{
	Super::PostLoad();

    // remove any broken references
    const int32 numTasks = m_Tasks.Num();
    m_Tasks.RemoveAll([](const UAblAbilityTask* Source) { return Source == nullptr; });
    if (numTasks < m_Tasks.Num())
    {
        UE_LOG(LogAble, Warning, TEXT("UAblAbility.PostLoad() %s had %d NULL Tasks."), *GetDisplayName(), m_Tasks.Num()-numTasks);
    }

	BindDynamicProperties();

#if WITH_EDITORONLY_DATA
	for (UAblAbilityTask* ReferencedTask : m_Tasks)
	{
        // Listen for any property changes on our Tasks. 
        ReferencedTask->GetOnTaskPropertyModified().AddUObject(this, &UAblAbility::OnReferencedTaskPropertyModified);
    }
#endif

	// Generate our Name hash.
	m_AbilityNameHash = FCrc::StrCrc32(*GetName());
}

bool UAblAbility::IsSupportedForNetworking() const
{
	return GetInstancePolicy() == EAblInstancePolicy::NewInstanceReplicated || GetOuter()->IsA(UPackage::StaticClass());
}

void UAblAbility::PreExecutionInit() const
{
	if (m_DependenciesDirty)
	{
		BuildDependencyList();
	}
}

EAblAbilityStartResult UAblAbility::CanAbilityExecute(UAblAbilityContext& Context) const
{
	// Check Targeting...
	if (m_Targeting != nullptr)
	{
		// If this is Async, it's safe to call it multiple times as it will poll for the results.
		m_Targeting->FindTargets(Context);

		if (m_Targeting->IsUsingAsync() && Context.HasValidAsyncHandle())
		{
			return EAblAbilityStartResult::AsyncProcessing;
		}

		if (RequiresTarget() && !Context.HasAnyTargets())
		{
			return EAblAbilityStartResult::InvalidTarget;
		}
	}

	// Check Custom BP logic
	if (!CustomCanAbilityExecuteBP(&Context))
	{
		return EAblAbilityStartResult::FailedCustomCheck;
	}

	return EAblAbilityStartResult::Success;
}

FString UAblAbility::GetDisplayName() const
{
	FString Result;
	GetName(Result);
	int32 UnderscoreIndex;

	// Chop off the variant (_A/B/C/D) from the Left side.
	if (Result.FindLastChar('_', UnderscoreIndex))
	{
		int32 UnderscoreSize = Result.Len() - UnderscoreIndex;
	
		static FString DefaultString(TEXT("Default__"));
		if (Result.Find(DefaultString) >= 0)
		{
			int32 StringSize = Result.Len() - UnderscoreSize - DefaultString.Len();
			Result = Result.Mid(DefaultString.Len(), StringSize);
		}
		else
		{
			Result = Result.LeftChop(UnderscoreSize);
		}
	}

	return Result;
}

EAblCallbackResult UAblAbility::OnCollisionEvent(const UAblAbilityContext* Context, const FName& EventName, const TArray<struct FAblQueryResult>& HitEntities) const
{
	return EAblCallbackResult::Complete;
}

EAblCallbackResult UAblAbility::OnRaycastEvent(const UAblAbilityContext* Context, const FName& EventName, const TArray<FHitResult>& HitResults) const
{
	return EAblCallbackResult::Complete;
}

FVector UAblAbility::GetTargetLocation(const UAblAbilityContext* Context) const
{
	if (Context && Context->GetSelfActor())
	{
		return Context->GetSelfActor()->GetActorLocation();
	}

	return FVector::ZeroVector;
}

bool UAblAbility::CheckCustomConditionEventBP_Implementation(const UAblAbilityContext* Context, const FName& EventName) const
{
	return CheckCustomConditionEvent(Context, EventName);
}

void UAblAbility::OnAbilityStackAdded(const UAblAbilityContext* Context) const
{

}

void UAblAbility::OnAbilityStackAddedBP_Implementation(const UAblAbilityContext* Context) const
{
	OnAbilityStackAdded(Context);
}

void UAblAbility::OnAbilityStackRemoved(const UAblAbilityContext* Context) const
{

}

void UAblAbility::OnAbilityStackRemovedBP_Implementation(const UAblAbilityContext* Context) const
{
	OnAbilityStackRemoved(Context);
}

TSubclassOf<UAblAbilityScratchPad> UAblAbility::GetAbilityScratchPadClass(const UAblAbilityContext* Context) const
{
	return nullptr;
}

TSubclassOf<UAblAbilityScratchPad> UAblAbility::GetAbilityScratchPadClassBP_Implementation(const UAblAbilityContext* Context) const
{
	return GetAbilityScratchPadClass(Context);
}

void UAblAbility::ResetAbilityScratchPad(UAblAbilityScratchPad* ScratchPad) const
{
	ResetAbilityScratchPadBP(ScratchPad);
}

void UAblAbility::ResetAbilityScratchPadBP_Implementation(UAblAbilityScratchPad* ScratchPad) const
{
	// Do nothing as we don't know the contents of the Scratchpad.
}

bool UAblAbility::CanClientCancelAbilityBP_Implementation(const UAblAbilityContext* Context) const
{
	return CanClientCancelAbility(Context);
}

USkeletalMeshComponent* UAblAbility::GetSkeletalMeshComponentForActor(const UAblAbilityContext* Context, AActor* Actor, const FName& EventName) const
{
	return GetSkeletalMeshComponentForActorBP(Context, Actor, EventName);
}

USkeletalMeshComponent* UAblAbility::GetSkeletalMeshComponentForActorBP_Implementation(const UAblAbilityContext* Context, AActor* Actor, const FName& EventName) const
{
	return nullptr;
}

UWorld* UAblAbility::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}
	return GetOuter()->GetWorld();
}

int32 UAblAbility::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking())
	{
		// This handles absorbing authority/cosmetic
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UAblAbility::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	check(GetOuter() != nullptr);

	AActor* Owner = CastChecked<AActor>(GetOuter());

	bool bProcessed = false;

	FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
	if (Context != nullptr)
	{
		for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
		{
			if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(Owner, Function))
			{
				Driver.NetDriver->ProcessRemoteFunction(Owner, Function, Parameters, OutParms, Stack, this);
				bProcessed = true;
			}
		}
	}

	return bProcessed;
}

FVector UAblAbility::GetTargetLocationBP_Implementation(const UAblAbilityContext* Context) const
{
	return GetTargetLocation(Context);
}

bool UAblAbility::ShouldCancelAbilityBP_Implementation(const UAblAbilityContext* Context, const UAblAbility* Ability, const FName& EventName) const
{
	return ShouldCancelAbility(Context, Ability, EventName);
}

float UAblAbility::CalculateCooldownBP_Implementation(const UAblAbilityContext* Context) const
{
	return GetCooldown(Context);
}

bool UAblAbility::CanInterruptAbilityBP_Implementation(const UAblAbilityContext* Context, const UAblAbility* CurrentAbility) const
{
	return CanInterruptAbility(Context, CurrentAbility);
}

bool UAblAbility::CustomFilterConditionBP_Implementation(const UAblAbilityContext* Context, const FName& EventName, AActor* Actor) const
{
	return CustomFilterCondition(Context, EventName, Actor);
}

float UAblAbility::CalculateCooldown(const UAblAbilityContext* Context) const
{
	return GetBaseCooldown();
}

int32 UAblAbility::GetMaxStacksBP_Implementation(const UAblAbilityContext* Context) const
{
	return GetMaxStacks(Context);
}

bool UAblAbility::CustomCanAbilityExecuteBP_Implementation(const UAblAbilityContext* Context) const
{
	return CustomCanAbilityExecute(Context);
}

bool UAblAbility::CustomCanLoopExecuteBP_Implementation(const UAblAbilityContext* Context) const
{
	return CustomCanLoopExecute(Context);
}

EAblConditionResults UAblAbility::CheckCustomChannelConditionalBP_Implementation(const UAblAbilityContext* Context, const FName& EventName) const
{
	return CheckCustomChannelConditional(Context, EventName);
}

float UAblAbility::CalculateDamageForActorBP_Implementation(const UAblAbilityContext* Context, const FName& EventName, float BaseDamage, AActor* Actor, FDamageEvent damageEvent) const
{
	return CalculateDamageForActor(Context, EventName, BaseDamage, Actor, damageEvent);
}

bool UAblAbility::CustomCanBranchToBP_Implementation(const UAblAbilityContext* Context, const UAblAbility* BranchAbility) const
{
	return CustomCanBranchTo(Context, BranchAbility);
}

void UAblAbility::OnAbilityStartBP_Implementation(const UAblAbilityContext* Context) const
{
	OnAbilityStart(Context);
}

void UAblAbility::OnAbilityEndBP_Implementation(const UAblAbilityContext* Context) const
{
	OnAbilityEnd(Context);
}

void UAblAbility::OnAbilityInterruptBP_Implementation(const UAblAbilityContext* Context) const
{
	OnAbilityInterrupt(Context);
}

void UAblAbility::OnAbilityBranchBP_Implementation(const UAblAbilityContext* Context) const
{
	OnAbilityBranch(Context);
}

EAblCallbackResult UAblAbility::OnCollisionEventBP_Implementation(const UAblAbilityContext* Context, const FName& EventName, const TArray<FAblQueryResult>& HitEntities) const
{
	return OnCollisionEvent(Context, EventName, HitEntities);
}

EAblCallbackResult UAblAbility::OnRaycastEventBP_Implementation(const UAblAbilityContext* Context, const FName& EventName, const TArray<FHitResult>& HitResults) const
{
	return OnRaycastEvent(Context, EventName, HitResults);
}

void UAblAbility::OnCustomEventBP_Implementation(const UAblAbilityContext* Context, const FName& EventName) const
{
	OnCustomEvent(Context, EventName);
}

void UAblAbility::OnSpawnedActorEventBP_Implementation(const UAblAbilityContext* Context, const FName& EventName, AActor* SpawnedActor, int SpawnIndex) const
{
	OnSpawnedActorEvent(Context, EventName, SpawnedActor, SpawnIndex);
}

void UAblAbility::BuildDependencyList() const
{
	m_AllDependentTasks.Empty();

	for (UAblAbilityTask* Task : m_Tasks)
	{
		if (!Task)
		{
			// Should never occur, but to be safe.
			continue; 
		}

        if (Task->HasDependencies())
        {
			Task->GetMutableTaskDependencies().RemoveAll([](const UAblAbilityTask* LHS){ return LHS == nullptr; });

			for (const UAblAbilityTask* TaskDependency : Task->GetTaskDependencies())
			{
				if (!TaskDependency)
				{
					continue;
				}

                // Make sure our Tasks and Dependencies are in the same realms (or Client/Server so they'll always run) and that they aren't stale somehow.
                if ((TaskDependency->GetTaskRealm() == Task->GetTaskRealm() ||
                    TaskDependency->GetTaskRealm() == EAblAbilityTaskRealm::ATR_ClientAndServer ||
                    Task->GetTaskRealm() == EAblAbilityTaskRealm::ATR_ClientAndServer) &&
                    m_Tasks.Contains(TaskDependency))
                {
                    m_AllDependentTasks.AddUnique(TaskDependency);
                }
            }
        }
	}

	m_DependenciesDirty = false;
}

FName UAblAbility::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_") + PropertyName;
	return FName(*DelegateName);
}

void UAblAbility::BindDynamicProperties()
{
	for (UAblAbilityTask* Task : m_Tasks )
    {
        Task->BindDynamicDelegates(this);
    }

	for (UAblChannelingBase* Channel : m_ChannelConditions)
	{
		if (Channel)
		{
			Channel->BindDynamicDelegates(this);
		}
	}

	if (m_Targeting)
	{
		m_Targeting->BindDynamicDelegates(this);
	}

	ABL_BIND_DYNAMIC_PROPERTY(this, m_Cooldown, TEXT("Cooldown"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_PlayRate, TEXT("Play Rate"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_InitialStackCount, TEXT("Initial Stacks"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_StackIncrement, TEXT("Stack Increment"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_StackDecrement, TEXT("Stack Decrement"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_MaxStacks, TEXT("Max Stacks"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_LoopMaxIterations, TEXT("Max Iterations"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_IsPassive, TEXT("Is Passive"));
    ABL_BIND_DYNAMIC_PROPERTY(this, m_DecayStackTime, TEXT("Stack Decay Time"));    
}

bool UAblAbility::HasTag(const FGameplayTag Tag) const
{
    return m_TagContainer.HasTag(Tag);
}

/* Returns true if we have any tags from the passed in container in our own container. */
bool UAblAbility::MatchesAnyTag(const FGameplayTagContainer Container) const
{
    return m_TagContainer.HasAny(Container);
}

/* Returns true if we have all the tags from the passed in container in our own container. */
bool UAblAbility::MatchesAllTags(const FGameplayTagContainer Container) const
{
    return m_TagContainer.HasAll(Container);
}

/* Returns true if the ability checks pass for a series of gameplay tags. */
bool UAblAbility::CheckTags(const FGameplayTagContainer& IncludesAny, const FGameplayTagContainer& IncludesAll, const FGameplayTagContainer& ExcludesAny) const
{
    if (!ExcludesAny.IsEmpty())
    {
        if (m_TagContainer.HasAny(ExcludesAny))
            return false;
    }
    if (!IncludesAny.IsEmpty())
    {
        if (!m_TagContainer.HasAny(IncludesAny))
            return false;
    }
    if (!IncludesAll.IsEmpty())
    {
        if (!m_TagContainer.HasAll(IncludesAll))
            return false;
    }
    return false;
}

const TArray<UAblAbilityTask*>& UAblAbility::GetTasks() const 
{
	return m_Tasks;
}

#if WITH_EDITOR

void UAblAbility::SanitizeTasks()
{
	bool hasInvalidTasks = m_Tasks.ContainsByPredicate([](const UAblAbilityTask* LHS)
	{
		return LHS == nullptr;
	});

	if (hasInvalidTasks)
	{
		m_Tasks.RemoveAll([](const UAblAbilityTask* LHS){ return LHS == nullptr; });
	}
}

void UAblAbility::CopyInheritedTasks(const UAblAbility& ParentObject)
{
	m_Tasks.Empty();

	const TArray<UAblAbilityTask*>& ParentTasks = ParentObject.GetTasks();
	for (UAblAbilityTask* ParentTask : ParentTasks)
	{
		if (ParentTask && ParentTask->IsInheritable())
		{
			UAblAbilityTask* taskCopy = NewObject<UAblAbilityTask>(Cast<UObject>(this), ParentTask->GetClass(), NAME_None, RF_Public | RF_Transactional, Cast<UObject>(ParentTask));
			if (!taskCopy)
			{
				UE_LOG(LogAble, Warning, TEXT("Failed to copy Parent Task [%s] from Parent[%s], it will not be available in the Child Ability [%s]"),
					*ParentTask->GetTaskName().ToString(),
					*ParentObject.GetDisplayName(),
					*GetDisplayName());

				continue;
			}
			m_Tasks.Add(taskCopy);
			MarkPackageDirty();
		}
	}
}

bool UAblAbility::FixUpObjectFlags()
{
	bool modified = false;

	for (UAblChannelingBase* ChannelCond : m_ChannelConditions)
	{
		if (!ChannelCond)
		{
			continue;
		}

		modified |= ChannelCond->FixUpObjectFlags();
	}

	for (UAblAbilityTask* Task : m_Tasks)
	{
		if (!Task)
		{
			continue;
		}

		modified |= Task->FixUpObjectFlags();
	}

	return modified;
}

void UAblAbility::AddTask(UAblAbilityTask& Task)
{
	m_Tasks.Add(&Task);
}

void UAblAbility::RemoveTask(UAblAbilityTask& Task)
{
	m_Tasks.Remove(&Task);

	ValidateDependencies();
}

void UAblAbility::RemoveTask(const UAblAbilityTask& Task)
{
	for (int i = 0; i < m_Tasks.Num(); ++i)
	{
		if (m_Tasks[i] == &Task)
		{
			m_Tasks.RemoveAt(i);
			break;
		}
	}

	ValidateDependencies();
}

void UAblAbility::RemoveTaskAtIndex(int32 Index)
{
	m_Tasks.RemoveAt(Index);

	ValidateDependencies();
}

void UAblAbility::SortTasks()
{
	if (m_Tasks.Num() <= 1)
	{
		return;
	}

	bool requiresSort = false;
	for (int i = 0; i < m_Tasks.Num() - 1; ++i)
	{
		if (m_Tasks[i]->GetStartTime() > m_Tasks[i + 1]->GetStartTime())
		{
			requiresSort = true;
			break;
		}
	}

	if (!requiresSort)
	{
		return;
	}

	m_Tasks.Sort([](const UAblAbilityTask& A, const UAblAbilityTask& B)
	{
		return A.GetStartTime() < B.GetStartTime();
	});
}

void UAblAbility::ValidateDependencies()
{
	for (UAblAbilityTask* Task : m_Tasks)
	{
		TArray<const UAblAbilityTask*>& Dependencies = Task->GetMutableTaskDependencies();
		for (int i = 0; i < Dependencies.Num();)
		{
			const UAblAbilityTask* CurrentDependency = Dependencies[i];
			// Task no longer exists in our Ability, remove it. 
			if (!m_Tasks.Contains(CurrentDependency))
			{
				// See if we have a replacement.
				UAblAbilityTask** replacementTask = m_Tasks.FindByPredicate([&](const UAblAbilityTask* LHS) { 
					return LHS->GetClass()->GetFName() == CurrentDependency->GetClass()->GetFName() &&
					FMath::IsNearlyEqual(LHS->GetStartTime(), CurrentDependency->GetStartTime()) &&
					FMath::IsNearlyEqual(LHS->GetEndTime(), CurrentDependency->GetEndTime()); 
				});

				if (replacementTask && *replacementTask)
				{
					Dependencies[i] = *replacementTask;
					++i;
				}
				else
				{
					Dependencies.RemoveAt(i, 1, false);
				}
				m_DependenciesDirty = true;
			}
			else
			{
				++i;
			}
		}
		Dependencies.Shrink();
	}
}


void UAblAbility::MarkTasksAsPublic()
{
	for (int32 i = 0; i < m_Tasks.Num(); ++i)
	{
		m_Tasks[i]->SetFlags(m_Tasks[i]->GetFlags() | RF_Public);
		m_Tasks[i]->MarkPackageDirty();
	}
}

void UAblAbility::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Loop End can never be past our length.
	m_LoopEnd = FMath::Min(m_LoopEnd, m_Length);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAblAbility, m_Tasks))
	{
		// Our Tasks have changed, rebuild dependencies.
		m_DependenciesDirty = true;
		ValidateDependencies();
		BuildDependencyList();
	}

	if (!m_AbilityNameHash)
	{
		m_AbilityNameHash = FCrc::StrCrc32(*GetName());
	}

}

void UAblAbility::OnReferencedTaskPropertyModified(UAblAbilityTask& Task, struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == FName(TEXT("m_Dependencies")))
	{
		// Our Task changed dependencies. Validate/Rebuild.
		m_DependenciesDirty = true;
		ValidateDependencies();
		BuildDependencyList();
	}

	if (!m_AbilityNameHash)
	{
		m_AbilityNameHash = FCrc::StrCrc32(*GetName());
	}

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == FName(TEXT("m_DynamicPropertyIdentifer")))
	{
		if (RequestEditorRefresh.IsBound())
		{
			RequestEditorRefresh.Broadcast();
		}
    }

	BindDynamicProperties();
}

bool UAblAbility::HasInvalidTasks() const
{
	for (const UAblAbilityTask* Task : m_Tasks)
	{
		if (!Task || Task->GetClass()->HasAnyClassFlags(CLASS_NewerVersionExists | CLASS_Deprecated))
		{
			return true;
		}
	}

	return false;
}

bool UAblAbility::NeedsCompactData() const
{
	const UAblAbilityTask* const * CompactDataTask = m_Tasks.FindByPredicate([](const UAblAbilityTask* task) { return task && task->HasCompactData(); });
	
	if (CompactData.Num() == 0 && (CompactDataTask && *CompactDataTask != nullptr))
	{
		return true;
	}

	return false;
}

void UAblAbility::SaveCompactData()
{
	CompactData.Empty();

	for (UAblAbilityTask* Task : m_Tasks)
	{
		if (Task && Task->HasCompactData())
		{
			Task->GetCompactData(CompactData.AddDefaulted_GetRef());
		}
	}
}

void UAblAbility::LoadCompactData()
{
	// Remove all compact data Tasks.
	m_Tasks.RemoveAll([](const UAblAbilityTask* LHS) { return !LHS || LHS->HasCompactData(); });

	// Reconstruct our Tasks that we removed.
	for (FAblCompactTaskData& data : CompactData)
	{
		if (data.TaskClass.IsValid())
		{
			UAblAbilityTask* NewTask = NewObject<UAblAbilityTask>(this, data.TaskClass.Get(), NAME_None, GetMaskedFlags(RF_PropagateToSubObjects) | RF_Transactional);
			if (!NewTask)
			{
				UE_LOG(LogAble, Warning, TEXT("Failed to create task from compact data using class %s\n"), *data.TaskClass.ToString());
				continue;
			}
			NewTask->LoadCompactData(data);
			m_Tasks.Add(NewTask);
		}
		else
		{
			UE_LOG(LogAble, Warning, TEXT("Task had invalid class data. Please re-add it manually. \n"));
		}
	}

	// Sort
	SortTasks();

	// Fix up dependencies
	ValidateDependencies();

	if (m_DependenciesDirty)
	{
		// Build Dependencies.
		BuildDependencyList();
	}
}

EDataValidationResult UAblAbility::IsDataValid(TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    const FText AssetName = FText::FromString(GetDisplayName());

    for (const UAblChannelingBase* Filter : m_ChannelConditions)
    {
        if (Filter == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullCollisionFilter", "Null Collision Filter: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }

    for (UAblAbilityTask* Task : m_Tasks)
    {
        if (Task == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullTask", "Null Task: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }

        if (Task->IsTaskDataValid(this, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
            result = EDataValidationResult::Invalid;
    }

	// This is tossing false positives somehow... Loading order issue with others BPs perhaps?
    //for (const UAblAbilityTask* Task : m_AllDependentTasks)
    //{
    //    if (Task == nullptr)
    //    {
    //        ValidationErrors.Add(FText::Format(LOCTEXT("NullDependency", "Null Dependency: {0}"), AssetName));
    //        result = EDataValidationResult::Invalid;
    //    }
    //}

    return result;
}

void UAblAbility::ValidateDataAgainstTemplate(const UAblAbility* Template)
{
	// Only allowed on Transient objects.
	if (!Template || !(GetFlags() & RF_Transient))
	{
		return;
	}

	// Check Tasks.
	const TArray<UAblAbilityTask*>& SourceTasks = Template->GetTasks();
	for (int32 i = 0; i < m_Tasks.Num(); ++i)
	{
		if (m_Tasks[i] == nullptr && SourceTasks[i] != nullptr)
		{
			m_Tasks[i] = SourceTasks[i];
		}
	}

	// Check Channel Conditions.
	const TArray<UAblChannelingBase*>& SourceConditions = Template->GetChannelConditions();
	for (int32 i = 0; i < m_ChannelConditions.Num(); ++i)
	{
		if (m_ChannelConditions[i] == nullptr && SourceConditions[i] != nullptr)
		{
			m_ChannelConditions[i] = SourceConditions[i];
		}
	}
}

#endif

void UAblAbilityRuntimeParametersScratchPad::SetIntParameter(FName Id, int Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_IntParameters.Add(Id, Value);
}

void UAblAbilityRuntimeParametersScratchPad::SetFloatParameter(FName Id, float Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_FloatParameters.Add(Id, Value);
}

void UAblAbilityRuntimeParametersScratchPad::SetStringParameter(FName Id, const FString& Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_StringParameters.Add(Id, Value);
}

void UAblAbilityRuntimeParametersScratchPad::SetUObjectParameter(FName Id, UObject* Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_UObjectParameters.Add(Id, Value);
}

int UAblAbilityRuntimeParametersScratchPad::GetIntParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (const int* var = m_IntParameters.Find(Id))
	{
		return *var;
	}
	return 0;
}

float UAblAbilityRuntimeParametersScratchPad::GetFloatParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (const float* var = m_FloatParameters.Find(Id))
	{
		return *var;
	}
	return 0.0f;
}

UObject* UAblAbilityRuntimeParametersScratchPad::GetUObjectParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (m_UObjectParameters.Contains(Id)) // Find is being weird with double ptr.
	{
		return m_UObjectParameters[Id];
	}
	return nullptr;
}

const FString& UAblAbilityRuntimeParametersScratchPad::GetStringParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (const FString* var = m_StringParameters.Find(Id))
	{
		return *var;
	}
	static FString EmptyString;
	return EmptyString;
}

void UAblAbilityRuntimeParametersScratchPad::ResetScratchpad()
{
	m_FloatParameters.Empty();
	m_IntParameters.Empty();
	m_StringParameters.Empty();
	m_UObjectParameters.Empty();
}

#undef LOCTEXT_NAMESPACE