// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "UObject/ObjectMacros.h"

#include "BTDecorator_IsPlayingAbility.generated.h"

/**
* IsPlayingAbility decorator node.
* A decorator node that bases its condition on whether the specified Actor (in the blackboard) is playing an (active) ability.
*
*/

UCLASS()
class ABLECORE_API UBTDecorator_IsPlayingAbility : public UBTDecorator
{
	GENERATED_UCLASS_BODY()
public:
	/* Returns True if the Actor is playing an Ability.*/
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	/* Returns the Description of the Decorator.*/
	virtual FString GetStaticDescription() const override;
protected:

	UPROPERTY(EditAnywhere, Category = Ability,
	Meta = (ToolTips = "Which Actor (from the blackboard) should be checked for the active ability?"))
	struct FBlackboardKeySelector ActorToCheck;

	/* Initialize this Decorator. */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
