// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ablAbilityComponent.h"

#include "ablAbility.h"
#include "ablAbilityInstance.h"
#include "ablAbilityUtilities.h"
#include "AbleCorePrivate.h"
#include "ablSettings.h"
#include "ablAbilityUtilities.h"
#include "Animation/AnimNode_AbilityAnimPlayer.h"

#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeLock.h"

#if WITH_EDITOR
#include "FXSystem.h"
#endif

#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "AbleCore"

FAblAbilityCooldown::FAblAbilityCooldown()
	: Ability(nullptr),
	Context(nullptr),
	CurrentTime(0.0f),
	CooldownTime(1.0f)
{

}
FAblAbilityCooldown::FAblAbilityCooldown(const UAblAbility& InAbility, const UAblAbilityContext& InContext)
	: Ability(nullptr),
	Context(nullptr),
	CurrentTime(0.0f),
	CooldownTime(1.0f)
{
	Ability = &InAbility;
	Context = &InContext;
	CooldownTime = Ability->GetCooldown(Context);
}

void FAblAbilityCooldown::Update(float DeltaTime)
{
	if (Ability && Context && !Ability->CanCacheCooldown())
	{
		CooldownTime = Ability->GetCooldown(Context);
	}

	CurrentTime += DeltaTime;
}

UAblAbilityComponent::UAblAbilityComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_ActiveAbilityInstance(),
    m_PassivesDirty(false),
	m_ClientPredictionKey(0),
	m_AbilityAnimationNode(nullptr),
	m_ServerPredictionKey(0)
{
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	SetIsReplicatedByDefault(true);

	m_Settings = GetDefault<UAbleSettings>(UAbleSettings::StaticClass());

	m_LocallyPredictedAbilities.Reserve(ABLE_ABILITY_PREDICTION_RING_SIZE);
	m_LocallyPredictedAbilities.AddDefaulted(ABLE_ABILITY_PREDICTION_RING_SIZE);
}

void UAblAbilityComponent::BeginPlay()
{
	m_TagContainer.AppendTags(m_AutoApplyTags);

	Super::BeginPlay();
}

void UAblAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (m_ActiveAbilityInstance.IsValid())
	{
		m_ActiveAbilityInstance.StopAbility();
	}
	m_ActiveAbilityInstance.Reset();
	m_ActiveAbilityResult = EAblAbilityTaskResult::Successful;
	
	for (FAblAbilityInstance& PassiveInstance : m_PassiveAbilityInstances)
	{
		if (PassiveInstance.IsValid())
		{
			PassiveInstance.StopAbility();
		}
	}
	m_PassiveAbilityInstances.Empty();

	Super::EndPlay(EndPlayReason);
}

void UAblAbilityComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AblAbilityComponent::TickComponent"), STAT_AblAbilityComponent_TickComponent, STATGROUP_Able);

	// Do our cooldowns first (if we're Async this will give it a bit of time to go ahead and start running).
	if (m_ActiveCooldowns.Num() > 0)
	{
		// Go ahead and fire off our cooldown update first.
		if (UAbleSettings::IsAsyncEnabled() && m_Settings->GetAllowAsyncCooldownUpdate())
		{
			TGraphTask<FAsyncAbilityCooldownUpdaterTask>::CreateTask().ConstructAndDispatchWhenReady(this, DeltaTime);
		}
		else
		{
			UpdateCooldowns(DeltaTime);
		}
	}

	bool ActiveChanged = false;
	bool PassivesChanged = false;

	// Check the status of our Active, we only do this on our authoritative client, or if we're locally controlled for local simulation purposes.
	if (m_ActiveAbilityInstance.IsValid() && 
		(IsAuthoritative() || IsOwnerLocallyControlled()))
	{
		// Have to turn this on to prevent Users from apparently canceling the Ability that is finishing... (Seems like an extreme case, but whatever).
		m_IsProcessingUpdate = true;

		// Check for done...
		if (m_ActiveAbilityInstance.IsIterationDone())
        {
			if (m_ActiveAbilityInstance.IsDone())
			{
				if (m_Settings->GetLogVerbose())
				{
					UE_LOG(LogAble, Warning, TEXT("[%s] TickComponent [%s] IsDone, Queueing cancel."),
						*FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
						*m_ActiveAbilityInstance.GetAbility().GetDisplayName());
				}

				m_PendingCancels.Add(FAblPendingCancelContext(m_ActiveAbilityInstance.GetAbilityNameHash(), EAblAbilityTaskResult::Successful));

				ActiveChanged = true;
			}
			else
			{
				m_ActiveAbilityInstance.ResetForNextIteration();
			}
		}
		
		// Check for Channeling...
		if (!ActiveChanged && m_ActiveAbilityInstance.IsChanneled())
		{
			if (m_ActiveAbilityInstance.CheckChannelConditions() == EAblConditionResults::ACR_Failed)
			{
				m_ActiveAbilityResult = m_ActiveAbilityInstance.GetChannelFailureResult();

                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAble, Warning, TEXT("[%s] TickComponent [%s] Channel Condition Failure [%s], Queueing cancel."),
                        *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(m_ActiveAbilityInstance.GetAbility().GetDisplayName()),
                        *FAbleLogHelper::GetTaskResultEnumAsString(m_ActiveAbilityResult));
                }

				if (!IsAuthoritative() && m_ActiveAbilityInstance.RequiresServerNotificationOfChannelFailure())
				{
					ServerCancelAbility(m_ActiveAbilityInstance.GetAbilityNameHash(), m_ActiveAbilityInstance.GetChannelFailureResult());
				}

				m_PendingCancels.Add(FAblPendingCancelContext(m_ActiveAbilityInstance.GetAbilityNameHash(), m_ActiveAbilityInstance.GetChannelFailureResult()));

				ActiveChanged = true;
			}
		}

		m_IsProcessingUpdate = false;
	}
	int32 PrePendingPassiveCount = m_PassiveAbilityInstances.Num();

	// Handle any Pending cancels.
	HandlePendingCancels();

#if WITH_EDITOR
	if (ActiveChanged && m_ActiveAbilityInstance.IsValid())
	{
		UE_LOG(LogAble, Warning, TEXT("Killed Active Ability manually after it failed to cancel."));
		m_ActiveAbilityInstance.Reset();
	}
#endif

	bool ActiveWasValid = m_ActiveAbilityInstance.IsValid();

	// Handle any Pending abilities.
	HandlePendingContexts();

	ActiveChanged |= ActiveWasValid != m_ActiveAbilityInstance.IsValid();
	PassivesChanged |= m_PassivesDirty;

	// Try and process our Async targeting queue..
	TArray<UAblAbilityContext*> RemovalList;
	for (TArray<UAblAbilityContext*>::TIterator ItAsync = m_AsyncContexts.CreateIterator(); ItAsync; ++ItAsync)
	{
		EAblAbilityStartResult StartResult = InternalStartAbility((*ItAsync));
		if (StartResult != EAblAbilityStartResult::AsyncProcessing)
		{
			// We got some result (either pass or fail), remove this from further processing.
			RemovalList.Add(*ItAsync);

			if ((*ItAsync)->GetAbility()->IsPassive())
			{
				PassivesChanged = true;
			}
			else
			{
				ActiveChanged = true;
			}
		}
	}

	for (UAblAbilityContext* ToRemove : RemovalList)
	{
		m_AsyncContexts.Remove(ToRemove);
	}

	m_IsProcessingUpdate = true;
    
	// Update our Active
	if (m_ActiveAbilityInstance.IsValid())
	{
		// Process update (or launch a task to do it).
		InternalUpdateAbility(&m_ActiveAbilityInstance, DeltaTime * m_ActiveAbilityInstance.GetPlayRate());
	}

	// Update Passives
	FAblAbilityInstance* Passive = nullptr;
	for (int i = 0; i < m_PassiveAbilityInstances.Num(); ++i)
	{
		Passive = &m_PassiveAbilityInstances[i];
		if (Passive->IsValid())
		{
            bool PassiveFinished = false;

			if (Passive->IsChanneled() && (IsAuthoritative() || IsOwnerLocallyControlled()))
			{
				if (Passive->CheckChannelConditions() == EAblConditionResults::ACR_Failed)
				{
					UE_LOG(LogAble, Warning, TEXT("[%s] TickComponent [%s] Channel Condition Failure [%s]"),
						*FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
						*(Passive->GetAbility().GetDisplayName()),
						*FAbleLogHelper::GetTaskResultEnumAsString(Passive->GetChannelFailureResult()));

					CancelAbility(&Passive->GetAbility(), Passive->GetChannelFailureResult());

					continue;
				}
			}

			if (Passive->IsIterationDone() && (IsAuthoritative() || IsOwnerLocallyControlled()))
			{
				if (Passive->IsDone())
				{
					if (m_Settings->GetLogVerbose())
					{
						UE_LOG(LogAble, Warning, TEXT("[%s] TickComponent [%s] Passive IsDone"),
							*FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
							*Passive->GetAbility().GetDisplayName());
					}

					PassiveFinished = true;
					Passive->FinishAbility();
				}
				else
				{
					Passive->ResetForNextIteration();
				}

			}
			else
			{
				InternalUpdateAbility(Passive, DeltaTime * Passive->GetPlayRate());

				if (Passive->IsIterationDone())
				{
					if (Passive->IsDone())
					{
						if (m_Settings->GetLogVerbose())
						{
							UE_LOG(LogAble, Warning, TEXT("[%s] TickComponent [%s] Passive Update->IsDone"),
								*FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
								*Passive->GetAbility().GetDisplayName());
						}

						PassiveFinished = true;
						Passive->FinishAbility();
					}
					else
					{
						Passive->ResetForNextIteration();
					}
				}

			}

            // Check for stack decay
            if (!PassiveFinished && (IsAuthoritative() || IsOwnerLocallyControlled()))
            {
                int32 decayedStacks = Passive->CheckForDecay(this);
                if (decayedStacks > 0)
                {
                    if (m_Settings->GetLogVerbose())
                    {
                        UE_LOG(LogAble, Warning, TEXT("[%s] TickComponent [%s] Passive Ability Decayed [%d] stacks"),
                            *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                            *(Passive->GetAbility().GetDisplayName()),
                            decayedStacks);
                    }
                }
            }
		}
	}

	// Clean up finished passives.
	const int32 removed = m_PassiveAbilityInstances.RemoveAll([](const FAblAbilityInstance& Instance)->bool
	{
		return !Instance.IsValid() || (Instance.IsIterationDone() && Instance.IsDone());
	});
    PassivesChanged |= removed > 0;

	if (IsNetworked() && IsAuthoritative())
	{
		// Make sure we keep our client watched fields in sync.
		if (ActiveChanged)
		{
			UpdateServerActiveAbility();
		}

		if (PassivesChanged)
		{
			UpdateServerPassiveAbilities();
            m_PassivesDirty = false;
		}
	}

	CheckNeedsTick();

	// We've finished our update, validate things for remote clients - any Abilities that need to restart will begin next frame.
	ValidateRemoteRunningAbilities();

	m_IsProcessingUpdate = false;
}

void UAblAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// These two fields are replicated and watched by the client.
	DOREPLIFETIME(UAblAbilityComponent, m_ServerActive);
	DOREPLIFETIME(UAblAbilityComponent, m_ServerPassiveAbilities);
	DOREPLIFETIME(UAblAbilityComponent, m_ServerPredictionKey);
}

void UAblAbilityComponent::BeginDestroy()
{
	Super::BeginDestroy();
}

bool UAblAbilityComponent::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (UAblAbility* Ability : m_CreatedAbilityInstances)
	{
		if (Ability && IsValid(Ability))
		{
			WroteSomething |= Channel->ReplicateSubobject(Ability, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

bool UAblAbilityComponent::IsNetworked() const
{
	return GetNetMode() != ENetMode::NM_Standalone && GetOwnerRole() != ROLE_SimulatedProxy;
}

EAblAbilityStartResult UAblAbilityComponent::CanActivateAbility(UAblAbilityContext* Context) const
{
	if (!Context)
	{
		UE_LOG(LogAble, Warning, TEXT("Null Context passed CanActivateAbility method."));
		return EAblAbilityStartResult::InternalSystemsError;
	}
	
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AblAbilityComponent::CanActivateAbility"), STAT_AblAbilityComponent_CanActivateAbility, STATGROUP_Able);
	
	if (const UAblAbility* Ability = Context->GetAbility())
	{
		if (Ability->IsPassive())
		{
			return CanActivatePassiveAbility(Context);
		}

		if (IsAbilityOnCooldown(Ability))
		{
			return EAblAbilityStartResult::CooldownNotExpired;
		}

		if (m_ActiveAbilityInstance.IsValid())
		{
			// We're already playing an active ability, check if interrupt is allowed.
			if (!Ability->CanInterruptAbilityBP(Context, &m_ActiveAbilityInstance.GetAbility()))
			{
				return EAblAbilityStartResult::CannotInterruptCurrentAbility;
			}

			if (m_Settings->GetLogVerbose())
			{
				UE_LOG(LogAble, Error, TEXT("[%s] CanActivateAbility Ability [%s] allowed to interrupt ability [%s]"),
					*FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
					*(Context->GetAbility()->GetDisplayName()),
					*m_ActiveAbilityInstance.GetAbility().GetDisplayName());
			}
		}

		return Ability->CanAbilityExecute(*Context);
	}

	// Our Ability wasn't valid. 
	return EAblAbilityStartResult::InternalSystemsError;
}

EAblAbilityStartResult UAblAbilityComponent::ActivateAbility(UAblAbilityContext* Context)
{
	EAblAbilityStartResult Result = EAblAbilityStartResult::InternalSystemsError;
	if (IsNetworked())
	{
		if (!IsAuthoritative())
		{
			// Pass it to the server to validate/execute.
			FAblAbilityNetworkContext AbilityNetworkContext(*Context);
			if (m_Settings->GetAlwaysForwardToServerFirst())
			{
				ServerActivateAbility(AbilityNetworkContext);
			}

			// If we're a locally controlled player...
			if (IsOwnerLocallyControlled())
			{
				if (IsProcessingUpdate()) // We're in the middle of an update, set it as the pending context so our Ability has a chance to finish.
				{
					// Track it for prediction.
					AddLocallyPredictedAbility(AbilityNetworkContext);

					QueueContext(Context, EAblAbilityTaskResult::Successful);

					Result = EAblAbilityStartResult::Success;
				}
				else if (Context->GetAbility()->GetInstancePolicy() != EAblInstancePolicy::NewInstanceReplicated)
				{
					// Go ahead and attempt to play the ability locally, the server is authoritative still. Worst case you end up playing an ability but
					// the server rejects it for whatever reason. That's still far better than having a delay to all actions.
					Result = InternalStartAbility(Context);

					if (!m_Settings->GetAlwaysForwardToServerFirst())
					{
						if (Result == EAblAbilityStartResult::Success)
						{
							ServerActivateAbility(AbilityNetworkContext);
							// Track it for prediction.
							AddLocallyPredictedAbility(AbilityNetworkContext);
						}
					}
					else
					{
						// Track it for prediction.
						AddLocallyPredictedAbility(AbilityNetworkContext);
					}
				}

			}
			else
			{
				Result = EAblAbilityStartResult::ForwardedToServer;
			}
		}
		else
		{
			if (IsProcessingUpdate()) // We're in the middle of an update, set it as the pending context so our Ability has a chance to finish.
			{
				QueueContext(Context, EAblAbilityTaskResult::Successful);

				Result = EAblAbilityStartResult::Success;
			}
			else
			{
				// We're authoritative. Start the Ability.
				Result = InternalStartAbility(Context);
			}
		}
	}
	else
	{
		if (IsProcessingUpdate()) // We're in the middle of an update, set it as the pending context so our Ability has a chance to finish.
		{
			QueueContext(Context, EAblAbilityTaskResult::Successful);

			Result = EAblAbilityStartResult::Success;
		}
		else
		{
			// Local game, just pass it along.
			Result = InternalStartAbility(Context);
		}
	}

	CheckNeedsTick();

	return Result;
}

bool UAblAbilityComponent::HasAbilityAnimation() const
{
	return m_AbilityAnimationNode && m_AbilityAnimationNode->HasAnimationToPlay();
}

void UAblAbilityComponent::CancelActiveAbility(EAblAbilityTaskResult ResultToUse)
{
	if (m_ActiveAbilityInstance.IsValid())
	{
		// Network forwarding should already be handled by this point by CancelAbility.
		if (IsAuthoritative() || IsOwnerLocallyControlled())
		{
			if (IsProcessingUpdate())
			{
				m_PendingCancels.Add(FAblPendingCancelContext(m_ActiveAbilityInstance.GetAbilityNameHash(), ResultToUse));
				return;
			}

			switch (ResultToUse)
			{
			case EAblAbilityTaskResult::Branched:
				m_ActiveAbilityInstance.BranchAbility();
				break;
			case EAblAbilityTaskResult::Interrupted:
				m_ActiveAbilityInstance.InterruptAbility();
				break;
			default:
			case EAblAbilityTaskResult::Successful:
				m_ActiveAbilityInstance.FinishAbility();
				break;
			}
		}

        if (m_Settings->GetLogVerbose())
        {
            UE_LOG(LogAble, Warning, TEXT("[%s] CancelActiveAbility( %s ) Result [%s]"),
                *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *m_ActiveAbilityInstance.GetAbility().GetDisplayName(),
                *FAbleLogHelper::GetTaskResultEnumAsString(ResultToUse));
        }
		m_ActiveAbilityInstance.Reset();
		m_ActiveAbilityResult = ResultToUse;
	}

	if (IsNetworked() && IsAuthoritative())
	{
		// Tell the client to reset as well.
		UpdateServerActiveAbility();
	}
}

const UAblAbility* UAblAbilityComponent::GetConstActiveAbility() const
{
	if (m_ActiveAbilityInstance.IsValid())
	{
		const UAblAbility& CurrentAbility = m_ActiveAbilityInstance.GetAbility();
		return &CurrentAbility;
	}

	return nullptr;
}

UAblAbility* UAblAbilityComponent::GetActiveAbility() const
{
	return const_cast<UAblAbility*>(GetConstActiveAbility());
}

void UAblAbilityComponent::ExecuteCustomEventForActiveAbility(const UAblAbility* Ability, const FName& EventName)
{
    if (m_ActiveAbilityInstance.IsValid())
    {
        const UAblAbilityContext& CurrentAbilityContext = m_ActiveAbilityInstance.GetContext();
        const UAblAbility& CurrentAbility = m_ActiveAbilityInstance.GetAbility();
        if (Ability == &CurrentAbility)
        {
            return CurrentAbility.OnCustomEvent(&CurrentAbilityContext, EventName);
        }
    }
}

EAblAbilityStartResult UAblAbilityComponent::CanActivatePassiveAbility(UAblAbilityContext* Context) const
{
	if (!Context)
	{
		UE_LOG(LogAble, Warning, TEXT("Null Context passed to CanActivatePassiveAbility method."));
		return EAblAbilityStartResult::InternalSystemsError;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AblAbilityComponent::CanActivatePassiveAbility"), STAT_AblAbilityComponent_CanActivatePassiveAbility, STATGROUP_Able);

	EAblAbilityStartResult Result = EAblAbilityStartResult::InternalSystemsError;

	if (const UAblAbility* Ability = Context->GetAbility())
	{
		if (IsAbilityOnCooldown(Ability))
		{
			return EAblAbilityStartResult::CooldownNotExpired;
		}

		if (!Ability->IsPassive())
		{
			return EAblAbilityStartResult::NotAllowedAsPassive;
		}

		// Do our stack count first, since it should be pretty cheap.
		int32 CurrentStackCount = GetCurrentStackCountForPassiveAbility(Ability);
		if (CurrentStackCount == 0 || CurrentStackCount < Ability->GetMaxStacks(Context))
		{
			return Ability->CanAbilityExecute(*Context);
		}
		else
		{
			return EAblAbilityStartResult::PassiveMaxStacksReached;
		}
	}

	return Result;
}

EAblAbilityStartResult UAblAbilityComponent::ActivatePassiveAbility(UAblAbilityContext* Context, bool ServerActivatedAbility)
{
	if (!Context)
	{
		UE_LOG(LogAble, Warning, TEXT("Null Context passed to ActivatePassiveAbility method."));
		return EAblAbilityStartResult::InternalSystemsError;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AblAbilityComponent::ActivatePassiveAbility"), STAT_AblAbilityComponent_ActivatePassiveAbility, STATGROUP_Able);

	EAblAbilityStartResult Result = ServerActivatedAbility ? EAblAbilityStartResult::Success : CanActivatePassiveAbility(Context);

	const UAblAbility* Ability = Context->GetAbility();

	if (Result == EAblAbilityStartResult::AsyncProcessing)
	{
        if (m_Settings->GetLogVerbose())
        {
            UE_LOG(LogAble, Warning, TEXT("[%s] ActivatePassiveAbility [%s] queued for async"),
                *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *(Context->GetAbility()->GetDisplayName()));
        }

		// Save and process it later once the Async query is done.
		m_AsyncContexts.AddUnique(Context);
		return Result;
	}
	else if (Result == EAblAbilityStartResult::PassiveMaxStacksReached)
	{
		if (Ability->AlwaysRefreshDuration())
		{
			FAblAbilityInstance* FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
			if (FoundPassive)
			{
				FoundPassive->ResetTime(Ability->RefreshLoopTimeOnly());

				if (Ability->ResetLoopCountOnRefresh())
				{
					FoundPassive->SetCurrentIteration(0);
				}

                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAble, Warning, TEXT("[%s] ActivatePassiveAbility [%s] reset time"),
                        *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(Context->GetAbility()->GetDisplayName()));
                }

				return EAblAbilityStartResult::Success;
			}
		}
        else
        {
            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAble, Warning, TEXT("[%s] ActivatePassiveAbility [%s] max stacks reached"),
                    *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetDisplayName()));
            }
        }
	}
	else if (Result == EAblAbilityStartResult::Success)
	{
		FAblAbilityInstance* FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
		if (FoundPassive)
		{
			UAblAbilityContext& MutableContext = FoundPassive->GetMutableContext();
			int32 StackIncrement = FMath::Max(Ability->GetStackIncrement(Context), 1);
			MutableContext.SetStackCount(MutableContext.GetCurrentStackCount() + StackIncrement);
			Ability->OnAbilityStackAddedBP(&MutableContext);

			if (Ability->RefreshDurationOnNewStack())
			{
				FoundPassive->ResetTime(Ability->RefreshLoopTimeOnly());
			}

			if (Ability->ResetLoopCountOnRefresh())
			{
				FoundPassive->SetCurrentIteration(0);
			}

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAble, Warning, TEXT("[%s] ActivatePassiveAbility [%s] stack added stack count %d (Increment %d)"),
                    *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetDisplayName()),
                    MutableContext.GetCurrentStackCount(),
					StackIncrement);
            }
		}
		else
		{
			// New Instance
			// We've passed all our checks, go ahead and allocate our Task scratch pads.
			Context->AllocateScratchPads();

			FAblAbilityInstance& NewInstance = m_PassiveAbilityInstances.AddDefaulted_GetRef();
			NewInstance.Initialize(*Context);
			
			// make sure the ability knows a stack was added and *don't* use the OnStart to duplicate the stack added behavior
			Ability->OnAbilityStackAddedBP(Context);

			// Go ahead and start our cooldown.
			AddCooldownForAbility(*Ability, *Context);

            m_PassivesDirty |= true;

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAble, Warning, TEXT("[%s] ActivatePassiveAbility [%s] Started"),
                    *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetDisplayName()));
            }
		}
	}
	else
	{
		if (m_Settings->GetLogAbilityFailures())
		{
			UE_LOG(LogAble, Error, TEXT("[%s] Failed to play Passive Ability [%s] due to reason [%s]."), 
                *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *(Context->GetAbility()->GetDisplayName()), 
                *FAbleLogHelper::GetResultEnumAsString(Result));
		}

		return Result;
	}

	CheckNeedsTick();

	return EAblAbilityStartResult::Success;
}


void UAblAbilityComponent::CancelAbility(const UAblAbility* Ability, EAblAbilityTaskResult ResultToUse)
{
	if (!Ability)
	{
		UE_LOG(LogAble, Warning, TEXT("Null Ability passed to CancelAbility method."));
		return;
	}

	if (!IsAuthoritative())
	{
		ServerCancelAbility(Ability->GetAbilityNameHash(), ResultToUse);

		// Fall through and Locally simulate the cancel if we're player controlled.
		if (!IsOwnerLocallyControlled())
		{
			return;
		}
	}

	InternalCancelAbility(Ability, ResultToUse);
}

int32 UAblAbilityComponent::GetCurrentStackCountForPassiveAbility(const UAblAbility* Ability) const
{
	if (!Ability)
	{
		return 0;
	}

	const FAblAbilityInstance* FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
	if (FoundPassive)
	{
		return FoundPassive->GetStackCount();
	}

	return 0;
}

void UAblAbilityComponent::GetCurrentPassiveAbilities(TArray<UAblAbility*>& OutPassives) const
{
	OutPassives.Empty();
	for (const FAblAbilityInstance& ActivePassive : m_PassiveAbilityInstances)
	{
		const UAblAbility& PassiveAbility = ActivePassive.GetAbility();
		// This is the devil, but BP doesn't like const pointers.
		OutPassives.Add(const_cast<UAblAbility*>(&PassiveAbility));
	}
}

void UAblAbilityComponent::SetPassiveStackCount(const UAblAbility* Ability, int32 NewStackCount, bool ResetDuration, EAblAbilityTaskResult ResultToUseOnCancel)
{
	if (!Ability)
	{
		return;
	}

	FAblAbilityInstance* FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
	if (FoundPassive)
	{
		if (NewStackCount == 0)
		{
			// make sure to notify of "stack removal" first
			FoundPassive->GetAbility().OnAbilityStackRemovedBP(&FoundPassive->GetContext());

			// Just cancel the Passive.
			CancelAbility(Ability, ResultToUseOnCancel);
		}
		else
		{
			int32 current = FoundPassive->GetStackCount();
			FoundPassive->SetStackCount(NewStackCount);

			if (current > NewStackCount)
			{
				FoundPassive->GetAbility().OnAbilityStackRemovedBP(&FoundPassive->GetContext());
			}
			else
			{
				FoundPassive->GetAbility().OnAbilityStackAddedBP(&FoundPassive->GetContext());
			}

			if (ResetDuration)
			{
				FoundPassive->ResetTime(Ability->RefreshLoopTimeOnly());
			}

			if (Ability->ResetLoopCountOnRefresh())
			{
				FoundPassive->SetCurrentIteration(0);
			}
		}
	}

}

float UAblAbilityComponent::GetAbilityCooldownRatio(const UAblAbility* Ability) const
{
	if (Ability)
	{
		if (const FAblAbilityCooldown* FoundCooldown = m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()))
		{
			return FoundCooldown->getTimeRatio();
		}
	}

	return 0.0f;
}

float UAblAbilityComponent::GetAbilityCooldownTotal(const UAblAbility* Ability) const
{
	if (Ability)
	{
		if (const FAblAbilityCooldown* FoundCooldown = m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()))
		{
			return FoundCooldown->GetCooldownTime();
		}
	}

	return 0.0f;
}

void UAblAbilityComponent::RemoveCooldown(const UAblAbility* Ability)
{
	if (Ability)
	{
		m_ActiveCooldowns.Remove(Ability->GetAbilityNameHash());
	}
}

void UAblAbilityComponent::SetCooldown(const UAblAbility* Ability, UAblAbilityContext* Context, float time)
{
	if (Ability)
	{
		if (time <= 0.0f)
		{
			RemoveCooldown(Ability);
		}

		if (FAblAbilityCooldown* cooldown = m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()))
		{
			cooldown->SetCooldownTime(time);
		}
		else if (Context)
		{
			FAblAbilityCooldown newCooldown(*Ability, *Context);
			m_ActiveCooldowns.Add(Ability->GetAbilityNameHash(), newCooldown);
		}
		else
		{
			UE_LOG(LogAble, Warning, TEXT("Unable to find Cooldown for Ability [%s] and no Context was provided so one could not be added."), *Ability->GetDisplayName());
		}
	}
}

float UAblAbilityComponent::GetAbilityCurrentTime(const UAblAbility* Ability) const
{
	if (Ability)
	{
		if (Ability->IsPassive())
		{
			FAblAbilityInstance const * FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
			if (FoundPassive)
			{
				return FoundPassive->GetCurrentTime();
			}
		}
		else if (m_ActiveAbilityInstance.IsValid() && m_ActiveAbilityInstance.GetAbilityNameHash() == Ability->GetAbilityNameHash())
		{
			return m_ActiveAbilityInstance.GetCurrentTime();
		}
	}

	return 0.0f;
}

float UAblAbilityComponent::GetAbilityCurrentTimeRatio(const UAblAbility* Ability) const
{
	if (Ability)
	{
		if (Ability->IsPassive())
		{
			FAblAbilityInstance const * FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
			if (FoundPassive)
			{
				return FoundPassive->GetCurrentTimeRatio();
			}
		}
		else if (m_ActiveAbilityInstance.IsValid() && m_ActiveAbilityInstance.GetAbilityNameHash() == Ability->GetAbilityNameHash())
		{
			return m_ActiveAbilityInstance.GetCurrentTimeRatio();
		}
	}

	return 0.0f;
}

void UAblAbilityComponent::QueueContext(UAblAbilityContext* Context, EAblAbilityTaskResult ResultToUse)
{
	if (IsAuthoritative() || IsOwnerLocallyControlled())
	{
		m_PendingContext.Add(Context); 
		m_PendingResult.Add(ResultToUse);

		CheckNeedsTick();
	}
}

void UAblAbilityComponent::AddAdditionTargetsToContext(const TWeakObjectPtr<const UAblAbilityContext>& Context, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool AllowDuplicates /*= false*/, bool ClearTargets/* = false*/)
{
	check(Context.IsValid());
	const uint32 AbilityNameHash = Context->GetAbility()->GetAbilityNameHash();
	if (!Context->GetAbility()->IsPassive())
	{
		if (m_ActiveAbilityInstance.IsValid())
		{
			if (m_ActiveAbilityInstance.GetAbilityNameHash() == AbilityNameHash)
			{
				m_ActiveAbilityInstance.AddAdditionalTargets(AdditionalTargets, AllowDuplicates, ClearTargets);
			}
		}
	}
	else
	{
		for (FAblAbilityInstance& Passive : m_PassiveAbilityInstances)
		{
			if (Passive.IsValid())
			{
				if (Passive.GetAbilityNameHash() == AbilityNameHash)
				{
					Passive.AddAdditionalTargets(AdditionalTargets, AllowDuplicates, ClearTargets);
					break;
				}
			}
		}
	}
}

void UAblAbilityComponent::ModifyContext(const TWeakObjectPtr<const UAblAbilityContext>& Context, AActor* Instigator, AActor* Owner, const FVector& TargetLocation, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool ClearTargets /*= false*/)
{
	check(Context.IsValid());
	const uint32 AbilityNameHash = Context->GetAbility()->GetAbilityNameHash();
	if (!Context->GetAbility()->IsPassive())
	{
		if (m_ActiveAbilityInstance.IsValid())
		{
			if (m_ActiveAbilityInstance.GetAbilityNameHash() == AbilityNameHash)
			{
				m_ActiveAbilityInstance.ModifyContext(Instigator, Owner, TargetLocation, AdditionalTargets, ClearTargets);
			}
		}
	}
	else
	{
		for (FAblAbilityInstance& Passive : m_PassiveAbilityInstances)
		{
			if (Passive.IsValid())
			{
				if (Passive.GetAbilityNameHash() == AbilityNameHash)
				{
					Passive.ModifyContext(Instigator, Owner, TargetLocation, AdditionalTargets, ClearTargets);
					break;
				}
			}
		}
	}
}

void UAblAbilityComponent::CheckNeedsTick()
{
	// We need to tick if we...
	bool NeedsTick = m_ActiveAbilityInstance.IsValid() || // Have an active ability...
        m_PassivesDirty || // Have pending dirty passives...
        m_PassiveAbilityInstances.Num() || // Have any passive abilities...
		m_ActiveCooldowns.Num() || // Have active cooldowns...
		m_AsyncContexts.Num() || // Have Async targeting to process...
		m_PendingContext.Num() || // We have a pending context...
		m_PendingCancels.Num();  // We have a pending cancel...

	PrimaryComponentTick.SetTickFunctionEnable(NeedsTick);
}

EAblAbilityStartResult UAblAbilityComponent::InternalStartAbility(UAblAbilityContext* Context, bool ServerActivatedAbility)
{
	if (!Context)
	{
		return EAblAbilityStartResult::InvalidParameter;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AblAbilityComponent::InternalStartAbility"), STAT_AblAbilityComponent_InternalStartAbility, STATGROUP_Able);
	
	EAblAbilityStartResult Result = EAblAbilityStartResult::InvalidParameter;
	if (Context->GetAbility())
	{
		HandleInstanceLogic(Context);

		// Hand off for Passive processing.
		if (Context->GetAbility()->IsPassive())
		{
			return ActivatePassiveAbility(Context, ServerActivatedAbility);
		}

		Result = ServerActivatedAbility ? EAblAbilityStartResult::Success : CanActivateAbility(Context);

		if (Result == EAblAbilityStartResult::CannotInterruptCurrentAbility && (m_ActiveAbilityInstance.IsIterationDone() && m_ActiveAbilityInstance.IsDone()))
		{
			CancelActiveAbility(EAblAbilityTaskResult::Successful);
			Result = CanActivateAbility(Context); // Try again.
		}

		if (Result == EAblAbilityStartResult::AsyncProcessing)
		{
			// Save and process it later once the Async query is done.
			m_AsyncContexts.AddUnique(Context);

			return Result;
		}
	}

	if (Result != EAblAbilityStartResult::Success)
	{
		if (m_Settings->GetLogAbilityFailures())
		{
			UE_LOG(LogAble, Error, TEXT("[%s] InternalStartAbility Failed to play Ability [%s] due to reason [%s]"),
                *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *(Context->GetAbility()->GetDisplayName()), 
                *FAbleLogHelper::GetResultEnumAsString(Result));
		}
		return Result;
	}

	// Interrupt any current abilities.
	if (m_ActiveAbilityInstance.IsValid())
	{
		if (!m_ActiveAbilityInstance.IsIterationDone())
		{
			if (IsProcessingUpdate())
			{
                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAble, Warning, TEXT("[%s] InternalStartAbility [%s] queued to interrupt [%s]"),
                        *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(Context->GetAbility()->GetDisplayName()),
                        *m_ActiveAbilityInstance.GetAbility().GetDisplayName());
                }

				// Delay processing till next frame.
				m_PendingCancels.Add(FAblPendingCancelContext(m_ActiveAbilityInstance.GetAbilityNameHash(), EAblAbilityTaskResult::Interrupted));
				return Result;
			}

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAble, Warning, TEXT("[%s] InternalStartAbility [%s] interrupted [%s]"),
                    *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetDisplayName()),
                    *m_ActiveAbilityInstance.GetAbility().GetDisplayName());
            }

			m_ActiveAbilityInstance.InterruptAbility();
		}
		else
		{
			if (IsProcessingUpdate())
			{
                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAble, Warning, TEXT("[%s] InternalStartAbility [%s] queued for next frame"),
                        *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(Context->GetAbility()->GetDisplayName()));
                }

				// Delay processing till next frame.
				QueueContext(Context, EAblAbilityTaskResult::Successful);
				return Result;
			}

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAble, Warning, TEXT("[%s] InternalStartAbility [%s] Ending Current Ability [%s]"),
                    *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetDisplayName()),
                    *m_ActiveAbilityInstance.GetAbility().GetDisplayName());
            }

			m_ActiveAbilityInstance.FinishAbility();
		}
		HandleInstanceCleanUp(m_ActiveAbilityInstance.GetAbility());
		m_ActiveAbilityInstance.Reset();
	}

    if (m_Settings->GetLogVerbose())
    {
        UE_LOG(LogAble, Warning, TEXT("[%s] InternalStartAbility [%s] Started"),
            *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
            *(Context->GetAbility()->GetDisplayName()));
    }

	// We've passed all our checks, go ahead and allocate our Task scratch pads.
	Context->AllocateScratchPads();

	m_ActiveAbilityInstance.Initialize(*Context);

	// Go ahead and start our cooldown.
	AddCooldownForAbility(*(Context->GetAbility()), *Context);

	if (IsNetworked() && IsAuthoritative())
	{
		// Propagate changes to client.
		UpdateServerActiveAbility();
	}

	CheckNeedsTick();

	return Result;
}

void UAblAbilityComponent::InternalCancelAbility(const UAblAbility* Ability, EAblAbilityTaskResult ResultToUse)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AblAbilityComponent::InternalCancelAbility"), STAT_AblAbilityComponent_InternalCancelAbility, STATGROUP_Able);

	if (Ability && IsProcessingUpdate())
	{
		// Mid update, let's defer this.
		m_PendingCancels.Add(FAblPendingCancelContext(Ability->GetAbilityNameHash(), ResultToUse));
		return;
	}

	if (Ability == GetActiveAbility())
	{
		CancelActiveAbility(ResultToUse);
	}
	else if (Ability && Ability->IsPassive())
	{
		for (int i = 0; i < m_PassiveAbilityInstances.Num(); ++i)
		{
			if (m_PassiveAbilityInstances[i].GetAbilityNameHash() == Ability->GetAbilityNameHash())
			{
                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAble, Warning, TEXT("[%s] CancelPassiveAbility( %s ) Result [%s]"),
                        *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *m_PassiveAbilityInstances[i].GetAbility().GetDisplayName(),
                        *FAbleLogHelper::GetTaskResultEnumAsString(ResultToUse));
                }

				m_PassiveAbilityInstances[i].FinishAbility();
				HandleInstanceCleanUp(m_PassiveAbilityInstances[i].GetAbility());
				m_PassiveAbilityInstances.RemoveAt(i);

                m_PassivesDirty |= true;
				break;
			}
		}
	}
}

void UAblAbilityComponent::AddCooldownForAbility(const UAblAbility& Ability, const UAblAbilityContext& Context)
{
	if (Ability.GetCooldown(&Context) > 0.0f)
	{
		m_ActiveCooldowns.Add(Ability.GetAbilityNameHash(), FAblAbilityCooldown(Ability, Context));
	}
}

bool UAblAbilityComponent::IsAbilityOnCooldown(const UAblAbility* Ability) const
{
	return Ability ? m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()) != nullptr : false;
}

bool UAblAbilityComponent::IsPassiveActive(const UAblAbility* Ability) const
{
	if (Ability && Ability->IsPassive())
	{
		return m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(Ability->GetAbilityNameHash())) != nullptr;
	}

	return false;
}

void UAblAbilityComponent::InternalUpdateAbility(FAblAbilityInstance* AbilityInstance, float DeltaTime)
{
	if (AbilityInstance)
	{
		if (!AbilityInstance->PreUpdate())
		{
			InternalCancelAbility(&AbilityInstance->GetAbility(), EAblAbilityTaskResult::Successful);
			return;
		}

		if (AbilityInstance->HasAsyncTasks())
		{
			if (UAbleSettings::IsAsyncEnabled() && m_Settings->GetAllowAbilityAsyncUpdate())
			{
				TGraphTask<FAsyncAbilityInstanceUpdaterTask>::CreateTask().ConstructAndDispatchWhenReady(AbilityInstance, AbilityInstance->GetCurrentTime(), DeltaTime);
			}
			else
			{
				// Run our Async update synchronously.
				AbilityInstance->AsyncUpdate(AbilityInstance->GetCurrentTime(), DeltaTime);
			}
		}

		AbilityInstance->SyncUpdate(DeltaTime);
	}
}

void UAblAbilityComponent::UpdateCooldowns(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AblAbilityComponent::UpdateCooldowns"), STAT_AblAbilityComponent_UpdateCooldowns, STATGROUP_Able);
	for (auto ItUpdate = m_ActiveCooldowns.CreateIterator(); ItUpdate; ++ItUpdate)
	{
		FAblAbilityCooldown& AbilityCooldown = ItUpdate->Value;

		AbilityCooldown.Update(DeltaTime);
		if (AbilityCooldown.IsComplete())
		{
			ItUpdate.RemoveCurrent();
		}
	}
}

void UAblAbilityComponent::HandlePendingContexts()
{
	check(m_PendingContext.Num() == m_PendingResult.Num());

	for (int i = 0; i < m_PendingContext.Num(); ++i)
	{
		if (const UAblAbility* PendingAbility = m_PendingContext[i]->GetAbility())
		{
			if (!PendingAbility->IsPassive())
			{
				EAblAbilityStartResult PendingStart = CanActivateAbility(m_PendingContext[i]);
				// Only process the pending context if we're going to succeed, or it's a branch and we know the target and cooldown are okay.
				bool validPendingContext = (PendingStart == EAblAbilityStartResult::Success);
				validPendingContext |= (m_PendingResult[i].GetValue() == EAblAbilityTaskResult::Branched &&
										(PendingStart != EAblAbilityStartResult::CooldownNotExpired && PendingStart != EAblAbilityStartResult::InvalidTarget));

				if (!validPendingContext)
				{
					continue;
				}

				// If we're about to start an Active ability, we need to first cancel any current active.
				if (m_ActiveAbilityInstance.IsValid())
				{
					switch (m_PendingResult[i].GetValue())
					{
					case EAblAbilityTaskResult::Interrupted:
						m_ActiveAbilityInstance.InterruptAbility();
						break;
					case EAblAbilityTaskResult::Successful:
						m_ActiveAbilityInstance.FinishAbility();
						break;
					case EAblAbilityTaskResult::Branched:
						m_ActiveAbilityInstance.BranchAbility();
						break;
					}

                    if (m_Settings->GetLogVerbose())
                    {
                        UE_LOG(LogAble, Warning, TEXT("[%s] HandlePendingContexts( %s ) Result [%s]"),
                            *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                            *FAbleLogHelper::GetTaskResultEnumAsString(m_PendingResult[i].GetValue()),
                            *m_ActiveAbilityInstance.GetAbility().GetDisplayName());
                    }

					HandleInstanceCleanUp(m_ActiveAbilityInstance.GetAbility());

					m_ActiveAbilityInstance.Reset();
					m_ActiveAbilityResult = m_PendingResult[i].GetValue();
				}
			}

			InternalStartAbility(m_PendingContext[i]);
		}
	}

	m_PendingContext.Empty();
	m_PendingResult.Empty();
}

void UAblAbilityComponent::HandlePendingCancels()
{
	for (FAblPendingCancelContext& CancelContext : m_PendingCancels)
	{
		if (m_ActiveAbilityInstance.IsValid() && m_ActiveAbilityInstance.GetAbilityNameHash() == CancelContext.GetNameHash())
		{
			switch (CancelContext.GetResult())
			{
			case EAblAbilityTaskResult::Interrupted:
				m_ActiveAbilityInstance.InterruptAbility();
				break;
			case EAblAbilityTaskResult::Successful:
				m_ActiveAbilityInstance.FinishAbility();
				break;
			case EAblAbilityTaskResult::Branched:
				m_ActiveAbilityInstance.BranchAbility();
				break;
			}

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAble, Warning, TEXT("[%s] HandlePendingCancels( %s ) Result [%s]"),
                    *FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *FAbleLogHelper::GetTaskResultEnumAsString(CancelContext.GetResult()),
                    *m_ActiveAbilityInstance.GetAbility().GetDisplayName());
            }
			
			HandleInstanceCleanUp(m_ActiveAbilityInstance.GetAbility());

			m_ActiveAbilityInstance.Reset();
			m_ActiveAbilityResult = CancelContext.GetResult();
		}
		else
		{
			for (int32 i = 0; i < m_PassiveAbilityInstances.Num(); ++i)
			{
				if (m_PassiveAbilityInstances[i].GetAbilityNameHash() == CancelContext.GetNameHash())
				{
					if (m_PassiveAbilityInstances[i].IsValid())
					{
						m_PassiveAbilityInstances[i].FinishAbility();
					}

					HandleInstanceCleanUp(m_PassiveAbilityInstances[i].GetAbility());
					m_PassiveAbilityInstances.RemoveAt(i);
					m_PassivesDirty |= true;
					break;
				}
			}
		}
	}

	m_PendingCancels.Empty();
}

void UAblAbilityComponent::HandleInstanceLogic(UAblAbilityContext* Context)
{
	check(Context->GetAbility());
	switch (Context->GetAbility()->GetInstancePolicy())
	{
		case EAblInstancePolicy::NewInstance:
		case EAblInstancePolicy::NewInstanceReplicated:
		{
			check(Context->GetAbility()->GetInstancePolicy() == EAblInstancePolicy::NewInstance || IsAuthoritative());
			UAblAbility* NewInstance = NewObject<UAblAbility>(GetOwner(), Context->GetAbility()->GetClass(), NAME_None, RF_Transient, Cast<UObject>(Context->GetAbilityBP()));
			check(NewInstance);

#if WITH_EDITOR
			// BlueprintGeneratedClass.cpp has a bug when the array size of the CDO doesn't match the array size of the Template - this can only happen in Editor builds. So, fix that here.
			NewInstance->ValidateDataAgainstTemplate(Context->GetAbility());
#endif
			m_CreatedAbilityInstances.Add(NewInstance);
			Context->SetAbility(NewInstance);
		}
		break;
		case EAblInstancePolicy::Default:
		default:
			break;
	}
}

void UAblAbilityComponent::HandleInstanceCleanUp(const UAblAbility& Ability)
{
	switch (Ability.GetInstancePolicy())
	{
	case EAblInstancePolicy::NewInstance:
	case EAblInstancePolicy::NewInstanceReplicated:
	{
		const UAblAbility* LHS = &Ability;

		// Only Server can destroy Replicated objects.
		if (LHS->GetInstancePolicy() == EAblInstancePolicy::NewInstanceReplicated && !IsAuthoritative())
		{
			break;
		}

		// The things I do to avoid a const_cast...
		UAblAbility** HeldAbility = m_CreatedAbilityInstances.FindByPredicate([&](UAblAbility* RHS)
		{
			return LHS == RHS;
		});

		if (HeldAbility)
		{
			UAblAbility* AbilityToRemove = *HeldAbility;
			m_CreatedAbilityInstances.Remove(AbilityToRemove);
			AbilityToRemove->MarkAsGarbage();
		}
	}
	break;
	case EAblInstancePolicy::Default:
	default:
		break;
	}
}

void UAblAbilityComponent::ValidateRemoteRunningAbilities()
{
	if (IsOwnerLocallyControlled() || IsAuthoritative())
	{
		return;
	}

	// Validate the Active Ability.
	if (m_ServerActive.IsValid())
	{
		if (!m_ActiveAbilityInstance.IsValid() || m_ServerActive.GetAbility()->GetAbilityNameHash() != m_ActiveAbilityInstance.GetAbilityNameHash())
		{
			// Server has an Active, but we aren't playing anything (or we're playing the wrong thing some how).
			InternalStartAbility(UAblAbilityContext::MakeContext(m_ServerActive), true);
		}
	}

	// Validate Passive Abilities.
	for (const FAblAbilityNetworkContext& PassiveContext : m_ServerPassiveAbilities)
	{
		if (PassiveContext.IsValid())
		{
			if (!IsPassiveActive(PassiveContext.GetAbility().Get()))
			{
				// Server has a Passive, but we aren't playing it. Fix that.
				InternalStartAbility(UAblAbilityContext::MakeContext(PassiveContext), true);
			}
		}
	}
}

bool UAblAbilityComponent::IsOwnerLocallyControlled() const
{
	//if (GetNetMode() == ENetMode::NM_ListenServer)
	//{
	//	return true;
	//}

	if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		return PawnOwner->IsLocallyControlled();
	}

	return false;
}

bool UAblAbilityComponent::IsAuthoritative() const
{
	return !IsNetworked() || (IsNetworked() && GetOwnerRole() == ROLE_Authority);
}

bool UAblAbilityComponent::AbilityClientPolicyAllowsExecution(const UAblAbility* Ability, bool IsPrediction) const
{
	if (!Ability)
	{
		return false;
	}

	switch (Ability->GetClientPolicy())
	{
		default:
		case EAblClientExecutionPolicy::Default:
		{
			// Remote, Client, or Authoritative
			if (IsPrediction)
			{
				return IsOwnerLocallyControlled() || IsAuthoritative();
			}

			return true;
		}
		break;
		case EAblClientExecutionPolicy::LocalAndAuthoritativeOnly:
		{
			return IsAuthoritative() || IsOwnerLocallyControlled();
		}
		break;
	}

	return false;
}

EAblAbilityStartResult UAblAbilityComponent::BranchAbility(UAblAbilityContext* Context)
{
	EAblAbilityStartResult Result = EAblAbilityStartResult::InternalSystemsError;
	if (!IsAuthoritative())
	{
		FAblAbilityNetworkContext AbilityNetworkContext(*Context);
		ServerBranchAbility(AbilityNetworkContext);

		if (IsOwnerLocallyControlled())
		{
			// Track for prediction.
			AddLocallyPredictedAbility(AbilityNetworkContext);

			QueueContext(Context, EAblAbilityTaskResult::Branched);
			Result = EAblAbilityStartResult::Success;
		}
		else
		{
			Result = EAblAbilityStartResult::ForwardedToServer;
		}
	}
	else
	{
		QueueContext(Context, EAblAbilityTaskResult::Branched);
		Result = EAblAbilityStartResult::Success;
	}

	CheckNeedsTick();

	return Result;
}

bool UAblAbilityComponent::IsAbilityRealmAllowed(const UAblAbility& Ability) const
{
	if (!IsNetworked())
	{
		return true;
	}

	switch (Ability.GetAbilityRealm())
	{
		case EAblAbilityTaskRealm::ATR_Client:
		{
			return !IsAuthoritative() || IsOwnerLocallyControlled() || GetWorld()->GetNetMode() == ENetMode::NM_ListenServer;
		}
		break;
		case EAblAbilityTaskRealm::ATR_Server:
		{
			return IsAuthoritative();
		}
		break;
		case EAblAbilityTaskRealm::ATR_ClientAndServer:
		{
			return true;
		}
		break;
		default:
			break;
	}

	return false;
}

void UAblAbilityComponent::UpdateServerPassiveAbilities()
{
	check(IsAuthoritative()); // Should only be called on the server.

	TArray<uint32> Whitelist;
	for (const FAblAbilityInstance& PassiveInstance : m_PassiveAbilityInstances)
	{
		Whitelist.Add(PassiveInstance.GetAbilityNameHash());
		if (FAblAbilityNetworkContext* ExistingNetworkContext = m_ServerPassiveAbilities.FindByPredicate(FAblFindAbilityNetworkContextByHash(PassiveInstance.GetAbilityNameHash())))
		{
			ExistingNetworkContext->SetCurrentStacks(PassiveInstance.GetStackCount());
		}
		else
		{
			// New Passive.
			m_ServerPassiveAbilities.Add(FAblAbilityNetworkContext(PassiveInstance.GetContext()));
		}
	}

	m_ServerPassiveAbilities.RemoveAll(FAblAbilityNetworkContextWhiteList(Whitelist));
}

void UAblAbilityComponent::UpdateServerActiveAbility()
{
	check(IsAuthoritative()); // Should only be called on the server.

	if (!m_ActiveAbilityInstance.IsValid())
	{
		m_ServerActive.Reset();
	}
	else if(!m_ServerActive.IsValid() || m_ServerActive.GetAbility()->GetAbilityNameHash() != m_ActiveAbilityInstance.GetAbilityNameHash())
	{
		m_ServerActive = FAblAbilityNetworkContext(m_ActiveAbilityInstance.GetContext(), m_ActiveAbilityResult.GetValue());
	}
	else if (m_ServerActive.IsValid() && m_ServerActive.GetAbility()->GetAbilityNameHash() == m_ActiveAbilityInstance.GetAbilityNameHash())
	{
		FAblAbilityNetworkContext forcedContext(m_ActiveAbilityInstance.GetContext());
		ClientForceAbility(forcedContext);
	}
}

void UAblAbilityComponent::ServerActivateAbility_Implementation(const FAblAbilityNetworkContext& Context)
{
	UAblAbilityContext* LocalContext = UAblAbilityContext::MakeContext(Context);
	if (InternalStartAbility(LocalContext) == EAblAbilityStartResult::Success)
	{
		if (!Context.GetAbility()->IsPassive())
		{
			m_ServerActive = FAblAbilityNetworkContext::UpdateNetworkContext(*LocalContext, Context);
		}
		else
		{
			int32 StackCount = GetCurrentStackCountForPassiveAbility(Context.GetAbility().Get());
			if (FAblAbilityNetworkContext* ExistingContext = m_ServerPassiveAbilities.FindByPredicate(FAblFindAbilityNetworkContextByHash(Context.GetAbility()->GetAbilityNameHash())))
			{
				ExistingContext->SetCurrentStacks(StackCount);
			}
			else
			{
				FAblAbilityNetworkContext NewNetworkContext = FAblAbilityNetworkContext::UpdateNetworkContext(*LocalContext, Context);
				NewNetworkContext.SetCurrentStacks(StackCount);
				m_ServerPassiveAbilities.Add(NewNetworkContext);
			}
		}
	}
}

bool UAblAbilityComponent::ServerActivateAbility_Validate(const FAblAbilityNetworkContext& Context)
{
	// Nothing really to check here since the server isn't trusting the Client at all.
	return Context.IsValid();
}

void UAblAbilityComponent::ServerCancelAbility_Implementation(uint32 AbilityNameHash, EAblAbilityTaskResult ResultToUse)
{
	const UAblAbility* Ability = nullptr;
	const UAblAbilityContext* Context = nullptr;
	if (m_ActiveAbilityInstance.IsValid() && m_ActiveAbilityInstance.GetAbilityNameHash() == AbilityNameHash)
	{
		Ability = &m_ActiveAbilityInstance.GetAbility();
		Context = &m_ActiveAbilityInstance.GetContext();
	}
	else
	{
		if (FAblAbilityInstance* PassiveInstance = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(AbilityNameHash)))
		{
			Ability = &PassiveInstance->GetAbility();
			Context = &PassiveInstance->GetContext();
		}
	}

	if (Ability && Context)
	{
		// We have a cancel request, but our Server hasn't canceled the Ability. Verify the client is allowed to cancel it.
		if (Ability->CanClientCancelAbilityBP(Context))
		{
			// Just pass it along.
			InternalCancelAbility(Ability, ResultToUse);
		}
		else if(m_Settings->GetLogVerbose())
		{
			UE_LOG(LogAble, Warning, TEXT("[%s] ServerCancelAbility ignored because Ability [%s] CanClientCancelAbility returned false."),
				*FAbleLogHelper::GetWorldName(GetOwner()->GetWorld()),
				*Ability->GetDisplayName());
		}
	}

}

bool UAblAbilityComponent::ServerCancelAbility_Validate(uint32 AbilityNameHash, EAblAbilityTaskResult ResultToUse)
{
	return AbilityNameHash != 0U;
}

void UAblAbilityComponent::ServerBranchAbility_Implementation(const FAblAbilityNetworkContext& Context)
{
	check(IsAuthoritative());

	QueueContext(UAblAbilityContext::MakeContext(Context), EAblAbilityTaskResult::Branched);
}

bool UAblAbilityComponent::ServerBranchAbility_Validate(const FAblAbilityNetworkContext& Context)
{
	return Context.IsValid();
}

bool UAblAbilityComponent::ClientForceAbility_Validate(const FAblAbilityNetworkContext& Context)
{
	return Context.IsValid();
}

void UAblAbilityComponent::ClientForceAbility_Implementation(const FAblAbilityNetworkContext& Context)
{
	if (GetOwner() && GetOwner()->GetNetMode() == ENetMode::NM_Client)
	{
		InternalStartAbility(UAblAbilityContext::MakeContext(Context), true);
	}
}

void UAblAbilityComponent::OnServerActiveAbilityChanged()
{
	if (m_ServerActive.IsValid() && 
		AbilityClientPolicyAllowsExecution(m_ServerActive.GetAbility().Get()))
	{
		if (WasLocallyPredicted(m_ServerActive))
		{
			// Same Ability, skip it since we were locally predicting it.
			return;
		}

		if (IsPlayingAbility())
		{
			// TODO: Should the client care about server based interrupt/branches?
			InternalCancelAbility(GetActiveAbility(), EAblAbilityTaskResult::Successful);
		}

		InternalStartAbility(UAblAbilityContext::MakeContext(m_ServerActive), true);
	}
	else
	{
		if (IsPlayingAbility())
		{
			InternalCancelAbility(GetActiveAbility(), m_ServerActive.GetResult());
		}
	}
}

void UAblAbilityComponent::OnServerPassiveAbilitiesChanged()
{
	TArray<uint32> ValidAbilityNameHashes;
	for (const FAblAbilityNetworkContext& ServerPassive : m_ServerPassiveAbilities)
	{
		if (ServerPassive.IsValid() && 
			ServerPassive.GetAbility().IsValid() && 
			AbilityClientPolicyAllowsExecution(ServerPassive.GetAbility().Get()))
		{
			if (FAblAbilityInstance* CurrentPassive = m_PassiveAbilityInstances.FindByPredicate(FAblFindAbilityInstanceByHash(ServerPassive.GetAbility()->GetAbilityNameHash())))
			{
				// Just make sure our stack count is accurate.
				CurrentPassive->SetStackCount(ServerPassive.GetCurrentStack());
			}
			else // New Passive Ability
			{
				if (!WasLocallyPredicted(ServerPassive))
				{
					ActivatePassiveAbility(UAblAbilityContext::MakeContext(ServerPassive));
				}
			}

			ValidAbilityNameHashes.Add(ServerPassive.GetAbility()->GetAbilityNameHash());
		}
	}

	m_PassiveAbilityInstances.RemoveAll(FAblAbilityInstanceWhiteList(ValidAbilityNameHashes));
    m_PassivesDirty |= true;
}

void UAblAbilityComponent::OnServerPredictiveKeyChanged()
{
	// Update our client key to use the latest key from the server.
	m_ClientPredictionKey = m_ServerPredictionKey;
}

#if WITH_EDITOR

void UAblAbilityComponent::PlayAbilityFromEditor(const UAblAbility* Ability, AActor* ForceTarget)
{
	// Interrupt any current abilities.
	if (m_ActiveAbilityInstance.IsValid())
	{
		m_ActiveAbilityInstance.StopAbility();
		m_ActiveAbilityInstance.Reset();
	}

	if (!Ability)
	{
		return;
	}

	UAblAbilityContext* FakeContext = UAblAbilityContext::MakeContext(Ability, this, GetOwner(), nullptr);
	// Run Targeting so we can visually see it - we don't care about the result since the editor always succeeds.
	Ability->CanAbilityExecute(*FakeContext);

	// Force our Target, if we have one.
	if (ForceTarget)
	{
		FakeContext->GetMutableTargetActors().Add(ForceTarget);
	}

	// We've passed all our checks, go ahead and allocate our Task scratch pads.
	FakeContext->AllocateScratchPads();

	m_ActiveAbilityInstance.Initialize(*FakeContext);

	CheckNeedsTick();
}

float UAblAbilityComponent::GetCurrentAbilityTime() const
{
	if (m_ActiveAbilityInstance.IsValid())
	{
		return m_ActiveAbilityInstance.GetCurrentTime();
	}

	return 0.0f;
}

void UAblAbilityComponent::SetAbilityTime(float NewTime)
{
	if (m_ActiveAbilityInstance.IsValid())
	{
		m_ActiveAbilityInstance.SetCurrentTime(NewTime);
	}
}

#endif

void UAblAbilityComponent::AddTag(const FGameplayTag Tag)
{
	m_TagContainer.AddTag(Tag);
}

void UAblAbilityComponent::RemoveTag(const FGameplayTag Tag)
{
	m_TagContainer.RemoveTag(Tag);
}

bool UAblAbilityComponent::HasTag(const FGameplayTag Tag, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    return CombinedTags.HasTag(Tag);
}

bool UAblAbilityComponent::MatchesAnyTag(const FGameplayTagContainer Container, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    return CombinedTags.HasAny(Container);
}

bool UAblAbilityComponent::MatchesAllTags(const FGameplayTagContainer Container, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    return CombinedTags.HasAll(Container);
}

bool UAblAbilityComponent::CheckTags(const FGameplayTagContainer& IncludesAny, const FGameplayTagContainer& IncludesAll, const FGameplayTagContainer& ExcludesAny, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    if (!ExcludesAny.IsEmpty())
    {
        if (CombinedTags.HasAny(ExcludesAny))
            return false;
    }

    if (!IncludesAny.IsEmpty())
    {
        if (!CombinedTags.HasAny(IncludesAny))
            return false;
    }

    if (!IncludesAll.IsEmpty())
    {
        if (!CombinedTags.HasAll(IncludesAll))
            return false;
    }
    return true;
}

void UAblAbilityComponent::GetCombinedGameplayTags(FGameplayTagContainer& CombinedTags, bool includeExecutingAbilities) const
{
    CombinedTags = FGameplayTagContainer::EmptyContainer;
    CombinedTags.AppendTags(m_TagContainer);

    if (includeExecutingAbilities)
    {
        if (m_ActiveAbilityInstance.IsValid())
        {
            CombinedTags.AppendTags(m_ActiveAbilityInstance.GetAbility().GetAbilityTagContainer());
        }

        for (const FAblAbilityInstance& Passive : m_PassiveAbilityInstances)
        {
            if (Passive.IsValid())
            {
                CombinedTags.AppendTags(Passive.GetAbility().GetAbilityTagContainer());
            }
        }
    }
}

bool UAblAbilityComponent::MatchesQuery(const FGameplayTagQuery Query, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);
    return CombinedTags.MatchesQuery(Query);
}

void UAblAbilityComponent::SetAbilityAnimationNode(const FAnimNode_AbilityAnimPlayer* Node)
{
	FScopeLock CS(&m_AbilityAnimNodeCS);

	m_AbilityAnimationNode = Node;
}

uint16 UAblAbilityComponent::GetPredictionKey()
{
	if (IsAuthoritative())
	{
		m_ServerPredictionKey = FMath::Max<uint16>(++m_ServerPredictionKey, 1);
		return m_ServerPredictionKey;
	}

	m_ClientPredictionKey = FMath::Max<uint16>(++m_ClientPredictionKey, 1);
	return m_ClientPredictionKey;
}

void UAblAbilityComponent::AddLocallyPredictedAbility(const FAblAbilityNetworkContext& Context)
{
	m_LocallyPredictedAbilities[Context.GetPredictionKey() % ABLE_ABILITY_PREDICTION_RING_SIZE] = Context;
}

bool UAblAbilityComponent::WasLocallyPredicted(const FAblAbilityNetworkContext& Context)
{
	if (Context.GetPredictionKey() == 0 || m_LocallyPredictedAbilities.Num() == 0)
	{
		return false;
	}

	uint32 PredictionTolerance = m_Settings.IsValid() ? m_Settings->GetPredictionTolerance() : 0U;

	FAblAbilityNetworkContext* FoundLocalContext = m_LocallyPredictedAbilities.FindByPredicate([&](const FAblAbilityNetworkContext& LHS)
	{
		return LHS.GetAbility().IsValid() && LHS.GetAbility()->GetAbilityNameHash() == Context.GetAbility()->GetAbilityNameHash() && (uint32)FMath::Abs(LHS.GetPredictionKey() - Context.GetPredictionKey()) <= PredictionTolerance;
	});

	if (FoundLocalContext)
	{
		// Found a match entry, consume it.
		FoundLocalContext->Reset();
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
