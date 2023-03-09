// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "UObject/ObjectMacros.h"

#include "BTDecorator_IsInAbilityRange.generated.h"

/**
* IsInAbilityRange decorator node.
* A decorator node that bases its condition on whether the distance between Point A & B is less than or equal to the range of the Ability.
*
*/
class UAblAbility;

UCLASS()
class ABLECORE_API UBTDecorator_IsInAbilityRange : public UBTDecorator
{
	GENERATED_UCLASS_BODY()
public:
	/* Returns true if the Actor is within range of the provided Ability. */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	/* Returns the Description of this Decorator. */
	virtual FString GetStaticDescription() const override;
protected:
    const UAblAbility* GetAbility(const UBlackboardComponent* BlackboardComp) const;

	UPROPERTY(EditAnywhere, Category = Ability,
	Meta = (ToolTips = "Which Actor, or vector, (from the blackboard) should be used as Point A"))
	struct FBlackboardKeySelector PointA;

	UPROPERTY(EditAnywhere, Category = Ability,
	Meta = (ToolTips = "Which Actor, or vector, (from the blackboard) should be used as Point B"))
	struct FBlackboardKeySelector PointB;

	/* The Ability to pull the range information from. */
	UPROPERTY(EditAnywhere, Category = Ability)
	TSubclassOf<UAblAbility> Ability;

	/* The Ability to check for from a blackboard key. */
	UPROPERTY(EditAnywhere, Category = Ability)
	struct FBlackboardKeySelector AbilityKey;

	UPROPERTY(EditAnywhere, Category = Ability,
	Meta = (ToolTips = "What value we should use as the range for this Ability. This needs to be a float value in the blackboard."))
	struct FBlackboardKeySelector RangeOverrideKey;

	/* If set to anything besides 0.0f, this value is substituted as the range of Ability. If the RangeOverride blackboard key is set, we'll use that instead.*/
	UPROPERTY(EditAnywhere, Category = Ability)
	float RangeOverrideValue;

	/* If true, we'll use a 2D distance check (XY Plane).*/
	UPROPERTY(EditAnywhere, Category = Ability)
	bool XYDistance;


	/* Initialize this Decorator. */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
