// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "UObject/ObjectMacros.h"
#include "ablAbilityContext.h"
#include "BTTask_PlayAbility.generated.h"

class UAblAbility;
class UAblAbilityComponent;
class UBehaviorTreeComponent;

struct FBTPlayAbilityTaskMemory
{
	TWeakObjectPtr<UAblAbilityComponent> AbilityComponent;
	uint8 bAbilityPlayed : 1;
};


/**
* Play Ability Task Node
* Plays the specified ability when executed.
*/
UCLASS()
class ABLECORE_API UBTTask_PlayAbility : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()
	
	/* What Ability to Play. */
	UPROPERTY(Category = Ability, EditAnywhere)
	TSubclassOf<UAblAbility> Ability;

	/* The Ability to check for from a blackboard key. */
	UPROPERTY(EditAnywhere, Category = Ability)
	struct FBlackboardKeySelector AbilityKey;

	/* If true, the Task will mark itself as in progress while the Ability is playing, otherwise it will immediately succeed once the Ability is started. */
	UPROPERTY(Category = Ability, EditAnywhere)
	bool MarkAsInProgressDuringExecution;

	/* Executes the Task.*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/* Handle Task Ticking if we're marked as In Progress.*/
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/* Finish the Task (Reset Task memory). */
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	
	/* Returns a Description of the Task.*/
	virtual FString GetStaticDescription() const override;

	/* Describes the Task at runtime based on the current state. */
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;

	/* Returns the Size of our Instance Memory. */
	virtual uint16 GetInstanceMemorySize() const override;

	/* Initialize this Task. */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	/* UObject Override. */
	virtual void PostLoad() override;
#if WITH_EDITOR
	/* Returns the Node Icon Name. */
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
private:
    const UAblAbility* GetAbility(const UBlackboardComponent* BlackboardComp) const;

	UPROPERTY(Transient)
    const UAblAbility *m_CachedAbility;    
};