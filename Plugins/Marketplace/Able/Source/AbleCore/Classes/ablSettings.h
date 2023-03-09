// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "ablSettings.generated.h"
/**
* Implements the settings for the Able Toolkit
*/
UCLASS(config = Engine, defaultconfig)
class ABLECORE_API UAbleSettings : public UObject
{
	GENERATED_BODY()

public:
	UAbleSettings(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleSettings();

	/* Returns true if Async is enabled (and allowed on this platform). */
	static bool IsAsyncEnabled();

	/* Returns true if Async is Enabled by the user. */
	FORCEINLINE bool GetEnableAsync() const { return m_EnableAsync; }

	/* Returns true if the Async Update for Abilities is allowed. */
	FORCEINLINE bool GetAllowAbilityAsyncUpdate() const { return m_AllowAsyncAbilityUpdate; }
	
	/* Returns true if the Async Cooldown Update is allowed. */
	FORCEINLINE bool GetAllowAsyncCooldownUpdate() const { return m_AllowAsyncCooldownUpdate; }

	/* Returns true if we should log all Ability failures. */
	FORCEINLINE bool GetLogAbilityFailures() const { return m_LogAbilityFailues; }
	
	/* Returns true if we should log all verbose output. */
	FORCEINLINE bool GetLogVerbose() const { return m_LogVerbose; }

	/* Returns true if we should echo our verbose output to screen. */
	FORCEINLINE bool GetEchoVerboseToScreen() const { return m_EchoVerboseToScreen; }

	/* Returns the lifetime of a Verbose screen message, in seconds. */
	FORCEINLINE float GetVerboseScreenLifetime() const { return m_VerboseScreenOutputLifetime; }

	/* Returns whether or not to force Ability Activation when coming from the server. */
	FORCEINLINE bool GetForceServerAbilityActivation() const { return m_ForceServerAbilityActivation; }

	/* Returns whether or not to make copies of our Inherited Task. */
	FORCEINLINE bool GetCopyInheritedTasks() const { return m_CopyInheritedTasks;  }

	/* Returns whether or not to always forward ability requests to the server first. */
	FORCEINLINE bool GetAlwaysForwardToServerFirst() const { return m_AlwaysForwardToServer; }

	/* Returns the Prediction tolerance for predicted Abilities. */
	FORCEINLINE uint32 GetPredictionTolerance() const { return m_PredictionTolerance; }

	/* Returns whether or not Scratchpad reuse/pooling is enabled. */
	FORCEINLINE bool GetAllowScratchPadReuse() const { return m_AllowScratchpadReuse; }

	/* Returns whether or not Ability Context reuse/pooling is enabled. */
	FORCEINLINE bool GetAllowAbilityContextReuse() const { return m_AllowAbilityContextReuse; }

	/* Returns the initial pool size if we're initializing the pool for the first time. */
	FORCEINLINE uint32 GetInitialContextPoolSize() const { return m_InitialPooledContextsSize; }

	/* Returns the Max Context pool size. */
	FORCEINLINE uint32 GetMaxContextPoolSize() const { return m_MaxPooledContextsSize; }

	/* Returns the Max ScratchPad pool size. */
	FORCEINLINE uint32 GetMaxScratchPadPoolSize() const { return m_MaxPooledScratchPadsSize; }
private:
	/* If true, Able will attempt to use Async options when available and hardware permits it. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta=(DisplayName="Enable Async"))
	bool m_EnableAsync;

	/* If true, allows abilities to use the async task graph to perform updates (when available). This can increase performance during heavy ability usage. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta=(DisplayName="Allow Async Ability Update", EditCondition=m_EnableAsync))
	bool m_AllowAsyncAbilityUpdate;

	/* If true, Ability components will launch a separate task to update their active cooldowns. This can increase performance during heavy ability usage. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Allow Async Cooldown Update", EditCondition = m_EnableAsync))
	bool m_AllowAsyncCooldownUpdate;

	/* If true, we write out Ability name and failure codes when they fail to play (cooldown, custom check, etc). */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Log Ability Failures"))
	bool m_LogAbilityFailues;

	/* If true, Abilities/Tasks marked as Verbose will echo their information to the log. Can be used with screen output. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Log Verbose Output"))
	bool m_LogVerbose;

	/* If true, Abilities/Tasks marked as Verbose will echo their information to the screen. Can be used with log output. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Verbose Output to Screen"))
	bool m_EchoVerboseToScreen;

	/* How long, in seconds, to display the Verbose output on the screen. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Verbose Screen Lifetime", EditCondition = m_EchoVerboseToScreen))
	float m_VerboseScreenOutputLifetime;

	/* If true, skip the "CanAbilityExecute" logic when the Server tells the client to play an Ability. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Force Server Ability Activation"))
	bool m_ForceServerAbilityActivation;

	/* If true, make copies of Inherited Tasks which can be modified. Otherwise, Inherited Tasks should be treated as a "one to many" object and be read only. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Copy Inherited Tasks"))
	bool m_CopyInheritedTasks;

	/* If true, we'll always forward a requested Ability to the server BEFORE checking if the client says it's successful. Turn this off if you want fewer RPCs and it doesn't hurt your gameplay. */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Always Forward To Server"))
	bool m_AlwaysForwardToServer;

	/* Leeway when determining if an Ability has played Locally or not. This is in "abilities played", so if you set it to 2, that means as long as your Ability played within 2 of the expected order, it's fine. Default is 0 (exact). */
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Prediction Tolerance"))
	uint32 m_PredictionTolerance;

	/* If true, Scratchpads are re-used. This can help with performance and memory fragmentation if you have lots of Abilities constantly firing.*/
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Allow Scratchpad Reuse"))
	bool m_AllowScratchpadReuse;

	/* If true, Ability Contexts are re-used. This can help with performance and memory fragmentation if you have lots of Abilities constantly firing.*/
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Allow Ability Context Reuse"))
	bool m_AllowAbilityContextReuse;

	/* The number of Contexts to create initially at world start. If set to 0, we spawn them on demand and re-use from there.*/
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Initial Context Pool Size"))
	uint32 m_InitialPooledContextsSize;

	/* The maximum number of Contexts to pool. You can use this value to prevent Able from holding on to too many Contexts if there's a sudden spike of Abilities. 0 = No limit. Only enable this if you see memory being an issue.*/
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Max Context Pool Size"))
	uint32 m_MaxPooledContextsSize;

	/* The maximum number of Scratchpads to pool. You can use this value to prevent Able from holding on to too many Scratchpads if there's a sudden spike of Abilities. 0 = No limit. Only enable this if you see memory being an issue.*/
	UPROPERTY(config, EditAnywhere, Category = Ability, meta = (DisplayName = "Max Scratchpad Pool Size"))
	uint32 m_MaxPooledScratchPadsSize;
};
