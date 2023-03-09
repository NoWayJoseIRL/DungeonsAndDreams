// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AI/BTDecorator_IsPlayingAbility.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTDecorator_IsPlayingAbility::UBTDecorator_IsPlayingAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	NodeName = "Ability Playing Condition";

	// Accept only actors
	ActorToCheck.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsPlayingAbility, ActorToCheck), AActor::StaticClass());

	// Default to using Self Actor
	ActorToCheck.SelectedKeyName = FBlackboard::KeySelf;

	// For now, don't allow users to select any "Abort Observers", because it's currently not supported.
	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

bool UBTDecorator_IsPlayingAbility::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return false;
	}

	if (AActor* Actor = Cast<AActor>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(ActorToCheck.GetSelectedKeyID())))
	{
		if (UAblAbilityComponent* AbilityComponent = Actor->FindComponentByClass<UAblAbilityComponent>())
		{
			return AbilityComponent->IsPlayingAbility();
		}
	}

	return false;
}

FString UBTDecorator_IsPlayingAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: checks if the actor is playing an active ability."), *Super::GetStaticDescription());
}

void UBTDecorator_IsPlayingAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ActorToCheck.ResolveSelectedKey(*BBAsset);
	}
}
