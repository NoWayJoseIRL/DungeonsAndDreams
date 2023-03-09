// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "UObject/ObjectMacros.h"

#include "BTDecorator_IsAbilityOnCooldown.generated.h"

/**
* IsAbilityOnCooldown decorator node.
* A decorator node that bases its condition on whether the specified Actor (in the blackboard) has a specific Ability on Cooldown.
* 
*/

class UAblAbility;

UCLASS()
class ABLECORE_API UBTDecorator_IsAbilityOnCooldown : public UBTDecorator
{
	GENERATED_UCLASS_BODY()
public:
	/* Returns true if the provided Ability is on cooldown. */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/* Returns the Description of the Decorator. */
	virtual FString GetStaticDescription() const override;
protected:
    const UAblAbility* GetAbility(const UBlackboardComponent* BlackboardComp) const;

	UPROPERTY(EditAnywhere, Category = Ability,
	Meta = (ToolTips = "Which Actor (from the blackboard) should be checked for the cooldown?"))
	struct FBlackboardKeySelector ActorToCheck;

	/* The Ability to check for.*/
	UPROPERTY(EditAnywhere, Category = Ability)
	TSubclassOf<UAblAbility> Ability;

	/* The Ability to check for from a blackboard key. */
	UPROPERTY(EditAnywhere, Category = Ability)
	struct FBlackboardKeySelector AbilityKey;

	/* Initialize this Decorator. */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
