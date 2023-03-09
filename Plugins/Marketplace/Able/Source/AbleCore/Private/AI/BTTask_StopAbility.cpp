// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AI/BTTask_StopAbility.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_StopAbility::UBTTask_StopAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	CancelActive(true),
	ResultToUse(EAblAbilityTaskResult::Successful)
{
	NodeName = "Stop Ability";
	AbilityKey.SelectedKeyName = FBlackboard::KeySelf;
	AbilityKey.AllowNoneAsValue(true);
}

const UAblAbility* UBTTask_StopAbility::GetAbility(const UBlackboardComponent* BlackboardComp) const
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

EBTNodeResult::Type UBTTask_StopAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* const OwnerController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerController->GetBlackboardComponent();

    if (OwnerController)
    {
        if (APawn* Owner = OwnerController->GetPawn())
        {
            if (UAblAbilityComponent* AbilityComponent = Owner->FindComponentByClass<UAblAbilityComponent>())
            {
                AbilityComponent->CancelAbility(CancelActive ? AbilityComponent->GetActiveAbility() : GetAbility(BlackboardComp), ResultToUse.GetValue());

                return EBTNodeResult::Succeeded;
            }
        }
    }

	return EBTNodeResult::Failed;
}

FString UBTTask_StopAbility::GetStaticDescription() const
{
    FString AbilityName = TEXT("NONE");
    if(CancelActive)
	{
        AbilityName = TEXT("Current Active");
	}
    else if (AbilityKey.IsSet())
	{
        AbilityName = FString::Format(TEXT("BlueprintKey(%s)"), { AbilityKey.SelectedKeyName.ToString() });
	}
    else if (Ability != nullptr)
	{
        AbilityName = FString::Format(TEXT("AbilityClass(%s)"), { Ability->GetDefaultObject()->GetName() });
	}

    return FString::Printf(TEXT("%s: Stops the [%s] Ability."), *Super::GetStaticDescription(), *AbilityName);
}

void UBTTask_StopAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		AbilityKey.ResolveSelectedKey(*BBAsset);
	}
}

#if WITH_EDITOR
FName UBTTask_StopAbility::GetNodeIconName() const
{
	return Super::GetNodeIconName();
}
#endif
