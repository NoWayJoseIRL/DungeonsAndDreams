// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ablTargetingMisc.h"

#include "ablAbility.h"
#include "ablAbilityContext.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/PrimitiveComponent.h"
#include "ablAbilityUtilities.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UAblTargetingInstigator::UAblTargetingInstigator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblTargetingInstigator::~UAblTargetingInstigator()
{

}

void UAblTargetingInstigator::FindTargets(UAblAbilityContext& Context) const
{
	if (AActor* InstigatorActor = Context.GetInstigator())
	{
		Context.GetMutableTargetActors().Add(InstigatorActor);
	}
	
	// No need to run filters.
}

#if WITH_EDITOR
EDataValidationResult UAblTargetingInstigator::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAblTargetingSelf::UAblTargetingSelf(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblTargetingSelf::~UAblTargetingSelf()
{

}

void UAblTargetingSelf::FindTargets(UAblAbilityContext& Context) const
{
	if (AActor* SelfActor = Context.GetSelfActor())
	{
		Context.GetMutableTargetActors().Add(SelfActor);
	}

	// Skip filters.
}

#if WITH_EDITOR
EDataValidationResult UAblTargetingSelf::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAblTargetingOwner::UAblTargetingOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblTargetingOwner::~UAblTargetingOwner()
{

}

void UAblTargetingOwner::FindTargets(UAblAbilityContext& Context) const
{
	if (AActor* OwnerActor = Context.GetOwner())
	{
		Context.GetMutableTargetActors().Add(OwnerActor);
	}

	// Skip filters.
}

#if WITH_EDITOR
EDataValidationResult UAblTargetingOwner::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAblTargetingCustom::UAblTargetingCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblTargetingCustom::~UAblTargetingCustom()
{

}

void UAblTargetingCustom::FindTargets(UAblAbilityContext& Context) const
{
	TArray<AActor*> FoundTargets;
	Context.GetAbility()->CustomTargetingFindTargetsBP(&Context, FoundTargets);
	Context.GetMutableTargetActors().Append(FoundTargets);

	// Run any extra filters.
	FilterTargets(Context);
}

#if WITH_EDITOR
EDataValidationResult UAblTargetingCustom::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CustomTargetingFindTargetsBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CustomTargetingFindTargetsBP_NotFound", "Function 'CustomTargetingFindTargetsBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    
    return result;
}
#endif

UAblTargetingBlackboard::UAblTargetingBlackboard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblTargetingBlackboard::~UAblTargetingBlackboard()
{

}

void UAblTargetingBlackboard::FindTargets(UAblAbilityContext& Context) const
{
	if (UBlackboardComponent* SelfBlackboard = FAblAbilityUtilities::GetBlackboard(Context.GetSelfActor()))
	{
		TArray<TWeakObjectPtr<AActor>>& MutableTargets = Context.GetMutableTargetActors();
		for (const FName& BBKey : m_BlackboardKeys)
		{
			if (AActor* targetActor = Cast<AActor>(SelfBlackboard->GetValueAsObject(BBKey)))
			{
				MutableTargets.Add(targetActor);
			}
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UAblTargetingBlackboard::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_BlackboardKeys.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoBlackboardKeys", "No Blackboard keys defined for Blackboard targeting: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAblTargetingLocation::UAblTargetingLocation(const FObjectInitializer& ObjectInitializer)
	: Super (ObjectInitializer)
{

}

UAblTargetingLocation::~UAblTargetingLocation()
{

}

void UAblTargetingLocation::FindTargets(UAblAbilityContext& Context) const
{
	if (Context.GetTargetLocation().SizeSquared() < SMALL_NUMBER)
	{
		Context.SetTargetLocation(Context.GetAbility()->GetTargetLocationBP(&Context));
	}
}

#if WITH_EDITOR
EDataValidationResult UAblTargetingLocation::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;    
    return result;
}
#endif
