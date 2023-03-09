// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Animation/AnimNode_AbilityAnimPlayer.h"

#include "AbleCorePrivate.h"

#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSyncScope.h"

FAnimNode_AbilityAnimPlayer::FAnimNode_AbilityAnimPlayer()
	: m_CachedOutputSequence(nullptr),
	m_CachedOutputTime(0.0f),
	m_AnimationQueue(),
	m_BlendType(EvaluateBlendType::Single)
{

}

float FAnimNode_AbilityAnimPlayer::GetCurrentAssetTime() const
{
	if (m_AnimationQueue.Num())
	{
		return m_AnimationQueue[0].TimeAccumulator;
	}

	return 0.0f;
}

float FAnimNode_AbilityAnimPlayer::GetCurrentAssetLength() const
{
	if (m_AnimationQueue.Num() && m_AnimationQueue[0].AnimationSequence)
	{
		return m_AnimationQueue[0].AnimationSequence->GetPlayLength();
	}

	return 0.0f;
}

float FAnimNode_AbilityAnimPlayer::GetCurrentAssetTimePlayRateAdjusted() const
{
	if (m_AnimationQueue.Num() && m_AnimationQueue[0].AnimationSequence)
	{
		const float SequencePlayRate = m_AnimationQueue[0].AnimationSequence->RateScale;
		const float FinalPlayRate = m_AnimationQueue[0].PlayRate * SequencePlayRate;
		return FinalPlayRate < 0.0f ? GetCurrentAssetLength() - m_AnimationQueue[0].TimeAccumulator : m_AnimationQueue[0].TimeAccumulator;
	}

	return 0.0f;
}

void FAnimNode_AbilityAnimPlayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);
}

void FAnimNode_AbilityAnimPlayer::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);

	float BlendValue = 1.0f;
	m_BlendType = EvaluateBlendType::Single;

	// Handle popping off any entries first.
	if (m_AnimationQueue.Num())
	{
		// Remove the entry if we've:

		// Finished our animation.
		bool popEntry = m_AnimationQueue[0].GetTimeRemaining() <= 0.0f;
		
		if (m_AnimationQueue.Num() > 1)
		{
			if (m_AnimationQueue[1].BlendIn.GetBlendTimeRemaining() <= 0.0f)
			{
				// We have just another entry waiting.
				popEntry = true;
			}
			else
			{
				// Or we have another entry, with blend in, and we've finished the blend in.
				popEntry |= m_AnimationQueue[1].BlendIn.IsComplete();
			}
		}

		if (popEntry)
		{
			m_AnimationQueue.RemoveAt(0);
		}
	}

	// Now, Update normally.
	if (m_AnimationQueue.Num())
	{
		FAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
		FAbilityAnimEntry* NextEntry = m_AnimationQueue.Num() > 1 ? &m_AnimationQueue[1] : nullptr;

		CurrentEntry.UpdateEntry(Context.GetDeltaTime());
		if (NextEntry)
		{
			NextEntry->UpdateEntry(Context.GetDeltaTime());
		}

		float TimeRemaining = CurrentEntry.GetTimeRemaining();

		// Find out what Blend we're using, if any. 
		if (NextEntry)
		{
			NextEntry->BlendIn.Update(Context.GetDeltaTime());
			BlendValue -= NextEntry->BlendIn.GetAlpha();

			m_BlendType = EvaluateBlendType::Multi;
		}
		else if (CurrentEntry.GetTimeRemaining() <= CurrentEntry.BlendOut.GetBlendTimeRemaining()) 
		{
			CurrentEntry.BlendOut.Update(Context.GetDeltaTime());
			BlendValue -= CurrentEntry.BlendOut.GetAlpha();

			// Turning this off for now, need to think about WHAT to blend to. Likely need an animation layer of some sort...
			//m_BlendType = EvaluateBlendType::SingleBlendOut;
		}

		if (!Context.AnimInstanceProxy->IsSkeletonCompatible(CurrentEntry.AnimationSequence->GetSkeleton()))
		{
			UE_LOG(LogAble, Warning, TEXT("Invalid Skeleton Asset %s for Animation Sequence %s"), *(Context.AnimInstanceProxy->GetSkeleton()->GetName()), *(CurrentEntry.AnimationSequence->GetName()));
			return;
		}

		UE::Anim::FAnimSyncGroupScope& SyncScope = Context.GetMessageChecked<UE::Anim::FAnimSyncGroupScope>();
		FAnimTickRecord TickRecord(const_cast<UAnimSequence*>(CurrentEntry.AnimationSequence), false, CurrentEntry.PlayRate, BlendValue, CurrentEntry.TimeAccumulator, CurrentEntry.MarkerTickRecord);
		TickRecord.DeltaTimeRecord = &CurrentEntry.DeltaTimeRecord;
		TickRecord.GatherContextData(Context);
		SyncScope.AddTickRecord(TickRecord);

		switch (m_BlendType)
		{
			default:
			case EvaluateBlendType::Single:
			case EvaluateBlendType::SingleBlendOut:
			break;
			case EvaluateBlendType::Multi:
			{
				if (Context.AnimInstanceProxy->IsSkeletonCompatible(NextEntry->AnimationSequence->GetSkeleton()))
				{
					FAnimTickRecord NextTickRecord(const_cast<UAnimSequence*>(NextEntry->AnimationSequence), false, NextEntry->PlayRate, 1.0f - BlendValue, NextEntry->TimeAccumulator, NextEntry->MarkerTickRecord);
					NextTickRecord.DeltaTimeRecord = &(NextEntry->DeltaTimeRecord);
					NextTickRecord.GatherContextData(Context);

					SyncScope.AddTickRecord(NextTickRecord);
				}
				else
				{
					UE_LOG(LogAble, Warning, TEXT("Invalid Skeleton Asset %s for Animation Sequence %s"), *(Context.AnimInstanceProxy->GetSkeleton()->GetName()), *(NextEntry->AnimationSequence->GetName()));
				}
			}
			break;
		}
	}
}

void FAnimNode_AbilityAnimPlayer::Evaluate_AnyThread(FPoseContext& Output)
{
	check(Output.AnimInstanceProxy);
	FAnimInstanceProxy* Proxy = Output.AnimInstanceProxy;
	FAnimationPoseData PoseData(Output);
	if (!m_AnimationQueue.Num())
	{
		if (m_CachedOutputSequence)
		{
			m_CachedOutputSequence->GetAnimationPose(PoseData, FAnimExtractContext(m_CachedOutputTime, Output.AnimInstanceProxy->ShouldExtractRootMotion()));
		}
		else
		{
			Output.ResetToRefPose();
		}

		return;
	}

	switch (m_BlendType)
	{
		case EvaluateBlendType::Single:
		{
			FAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
			CurrentEntry.AnimationSequence->GetAnimationPose(PoseData, FAnimExtractContext(CurrentEntry.TimeAccumulator, Output.AnimInstanceProxy->ShouldExtractRootMotion()));
		}
		break;
		case EvaluateBlendType::Multi:
		case EvaluateBlendType::SingleBlendOut:
		{
			FAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
			FAbilityAnimEntry* NextEntry = m_AnimationQueue.Num() > 1 ? &m_AnimationQueue[1] : nullptr;

			FCompactPose Poses[2];
			FBlendedCurve Curves[2];
			UE::Anim::FStackAttributeContainer Attribs[2];
			float Weights[2] = { 0.0f };

			FAnimationPoseData PoseA(Poses[0], Curves[0], Attribs[0]);
			FAnimationPoseData PoseB(Poses[1], Curves[1], Attribs[1]);

			const FBoneContainer& RequiredBone = Proxy->GetRequiredBones();
			Poses[0].SetBoneContainer(&RequiredBone);
			Poses[1].SetBoneContainer(&RequiredBone);

			Curves[0].InitFrom(RequiredBone);
			Curves[1].InitFrom(RequiredBone);

			float AlphaValue = 0.0f;

			if (NextEntry)
			{
				AlphaValue += NextEntry->BlendIn.GetAlpha();
			}

			Weights[0] = 1.0f - AlphaValue;
			Weights[1] = AlphaValue;

			FAnimationPoseData AnimationPoseData(Output);
			FAnimationRuntime::BlendPosesTogether(Poses, Curves, Attribs, Weights, AnimationPoseData);
		}
		break;
		default:
			checkNoEntry();
			break;
	}

	if (m_AnimationQueue.Num())
	{
		m_CachedOutputSequence = m_AnimationQueue[0].AnimationSequence;
		m_CachedOutputTime = m_AnimationQueue[0].TimeAccumulator;
	}
}

void FAnimNode_AbilityAnimPlayer::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugData.AddDebugItem(DebugLine, true);
}

void FAnimNode_AbilityAnimPlayer::PlayAnimationSequence(const UAnimSequence* Animation, float PlayRate,const FAlphaBlend& BlendIn, const FAlphaBlend& BlendOut)
{
	check(Animation);

	m_AnimationQueue.Add(FAbilityAnimEntry(Animation, BlendIn, BlendOut, PlayRate));
}

void FAnimNode_AbilityAnimPlayer::OnAbilityInterrupted(bool clearQueue)
{
	if (clearQueue)
	{
		m_AnimationQueue.Empty();
		m_CachedOutputSequence = nullptr;
		m_CachedOutputTime = 0.0f;
	}
	else if (m_AnimationQueue.Num() != 0)
	{
		m_AnimationQueue.RemoveAt(0);
	}
}

void FAnimNode_AbilityAnimPlayer::SetAnimationTime(float NewTime)
{
	if (m_AnimationQueue.Num())
	{
		m_AnimationQueue[0].TimeAccumulator = NewTime;
	}
}

void FAnimNode_AbilityAnimPlayer::ResetInternalTimeAccumulator()
{
	if (m_AnimationQueue.Num())
	{
		FAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
		if (CurrentEntry.AnimationSequence && (CurrentEntry.PlayRate * CurrentEntry.AnimationSequence->RateScale) < 0.0f)
		{
			CurrentEntry.TimeAccumulator = CurrentEntry.AnimationSequence->GetPlayLength();
		}
		else
		{
			CurrentEntry.TimeAccumulator = 0.0f;
		}
	}
}

FAbilityAnimEntry::FAbilityAnimEntry()
	: AnimationSequence(nullptr),
	TimeAccumulator(0.0f),
	BlendIn(),
	BlendOut(),
	PlayRate(1.0f)
{
	BlendIn.Reset();
	BlendOut.Reset();
}

FAbilityAnimEntry::FAbilityAnimEntry(const UAnimSequence* InSequence, const FAlphaBlend& InBlend, const FAlphaBlend& OutBlend, float InPlayRate)
	: AnimationSequence(InSequence),
	TimeAccumulator(0.0f),
	BlendIn(InBlend),
	BlendOut(OutBlend),
	PlayRate(InPlayRate)
{
	BlendIn.Reset();
	BlendOut.Reset();
}

void FAbilityAnimEntry::UpdateEntry(float deltaTime)
{
	TimeAccumulator = FMath::Clamp(TimeAccumulator, 0.0f, AnimationSequence->GetPlayLength());
}

float FAbilityAnimEntry::GetTimeRemaining() const
{
	return FMath::Max<float>(AnimationSequence->GetPlayLength() - TimeAccumulator, 0.0f);
}
