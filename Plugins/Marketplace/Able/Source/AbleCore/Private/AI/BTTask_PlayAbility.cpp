// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AI/BTTask_PlayAbility.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_PlayAbility::UBTTask_PlayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	MarkAsInProgressDuringExecution(false),
    m_CachedAbility(nullptr)
{
	NodeName = "Play Ability";
	bNotifyTaskFinished = true;
	AbilityKey.SelectedKeyName = FBlackboard::KeySelf;
	AbilityKey.AllowNoneAsValue(true);
}

const UAblAbility* UBTTask_PlayAbility::GetAbility(const UBlackboardComponent* BlackboardComp) const
{
    if (AbilityKey.IsSet())
    {
        UClass* AbilityClass = BlackboardComp->GetValueAsClass(AbilityKey.SelectedKeyName);
        if (const UAblAbility* AbilityCDO = Cast<UAblAbility>(AbilityClass->GetDefaultObject()))
        {
            return AbilityCDO;
        }
    }
    else if (Ability != nullptr)
    {
        if (const UAblAbility* AbilityCDO = Cast<UAblAbility>(Ability.GetDefaultObject()))
        {
            return AbilityCDO;
        }
    }
    return nullptr;
}

EBTNodeResult::Type UBTTask_PlayAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTPlayAbilityTaskMemory* TaskMemory = reinterpret_cast<FBTPlayAbilityTaskMemory*>(NodeMemory);
	AAIController* const OwnerController = OwnerComp.GetAIOwner();
	if (!OwnerController)
	{
		return EBTNodeResult::Aborted;
	}
	UBlackboardComponent* BlackboardComp = OwnerController->GetBlackboardComponent();

    if (const UAblAbility* AbilityCDO = GetAbility(BlackboardComp))
	{
		m_CachedAbility = AbilityCDO;
		if (m_CachedAbility && OwnerController)
		{
			if (APawn* Owner = OwnerController->GetPawn())
			{
				if (UAblAbilityComponent* AbilityComponent = Owner->FindComponentByClass<UAblAbilityComponent>())
				{
					if (!TaskMemory->bAbilityPlayed)
					{
						UAblAbilityContext* Context = UAblAbilityContext::MakeContext(m_CachedAbility, AbilityComponent, Owner, nullptr);
						EAblAbilityStartResult Result = AbilityComponent->ActivateAbility(Context);
						if (Result == EAblAbilityStartResult::Success || Result == EAblAbilityStartResult::AsyncProcessing)
						{
							TaskMemory->bAbilityPlayed = true;
							if (MarkAsInProgressDuringExecution)
							{
								TaskMemory->AbilityComponent = AbilityComponent;
								return EBTNodeResult::InProgress;
							}
							else
							{
								return EBTNodeResult::Succeeded;
							}
						}
						else
						{
							return EBTNodeResult::Failed;
						}
					}
				}
			}
		}
	}

	return EBTNodeResult::Failed;
}

void UBTTask_PlayAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTPlayAbilityTaskMemory* TaskMemory = reinterpret_cast<FBTPlayAbilityTaskMemory*>(NodeMemory);
	if (MarkAsInProgressDuringExecution && TaskMemory->bAbilityPlayed)
	{
		if (UAblAbilityComponent* AbilityComponent = TaskMemory->AbilityComponent.Get())
		{
			// Our Active is no longer the Ability we were playing. Finish this Task.
			if (!m_CachedAbility || AbilityComponent->GetActiveAbility() != m_CachedAbility)
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
		}
	}
}

void UBTTask_PlayAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	FBTPlayAbilityTaskMemory* TaskMemory = reinterpret_cast<FBTPlayAbilityTaskMemory*>(NodeMemory);
	
	TaskMemory->bAbilityPlayed = false;
	TaskMemory->AbilityComponent.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_PlayAbility::GetStaticDescription() const
{
    FString AbilityName = TEXT("NONE");
    if (AbilityKey.IsSet())
	{
        AbilityName = FString::Format(TEXT("BlueprintKey(%s)"), { AbilityKey.SelectedKeyName.ToString() });
	}
    else if (Ability != nullptr)
	{
        AbilityName = FString::Format(TEXT("AbilityClass(%s)"), { Ability->GetDefaultObject()->GetName() });
	}

    return FString::Printf(TEXT("%s: Plays the [%s] Ability."), *Super::GetStaticDescription(), *AbilityName);
}

void UBTTask_PlayAbility::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	FBTPlayAbilityTaskMemory* TaskMemory = reinterpret_cast<FBTPlayAbilityTaskMemory*>(NodeMemory);

	const FString RuntimeDesc = TaskMemory->bAbilityPlayed ? (MarkAsInProgressDuringExecution ? TEXT("In Progress") : TEXT("Success")) : TEXT("Waiting");

	Values.Add(RuntimeDesc);
}

uint16 UBTTask_PlayAbility::GetInstanceMemorySize() const
{
	return sizeof(FBTPlayAbilityTaskMemory);
}

void UBTTask_PlayAbility::PostLoad()
{
	Super::PostLoad();

	bNotifyTick = MarkAsInProgressDuringExecution;
}

void UBTTask_PlayAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		AbilityKey.ResolveSelectedKey(*BBAsset);
	}
}

#if WITH_EDITOR
FName UBTTask_PlayAbility::GetNodeIconName() const
{
	return Super::GetNodeIconName();
}
#endif
