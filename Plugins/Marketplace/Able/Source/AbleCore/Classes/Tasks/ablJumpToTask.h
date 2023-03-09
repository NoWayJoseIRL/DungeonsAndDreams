// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Components/AudioComponent.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablJumpToTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblJumpToScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblJumpToScratchPad();
	virtual ~UAblJumpToScratchPad();

	FVector CurrentTargetLocation;
	TArray<TWeakObjectPtr<APawn>> JumpingPawns;
};

UENUM()
enum EAblJumpToTarget
{
	JTT_Actor UMETA(DisplayName = "Actor"),
	JTT_Location UMETA(DisplayName = "Location")
};

UCLASS()
class ABLECORE_API UAblJumpToTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblJumpToTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblJumpToTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Tick our Task. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const override;

	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const override;

	/* Is our Task finished yet? */
	virtual bool IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;

	/* Returns if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return false; }

	/* Returns true if our Task is a single frame. */
	virtual bool IsSingleFrame() const { return false; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return m_TaskRealm; }

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAblAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblJumpToTaskCategory", "Movement"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblJumpToTask", "Jump To"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblJumpToTaskDesc", "Causes an actor to jump/leap, using physics and a target destination."); }

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(74.0 / 255.0f, 63.0 / 255.0f, 163.0f / 255.0f); }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif

protected:
	FVector GetTargetLocation(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;
	void SetPhysicsVelocity(const TWeakObjectPtr<const UAblAbilityContext>& Context, APawn* Target, const FVector& EndLocation, UAblJumpToScratchPad* ScratchPad, bool addToScratchPad = true) const;

	/* Which Target to jump towards: Location, or Actor.*/
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Type"))
	TEnumAsByte<EAblJumpToTarget> m_TargetType;

	/* The Target Actor to move to. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Actor", EditCondition = "m_TargetType == EAblJumpToTarget::JTT_Actor"))
	TEnumAsByte<EAblAbilityTargetType> m_TargetActor;

	/* Follow the actor, in mid air if need be to ensure we land near the actor.*/
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Track Actor", EditCondition = "m_TargetType == EAblJumpToTarget::JTT_Actor"))
	bool m_TrackActor;

	/* The Target Location to move to. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Location", EditCondition = "m_TargetType == EAblJumpToTarget::JTT_Location", AblBindableProperty))
	FVector m_TargetLocation;

	UPROPERTY()
	FGetAblVector m_TargetLocationDelegate;

	/* An offset, from our direction to our target, on where to land. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Offset"))
	float m_TargetActorOffset;

	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Jump Height", ClampMin = 0.0f, AblBindableProperty))
	float m_JumpHeight;

	UPROPERTY()
	FGetAblFloat m_JumpHeightDelegate;

	/* What our speed should be if using Physics to drive movement. */
	UPROPERTY(EditAnywhere, Category = "Move|Physics", meta = (DisplayName = "Speed", ClampMin = 0.0f, AblBindableProperty))
	float m_Speed;

	UPROPERTY()
	FGetAblFloat m_SpeedDelegate;

	/* How close we need to be to our Target for this task to be completed or when we need to update our target location.*/
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Acceptable Radius", ClampMin = 0.0f))
	float m_AcceptableRadius;

	/* If true, ignore the Z axis when checking for new locations and if we're at our end goal. This is generally optimal and should be left on unless you need 3D checks due to verticality. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Use 2D Distance Checks", ClampMin = 0.0f))
	bool m_Use2DDistanceChecks;

	/* If true, don't wait till we reach our goal. Set the jump velocity and then exit. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "End Task Immediately", ClampMin = 0.0f))
	bool m_EndTaskImmediately;

	/* If true, cancel our velocity when we're interrupted.  */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Cancel Jump On Interrupt"))
	bool m_CancelMoveOnInterrupt;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAblAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE