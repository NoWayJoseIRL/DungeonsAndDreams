// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablSetCollisionChannelResponseTask.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

class UPrimitiveComponent;

/* Helper struct that keeps track of a Primitive component, Channel, and Response.*/
USTRUCT()
struct FCollisionLayerResponseEntry
{
	GENERATED_USTRUCT_BODY()
public:
	FCollisionLayerResponseEntry() 
	: Primitive(nullptr),
	Channel(ECollisionChannel::ECC_Pawn),
	Response(ECollisionResponse::ECR_Overlap) {};
	FCollisionLayerResponseEntry(UPrimitiveComponent* InPrimitive, ECollisionChannel InChannel, ECollisionResponse InResponse)
		: Primitive(InPrimitive),
		Channel(InChannel),
		Response(InResponse)
	{ }
	TWeakObjectPtr<UPrimitiveComponent> Primitive;
	ECollisionChannel Channel;
	ECollisionResponse Response;
};

USTRUCT()
struct FCollisionChannelResponsePair
{
	GENERATED_USTRUCT_BODY();
public:
	FCollisionChannelResponsePair(): CollisionChannel(ECollisionChannel::ECC_Pawn), CollisionResponse(ECollisionResponse::ECR_Ignore) {};
	FCollisionChannelResponsePair(ECollisionChannel _channel, ECollisionResponse _resp) : 
	CollisionChannel(_channel), CollisionResponse(_resp) 
	{ }
	
	UPROPERTY(EditInstanceOnly, Category = "Collision")
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	UPROPERTY(EditInstanceOnly, Category = "Collision")
	TEnumAsByte<ECollisionResponse> CollisionResponse;
};

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblSetCollisionChannelResponseTaskScratchPad : public UAblAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblSetCollisionChannelResponseTaskScratchPad();
	virtual ~UAblSetCollisionChannelResponseTaskScratchPad();

	/* The original values of all the channels we've changed. */
	UPROPERTY(transient)
	TArray<FCollisionLayerResponseEntry> PreviousCollisionValues;
};

UCLASS()
class UAblSetCollisionChannelResponseTask : public UAblAbilityTask
{
	GENERATED_BODY()
public:
	UAblSetCollisionChannelResponseTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblSetCollisionChannelResponseTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const override;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const override;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return true; }

	/* Returns the realm this Task belongs to. */
	virtual EAblAbilityTaskRealm GetTaskRealm() const override { return EAblAbilityTaskRealm::ATR_ClientAndServer; } // Client for Auth client, Server for AIs/Proxies.

	/* Creates the Scratchpad for our Task. */
	virtual UAblAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblSetCollisionChannelResponseTaskCategory", "Collision"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblSetCollisionChannelResponseTask", "Set Collision Channel Response"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblSetCollisionChannelResponseTaskDesc", "Sets the Collision Channel (or all channels) Response on any targets, can optionally restore the previous channel responses."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(184.0f / 255.0f, 237.0f / 255.0f, 58.0f / 255.0f); }
#endif

protected:
	/* The Collision Channel to set the response for. -- DEPRECATED */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Channel"))
	TEnumAsByte<ECollisionChannel> m_Channel;

	/* The Response to change to.*/
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Response"))
	TEnumAsByte<ECollisionResponse> m_Response;

	/* The channels and responses to change. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName= "Channels"))
	TArray<FCollisionChannelResponsePair> m_Channels;

	/* If true, all channels will be set to this response. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Set All Channels"))
	bool m_SetAllChannelsToResponse;

	/* If true, all the original values will be restored when the Task ends. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Restore Response On End"))
	bool m_RestoreOnEnd;
};

#undef LOCTEXT_NAMESPACE