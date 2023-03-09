// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AITypes.h"
#include "AI/Navigation/NavigationTypes.h"
#include "NavigationSystemTypes.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablMoveToTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblMoveToScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblMoveToScratchPad();
	virtual ~UAblMoveToScratchPad();

	FVector CurrentTargetLocation;

	TArray<TPair<uint32, TWeakObjectPtr<APawn>>> AsyncQueryIdArray;
	TArray<TPair<FAIRequestID, TWeakObjectPtr<APawn>>> ActiveMoveRequests;
	TArray<TWeakObjectPtr<AActor>> ActivePhysicsMoves;
	TArray<TPair<uint32, FNavPathSharedPtr>> CompletedAsyncQueries;

	void OnNavPathQueryFinished(uint32 Id, ENavigationQueryResult::Type typeData, FNavPathSharedPtr PathPtr);
	FNavPathQueryDelegate NavPathDelegate;
};

UENUM()
enum EAblMoveToTarget
{
	MTT_Actor UMETA(DisplayName = "Actor"),
	MTT_Location UMETA(DisplayName = "Location")
};

UENUM()
enum EAblPathFindingType // Navigation System's ENUM isn't exported, so we duplicate it.
{
	Regular UMETA(DisplayName = "Regular"),
	Hierarchical UMETA(DisplayName = "Hierarchical")
};

UCLASS()
class ABLECORE_API UAblMoveToTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblMoveToTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblMoveToTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	virtual bool NeedsTick() const { return true; }

	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;

	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const override;

	/* Returns if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return false; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	virtual float GetEndTime() const { return GetStartTime() + FMath::Max<float>(m_TimeOut, 0.5f); }

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	virtual bool IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAblAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblMoveToTaskCategory", "Movement"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblMoveToTask", "Move To"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblMoveToTaskDesc", "Moves an Actor from one location to another, either using physics or the nav mesh."); }

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(108.0f / 255.0f, 102.0 / 255.0f, 196.0f / 255.0f); }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif

protected:
	FVector GetTargetLocation(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;
	void StartPathFinding(const TWeakObjectPtr<const UAblAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAblMoveToScratchPad* ScratchPad) const;
	void SetPhysicsVelocity(const TWeakObjectPtr<const UAblAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAblMoveToScratchPad* ScratchPad) const;


	/* Which Target to move towards: Location, or Actor.*/
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Target Type"))
	TEnumAsByte<EAblMoveToTarget> m_TargetType;

	/* The Target Actor to move to. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Target Actor", EditCondition = "m_TargetType == EAblMoveToTarget::MTT_Actor"))
	TEnumAsByte<EAblAbilityTargetType> m_TargetActor;

	/* The Target Location to move to. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Target Location", EditCondition = "m_TargetType == EAblMoveToTarget::MTT_Location", AblBindableProperty))
	FVector m_TargetLocation;

	UPROPERTY()
	FGetAblVector m_TargetLocationDelegate;

	/* Whether or not to continually update our end point. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Update Target"))
	bool m_UpdateTargetPerFrame;

	/* How close we need to be to our Target for this task to be completed.*/
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Acceptable Radius", ClampMin = 0.0f))
	float m_AcceptableRadius;

	/* If true, ignore the Z axis when checking for new locations and if we're at our end goal. This is generally optimal and should be left on unless you need 3D path finding. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Use 2D Distance Checks", ClampMin = 0.0f))
	bool m_Use2DDistanceChecks;

	/* Whether or not to use Navigational Pathing or simple physics.*/
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Use Nav Pathing"))
	bool m_UseNavPathing;

	/* If using Nav pathing, whether to abort or not if no path can be found. */
	UPROPERTY(EditAnywhere, Category = "Move|NavPath", meta = (DisplayName = "Cancel On No Path", EditCondition = "m_UseNavPathing"))
	bool m_CancelOnNoPathAvailable;

	/* If using Nav pathing, which mode to use when finding a path. */
	UPROPERTY(EditAnywhere, Category = "Move|NavPath", meta = (DisplayName = "NavPath Finding Type", EditCondition = "m_UseNavPathing"))
	TEnumAsByte<EAblPathFindingType> m_NavPathFindingType;

	/* If true, use an Async Path Finding Query vs a blocking one.  */
	UPROPERTY(EditAnywhere, Category = "Move|NavPath", meta = (DisplayName = "Use Async NavPath Query", EditCondition = "m_UseNavPathing"))
	bool m_UseAsyncNavPathFinding;

	/* What our speed should be if using Physics to drive movement. */
	UPROPERTY(EditAnywhere, Category = "Move|Physics", meta = (DisplayName = "Speed", ClampMin = 0.0f, AblBindableProperty))
	float m_Speed;

	UPROPERTY()
	FGetAblFloat m_SpeedDelegate;

	/* Timeout for this Task. A value of 0.0 means there is no time out. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Timeout"))
	float m_TimeOut;

	/* If true, cancel our velocity when we're interrupted.  */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Cancel Move On Interrupt"))
	bool m_CancelMoveOnInterrupt;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE