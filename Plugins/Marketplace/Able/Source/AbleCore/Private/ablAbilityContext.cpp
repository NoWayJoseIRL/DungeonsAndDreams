// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ablAbilityContext.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablSettings.h"
#include "ablSubSystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeLock.h"
#include "Net/UnrealNetwork.h"
#include "Tasks/IAblAbilityTask.h"

FVector FAblQueryResult::GetLocation() const
{
	FTransform TargetTransform;
	GetTransform(TargetTransform);

	return TargetTransform.GetTranslation();
}

void FAblQueryResult::GetTransform(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (Actor.IsValid())
	{
		OutTransform = Actor->GetActorTransform();
	}
	else if (PrimitiveComponent.IsValid())
	{
		if (AActor* ActorOwner = PrimitiveComponent->GetOwner())
		{
			OutTransform = ActorOwner->GetActorTransform();
		}
		else
		{
			// Is this ever really valid?
			OutTransform = PrimitiveComponent->GetComponentTransform();
		}
	}
}

UAblAbilityContext::UAblAbilityContext(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),    
    m_AbilityActorStartLocation(FVector::ZeroVector),
	m_Ability(nullptr),
	m_AbilityComponent(nullptr),
	m_StackCount(1),
	m_LoopIteration(0),
	m_CurrentTime(0.0f),
	m_LastDelta(0.0f),
	m_AbilityScratchPad(nullptr),
	m_TargetLocation(FVector::ZeroVector),
	m_PredictionKey(0)
{

}

UAblAbilityContext::~UAblAbilityContext()
{

}

UAblAbilityContext* UAblAbilityContext::MakeContext(const UAblAbility* Ability, UAblAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator)
{
	if (!Ability || !AbilityComponent)
	{
		return nullptr;
	}

	static const UAbleSettings* Settings = nullptr;
	if (!Settings)
	{
		// This shouldn't be able to change out from under us.
		Settings = GetDefault<UAbleSettings>();
	}

	UAblAbilityContext* NewContext = nullptr;
	if (Settings && Settings->GetAllowAbilityContextReuse())
	{
		if (const AActor* ACOwner = AbilityComponent->GetOwner())
		{
			if (UWorld* OwnerWorld = ACOwner->GetWorld())
			{
				if (UAblAbilityUtilitySubsystem* ContextSubsystem = OwnerWorld->GetSubsystem<UAblAbilityUtilitySubsystem>())
				{
					NewContext = ContextSubsystem->FindOrConstructContext();
				}
			}
		}
	}
	
	if (NewContext == nullptr)
	{
		NewContext = NewObject<UAblAbilityContext>(AbilityComponent);
	}
	check(NewContext != nullptr);

	NewContext->m_Ability = Ability;
	NewContext->m_AbilityComponent = AbilityComponent;

	if (Owner)
	{
		NewContext->m_Owner = Owner;
	}

	if (Instigator)
	{
		NewContext->m_Instigator = Instigator;
	}

	return NewContext;
}

UAblAbilityContext* UAblAbilityContext::MakeContext(const FAblAbilityNetworkContext& NetworkContext)
{
	UAblAbilityContext* NewContext = MakeContext(NetworkContext.GetAbility().Get(), NetworkContext.GetAbilityComponent().Get(), NetworkContext.GetOwner().Get(), NetworkContext.GetInstigator().Get());
	NewContext->GetMutableTargetActors().Append(NetworkContext.GetTargetActors());
	NewContext->SetTargetLocation(NetworkContext.GetTargetLocation());
	NewContext->GetMutableIntParameters().Append(NetworkContext.GetIntParameters());
	NewContext->GetMutableFloatParameters().Append(NetworkContext.GetFloatParameters());
	NewContext->GetMutableStringParameters().Append(NetworkContext.GetStringParameters());
	NewContext->GetMutableUObjectParameters().Append(NetworkContext.GetUObjectParameters());
	NewContext->GetMutableVectorParameters().Append(NetworkContext.GetVectorParameters());
	return NewContext;
}

UAblAbilityContext* UAblAbilityContext::MakeContext(const UAblAbility* Ability, UAblAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator, FVector Location)
{
	UAblAbilityContext* NewContext = MakeContext(Ability, AbilityComponent, Owner, Instigator);
	NewContext->SetTargetLocation(Location);

	return NewContext;
}

void UAblAbilityContext::AllocateScratchPads()
{
	TSubclassOf<UAblAbilityScratchPad> AbilityScratchPad = m_Ability->GetAbilityScratchPadClassBP(this);
	if (UClass* AbilityScratchPadClass = AbilityScratchPad.Get())
	{
		if (UAblAbilityUtilitySubsystem* SubSystem = GetUtilitySubsystem())
		{
			m_AbilityScratchPad = SubSystem->FindOrConstructAbilityScratchPad(AbilityScratchPad);
			if (m_AbilityScratchPad)
			{
				// Reset the state.
				m_Ability->ResetAbilityScratchPad(m_AbilityScratchPad);
			}
		}
		else
		{
			// Couldn't get the Subsystem, fall back to the old system.
			m_AbilityScratchPad = NewObject<UAblAbilityScratchPad>(this, AbilityScratchPadClass);
		}
	}
	
	const TArray<UAblAbilityTask*>& Tasks = m_Ability->GetTasks();

	for (UAblAbilityTask* Task : Tasks)
	{
		if (!Task)
		{
			continue;
		}

		if (UAblAbilityTaskScratchPad* ScratchPad = Task->CreateScratchPad(TWeakObjectPtr<UAblAbilityContext>(this)))
		{
			m_TaskScratchPadMap.Add(Task->GetUniqueID(), ScratchPad);
		}
	}
}

void UAblAbilityContext::ReleaseScratchPads()
{
	if (UAblAbilityUtilitySubsystem* SubSystem = GetUtilitySubsystem())
	{
		if (m_AbilityScratchPad)
		{
			SubSystem->ReturnAbilityScratchPad(m_AbilityScratchPad);
			m_AbilityScratchPad = nullptr;
		}

		TMap<uint32, UAblAbilityTaskScratchPad*>::TConstIterator Iter = m_TaskScratchPadMap.CreateConstIterator();
		for (; Iter; ++Iter)
		{
			SubSystem->ReturnTaskScratchPad(Iter->Value);
		}

		m_TaskScratchPadMap.Empty();
	}
}

void UAblAbilityContext::UpdateTime(float DeltaTime)
{
	FPlatformMisc::MemoryBarrier();
	m_CurrentTime += DeltaTime;
	m_LastDelta = DeltaTime;
}

UAblAbilityTaskScratchPad* UAblAbilityContext::GetScratchPadForTask(const class UAblAbilityTask* Task) const
{
	UAblAbilityTaskScratchPad* const * ScratchPad = m_TaskScratchPadMap.Find(Task->GetUniqueID());
	if (ScratchPad && (*ScratchPad) != nullptr)
	{
		return *ScratchPad;
	}

	return nullptr;
}

UAblAbilityComponent* UAblAbilityContext::GetSelfAbilityComponent() const
{
	return m_AbilityComponent;
}

AActor* UAblAbilityContext::GetSelfActor() const
{
	if (m_AbilityComponent)
	{
		return m_AbilityComponent->GetOwner();
	}

	return nullptr;
}

int32 UAblAbilityContext::GetCurrentStackCount() const
{
	return m_StackCount;
}

int32 UAblAbilityContext::GetCurrentLoopIteration() const
{
	return m_LoopIteration;
}

void UAblAbilityContext::SetStackCount(int32 Stack)
{
	int32 MaxStacks = GetAbility()->GetMaxStacks(this);
	m_StackCount = FMath::Clamp(Stack, 1, MaxStacks);
}

void UAblAbilityContext::SetLoopIteration(int32 Loop)
{
	m_LoopIteration = FMath::Clamp(Loop, 0, (int32)GetAbility()->GetLoopMaxIterations(this));
}

float UAblAbilityContext::GetCurrentTimeRatio() const
{
	return FMath::Clamp(m_CurrentTime / m_Ability->GetLength(), 0.0f, 1.0f);
}

void UAblAbilityContext::SetIntParameter(FName Id, int Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_IntParameters.Add(Id, Value);
}

void UAblAbilityContext::SetFloatParameter(FName Id, float Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_FloatParameters.Add(Id, Value);
}

void UAblAbilityContext::SetStringParameter(FName Id, const FString& Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_StringParameters.Add(Id, Value);
}

void UAblAbilityContext::SetUObjectParameter(FName Id, UObject* Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_UObjectParameters.Add(Id, Value);
}

void UAblAbilityContext::SetVectorParameter(FName Id, FVector Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_VectorParameters.Add(Id, Value);
}

int UAblAbilityContext::GetIntParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const int* var = m_IntParameters.Find(Id))
	{
		return *var;
	}
	return 0;
}

float UAblAbilityContext::GetFloatParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const float* var = m_FloatParameters.Find(Id))
	{
		return *var;
	}
	return 0.0f;
}

UObject* UAblAbilityContext::GetUObjectParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (m_UObjectParameters.Contains(Id)) // Find is being weird with double ptr.
	{
		return m_UObjectParameters[Id];
	}
	return nullptr;
}

FVector UAblAbilityContext::GetVectorParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const FVector* var = m_VectorParameters.Find(Id))
	{
		return *var;
	}
	return FVector::ZeroVector;
}

const FString& UAblAbilityContext::GetStringParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const FString* var = m_StringParameters.Find(Id))
	{
		return *var;
	}
	static FString EmptyString;
	return EmptyString;
}

UAblAbilityUtilitySubsystem* UAblAbilityContext::GetUtilitySubsystem() const
{
	if (UWorld* CurrentWorld = GetWorld())
	{
		if (UAblAbilityUtilitySubsystem* UtilitySubsys = CurrentWorld->GetSubsystem<UAblAbilityUtilitySubsystem>())
		{
			return UtilitySubsys;
		}
	}

	return nullptr;
}

void UAblAbilityContext::Reset()
{
	m_AbilityActorStartLocation = FVector::ZeroVector;
	m_Ability = nullptr;
	m_AbilityComponent = nullptr;
	m_StackCount = 1;
	m_CurrentTime = 0.0f;
	m_LastDelta = 0.0f;
	m_Owner.Reset();
	m_Instigator.Reset();
	m_TargetActors.Empty();
	m_TaskScratchPadMap.Empty();
	m_AbilityScratchPad = nullptr;
	m_AsyncHandle._Handle = 0;
	m_AsyncQueryTransform = FTransform::Identity;
	m_TargetLocation = FVector::ZeroVector;
	m_PredictionKey = 0;
	m_IntParameters.Empty();
	m_FloatParameters.Empty();
	m_StringParameters.Empty();
	m_UObjectParameters.Empty();
	m_VectorParameters.Empty();

	if (UAblAbilityUtilitySubsystem* ContextSubsystem = GetUtilitySubsystem())
	{
		ContextSubsystem->ReturnContext(this);
	}
}

const TArray<AActor*> UAblAbilityContext::GetTargetActors() const
{
	// Blueprints don't like Weak Ptrs, so we have to do this fun copy.
	TArray<AActor*> ReturnVal;
	ReturnVal.Reserve(m_TargetActors.Num());

	for (const TWeakObjectPtr<AActor>& Actor : m_TargetActors)
	{
		if (Actor.IsValid())
		{
			ReturnVal.Add(Actor.Get());
		}
	}

	return ReturnVal;
}

FAblAbilityNetworkContext::FAblAbilityNetworkContext()
{
	Reset();
}

FAblAbilityNetworkContext::FAblAbilityNetworkContext(const UAblAbilityContext& Context)
	: m_Ability(Context.GetAbility()),
	m_AbilityComponent(Context.GetSelfAbilityComponent()),
	m_Owner(Context.GetOwner()),
	m_Instigator(Context.GetInstigator()),
	m_TargetActors(Context.GetTargetActors()),
	m_CurrentStacks(Context.GetCurrentStackCount()),
	m_TimeStamp(0.0f),
	m_Result(EAblAbilityTaskResult::Successful),
	m_TargetLocation(Context.GetTargetLocation()),
	m_PredictionKey(0),
	m_IntParameters(Context.GetIntParameters()),
	m_FloatParameters(Context.GetFloatParameters()),
	m_StringParameters(Context.GetStringParameters()),
	m_UObjectParameters(Context.GetUObjectParameters()),
	m_VectorParameters(Context.GetVectorParameters())
{
	if (GEngine && GEngine->GetWorld())
	{
		m_TimeStamp = GEngine->GetWorld()->GetRealTimeSeconds();
	}

	// Instanced Replicated / Server Abilities don't have local prediction.
	if (m_Ability->GetInstancePolicy() != EAblInstancePolicy::NewInstanceReplicated &&
		m_Ability->GetAbilityRealm() != EAblAbilityTaskRealm::ATR_Server)
	{
		m_PredictionKey = Context.GetSelfAbilityComponent()->GetPredictionKey();
	}
}

FAblAbilityNetworkContext::FAblAbilityNetworkContext(const UAblAbilityContext& Context, EAblAbilityTaskResult Result)
	: m_Ability(Context.GetAbility()),
	m_AbilityComponent(Context.GetSelfAbilityComponent()),
	m_Owner(Context.GetOwner()),
	m_Instigator(Context.GetInstigator()),
	m_TargetActors(Context.GetTargetActors()),
	m_CurrentStacks(Context.GetCurrentStackCount()),
	m_TimeStamp(0.0f),
	m_Result(Result),
	m_TargetLocation(Context.GetTargetLocation()),
	m_PredictionKey(0),
	m_IntParameters(Context.GetIntParameters()),
	m_FloatParameters(Context.GetFloatParameters()),
	m_StringParameters(Context.GetStringParameters()),
	m_UObjectParameters(Context.GetUObjectParameters()),
	m_VectorParameters(Context.GetVectorParameters())
{
	if (GEngine && GEngine->GetWorld())
	{
		m_TimeStamp = GEngine->GetWorld()->GetRealTimeSeconds();
	}

	// Instanced Replicated / Server Abilities don't have local prediction.
	if (m_Ability->GetInstancePolicy() != EAblInstancePolicy::NewInstanceReplicated &&
		m_Ability->GetAbilityRealm() != EAblAbilityTaskRealm::ATR_Server)
	{
		m_PredictionKey = Context.GetSelfAbilityComponent()->GetPredictionKey();
	}
}

void FAblAbilityNetworkContext::Reset()
{
	m_Ability = nullptr;
	m_AbilityComponent = nullptr;
	m_Owner.Reset();
	m_Instigator.Reset();
	m_TargetActors.Empty();
	m_CurrentStacks = 0;
	m_TimeStamp = 0.0f;
	m_Result = EAblAbilityTaskResult::Successful;
	m_TargetLocation = FVector::ZeroVector;
	m_PredictionKey = 0;
	m_IntParameters.Empty();
	m_FloatParameters.Empty();
	m_StringParameters.Empty();
	m_UObjectParameters.Empty();
	m_VectorParameters.Empty();
}

bool FAblAbilityNetworkContext::IsValid() const
{
	return m_Ability != nullptr && m_AbilityComponent != nullptr;
}

FAblAbilityNetworkContext FAblAbilityNetworkContext::UpdateNetworkContext(const UAblAbilityContext& UpdatedContext, const FAblAbilityNetworkContext& Source)
{
	FAblAbilityNetworkContext newContext(UpdatedContext);
	newContext.m_PredictionKey = Source.GetPredictionKey(); // Persist prediction key.
	
	return newContext;
}
