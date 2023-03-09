// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AI/BTDecorator_HasActivePassiveAbility.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "VisualLogger/VisualLogger.h"

UBTDecorator_HasActivePassiveAbility::UBTDecorator_HasActivePassiveAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	NodeName = "Ability Playing Condition";

	// Accept only actors
	ActorToCheck.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasActivePassiveAbility, ActorToCheck), AActor::StaticClass());

	// Default to using Self Actor
	ActorToCheck.SelectedKeyName = FBlackboard::KeySelf;
	AbilityKey.SelectedKeyName = FBlackboard::KeySelf;

	// For now, don't allow users to select any "Abort Observers", because it's currently not supported.
	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

const UAblAbility* UBTDecorator_HasActivePassiveAbility::GetAbility(const UBlackboardComponent* BlackboardComp) const
{
    if (BlackboardComp == nullptr)
    {
        return nullptr;
    }

    if (AbilityKey.IsSet())
    {
        UClass* AbilityClass = BlackboardComp->GetValueAsClass(AbilityKey.SelectedKeyName);
        if (AbilityClass == nullptr)
        {
            UE_VLOG(BlackboardComp->GetOwner(), LogBehaviorTree, Warning, TEXT("Tree %s, Node %s, NULL Ability from Blackboard( %s ) Key: %s"),
                *GetTreeAsset()->GetFName().ToString(),
                *GetParentNode()->GetNodeName(),
                *BlackboardComp->GetFName().ToString(),
                *AbilityKey.SelectedKeyName.ToString());
        }
        else if (const UAblAbility* AbilityCDO = Cast<UAblAbility>(AbilityClass->GetDefaultObject()))
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

bool UBTDecorator_HasActivePassiveAbility::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return false;
	}

	if (AActor* Actor = Cast<AActor>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(ActorToCheck.GetSelectedKeyID())))
	{
        if (const UAblAbility* AbilityCDO = GetAbility(BlackboardComp))
        {
            if (UAblAbilityComponent* AbilityComponent = Actor->FindComponentByClass<UAblAbilityComponent>())
            {
                return AbilityComponent->IsPassiveActive(AbilityCDO);
            }
        }
	}

	return false;
}

FString UBTDecorator_HasActivePassiveAbility::GetStaticDescription() const
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

	return FString::Printf(TEXT("%s: checks if the actor has the passive Ability [%s] active."), *Super::GetStaticDescription(), *AbilityName);
}

void UBTDecorator_HasActivePassiveAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ActorToCheck.ResolveSelectedKey(*BBAsset);
		AbilityKey.ResolveSelectedKey(*BBAsset);
	}
}
