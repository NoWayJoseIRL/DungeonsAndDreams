// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AlphaBlend.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNode_AssetPlayerBase.h"
#include "Templates/SharedPointer.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "AnimNode_AbilityAnimPlayer.generated.h"

class UAnimSequence;

USTRUCT()
struct ABLECORE_API FAbilityAnimEntry
{
	GENERATED_BODY()
	FAbilityAnimEntry();
	FAbilityAnimEntry(const UAnimSequence* InSequence, const FAlphaBlend& InBlend, const FAlphaBlend& OutBlend, float InPlayRate);
public:
	void UpdateEntry(float deltaTime);
	float GetTimeRemaining() const;

	UPROPERTY()
	const UAnimSequence* AnimationSequence;

	float TimeAccumulator;

	FDeltaTimeRecord DeltaTimeRecord;

	FMarkerTickRecord MarkerTickRecord;

	FAlphaBlend BlendIn;

	FAlphaBlend BlendOut;

	float PlayRate;
};

/* A Special Animation State Node that allows the Play Animation Task to feed in Animations at runtime, it extends the AssetPlayer node.*/
USTRUCT()
struct ABLECORE_API FAnimNode_AbilityAnimPlayer : public FAnimNode_AssetPlayerBase
{
	GENERATED_BODY()

	FAnimNode_AbilityAnimPlayer();

public:

	// FAnimNode_AssetPlayerBase interface
	virtual float GetCurrentAssetTime() const override;
	virtual float GetCurrentAssetLength() const  override;
	virtual float GetCurrentAssetTimePlayRateAdjusted() const override;
	// End of FAnimNode_AssetPlayerBase interface

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	/* Plays the specified Animation Sequence.*/
	void PlayAnimationSequence(const UAnimSequence* Animation, float PlayRate, const FAlphaBlend& BlendIn, const FAlphaBlend& BlendOut);
	
	/* Special logic to play when interrupts occur.*/
	void OnAbilityInterrupted(bool clearQueue = false);

	/* Returns true if our Sequence has been set. It'll be reset once the clip is completed. */
	bool HasAnimationToPlay() const { return m_AnimationQueue.Num() > 0; }

	/* Allows the direct setting of the Internal Time value. */
	void SetAnimationTime(float NewTime);

private:
	enum EvaluateBlendType
	{
		Single,
		SingleBlendOut,
		Multi,
	};

	/* Helper Method to Reset Internal Time Accumulators.*/
	void ResetInternalTimeAccumulator();

	/* This is the last valid sequence we played, used in blending if we have any updates after we've reset our sequence to avoid any T-Posing. */
	const UAnimSequence* m_CachedOutputSequence;

	float m_CachedOutputTime;

	UPROPERTY()
	TArray<FAbilityAnimEntry> m_AnimationQueue;

	EvaluateBlendType m_BlendType;
};
