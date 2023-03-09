// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablSetCollisionChannelTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

class UPrimitiveComponent;

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblSetCollisionChannelTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblSetCollisionChannelTaskScratchPad();
	virtual ~UAblSetCollisionChannelTaskScratchPad();

	/* Map of Primitive Components to Collision Channels. */
	UPROPERTY(transient)
	TMap<TWeakObjectPtr<UPrimitiveComponent>, TEnumAsByte<ECollisionChannel>> PrimitiveToChannelMap;
};

UCLASS()
class UAblSetCollisionChannelTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblSetCollisionChannelTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblSetCollisionChannelTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return true; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_ClientAndServer; } // Client for Auth client, Server for AIs/Proxies.

	/* Creates the Scratchpad for this Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblSetCollisionChannelCategory", "Collision"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblSetCollisionChannelTask", "Set Collision Channel"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblSetCollisionChannelTaskDesc", "Sets the Collision Channel on any targets, can optionally restore the previous channel as well."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(83.0f / 255.0f, 214.0f / 255.0f, 93.0f / 255.0f); }
#endif

protected:
	/* The collision channel to set our Task targets to. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Channel"))
	TEnumAsByte<ECollisionChannel> m_Channel;

	/* If true, the original channel values will be restored when the Task ends. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Restore Channel On End"))
	bool m_RestoreOnEnd;
};

#undef LOCTEXT_NAMESPACE