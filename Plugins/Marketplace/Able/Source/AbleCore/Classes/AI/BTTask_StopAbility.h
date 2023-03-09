// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "BTTask_StopAbility.generated.h"

class UAblAbility;
class UBehaviorTreeComponent;

/**
* Stop Ability Task Node
* Stops the active Ability (or the provided Ability) if it's currently being played. 
*/
UCLASS()
class ABLECORE_API UBTTask_StopAbility : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/* If true, cancel whatever Ability is the current Active Ability.*/
	UPROPERTY(Category = Ability, EditAnywhere)
	bool CancelActive;
	
	/* What Ability to Stop. */
	UPROPERTY(Category = Ability, EditAnywhere)
	TSubclassOf<UAblAbility> Ability;

	/* The Ability to check for from a blueprint key. */
	UPROPERTY(EditAnywhere, Category = Ability)
	struct FBlackboardKeySelector AbilityKey;

	/* What result to use when stopping the Ability. */
	UPROPERTY(Category = Ability, EditAnywhere)
	TEnumAsByte<EAblAbilityTaskResult> ResultToUse;

	/* Executes the Task.*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/* Returns a Description of the Task.*/
	virtual FString GetStaticDescription() const override;

	/* Initialize this Task. */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

#if WITH_EDITOR
	/* Returns the Node Icon Name. */
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
protected:
    const UAblAbility* GetAbility(const UBlackboardComponent* BlackboardComp) const;

};