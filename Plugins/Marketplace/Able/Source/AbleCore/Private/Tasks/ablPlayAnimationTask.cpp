// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPlayAnimationTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNode_AbilityAnimPlayer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimStateMachineTypes.h"
#include "Animation/AnimNode_StateMachine.h"

#include "Components/SkeletalMeshComponent.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblPlayAnimationTaskScratchPad::UAblPlayAnimationTaskScratchPad()
{

}

UAblPlayAnimationTaskScratchPad::~UAblPlayAnimationTaskScratchPad()
{

}

UAblPlayAnimationTask::UAblPlayAnimationTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_AnimationAsset(nullptr),
	m_AnimationMontageSection(NAME_None),
	m_OnEndAnimationMontageSection(NAME_None),
	m_AnimationMode(EAblPlayAnimationTaskAnimMode::SingleNode),
	m_StateMachineName(),
	m_AbilityStateName(),
	m_Loop(false),
	m_SlotName(TEXT("Ability")),
	m_TimeToStartMontageAt(0.0f),
	m_BlendOutTriggerTime(-1.0f),
	m_NumberOfLoops(1),
	m_StopAllMontages(false),
	m_PlayRate(1.0f),
	m_ScaleWithAbilityPlayRate(true),
	m_StopAnimationOnInterrupt(true),
	m_ClearQueuedAnimationOnInterrupt(false),
	m_ResetAnimationStateOnEnd(true),
	m_ManuallySpecifyAnimationLength(false),
	m_ManualLengthIsInterrupt(true),
	m_EventName(NAME_None),
	m_PlayOnServer(false)
{

}

UAblPlayAnimationTask::~UAblPlayAnimationTask()
{

}

void UAblPlayAnimationTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	const UAnimationAsset* AnimationAsset = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AnimationAsset);

	if (!AnimationAsset)
	{
		UE_LOG(LogAble, Warning, TEXT("No Animation set for PlayAnimationTask in Ability [%s]"), *Context->GetAbility()->GetDisplayName());
		return;
	}

	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	UAblPlayAnimationTaskScratchPad* ScratchPad = CastChecked<UAblPlayAnimationTaskScratchPad>(Context->GetScratchPadForTask(this));
	ScratchPad->AbilityComponents.Empty();
	ScratchPad->SingleNodeSkeletalComponents.Empty();

	float BasePlayRate = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_PlayRate);
	float PlayRate = BasePlayRate * (m_ScaleWithAbilityPlayRate ? Context->GetAbility()->GetPlayRate(Context.Get()) : 1.0f);
	FName MontageSection = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AnimationMontageSection);

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (Target.IsValid())
		{
			if (USkeletalMeshComponent* PreferredComponent = Context->GetAbility()->GetSkeletalMeshComponentForActor(Context.Get(), Target.Get(), m_EventName))
			{
				PlayAnimation(Context, AnimationAsset, MontageSection, *Target.Get(), *ScratchPad, *PreferredComponent, PlayRate);
			}
			else
			{
				TInlineComponentArray<USkeletalMeshComponent*> InSkeletalComponents(Target.Get());

				for (UActorComponent* SkeletalComponent : InSkeletalComponents)
				{
					PlayAnimation(Context, AnimationAsset, MontageSection, *Target.Get(), *ScratchPad, *Cast<USkeletalMeshComponent>(SkeletalComponent), PlayRate);
				}
			}
		}
	}
}

void UAblPlayAnimationTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (!Context.IsValid())
	{
		return;
	}

	UAblPlayAnimationTaskScratchPad* ScratchPad = CastChecked<UAblPlayAnimationTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	
	// Reset any Single Node instances that were previous AnimBlueprint mode.
	for (TWeakObjectPtr<USkeletalMeshComponent>& SkeletalComponent : ScratchPad->SingleNodeSkeletalComponents)
	{
		if (m_ResetAnimationStateOnEnd)
		{
			SkeletalComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}

		// If we're in Animation Node mode, and our Ability was interrupted - tell the Animation Node.
		if (m_StopAnimationOnInterrupt && (result == EAblAbilityTaskResult::Interrupted || (m_ManuallySpecifyAnimationLength && m_ManualLengthIsInterrupt) ) )
		{
			switch (m_AnimationMode)
			{
			case EAblPlayAnimationTaskAnimMode::AbilityAnimationNode:
			{
				if (FAnimNode_AbilityAnimPlayer* AnimPlayer = GetAbilityAnimGraphNode(Context, SkeletalComponent.Get()))
				{
					AnimPlayer->OnAbilityInterrupted(m_ClearQueuedAnimationOnInterrupt);
				}
			}
			break;
			case EAblPlayAnimationTaskAnimMode::DynamicMontage:
			{
				if (UAnimInstance* Instance = SkeletalComponent->GetAnimInstance())
				{
					FName MontageOnEnd = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_OnEndAnimationMontageSection);
					if (MontageOnEnd != NAME_None)
					{
						Instance->Montage_JumpToSection(MontageOnEnd);
					}
					else
					{
						Instance->Montage_Stop(m_DynamicMontageBlend.m_BlendOut);
					}
				}
			}
			break;
			case EAblPlayAnimationTaskAnimMode::SingleNode:
			{
				if (UAnimSingleNodeInstance* SingleNode = SkeletalComponent->GetSingleNodeInstance())
				{
					SingleNode->StopAnim();
				}
			}
			break;
			default:
				checkNoEntry();
				break;
			}

		}
	}
}


bool UAblPlayAnimationTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	float PlayRate = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_PlayRate);
	return Context->GetCurrentTime() > GetEndTime() * PlayRate;
}

float UAblPlayAnimationTask::GetEndTime() const
{
	if (m_ManuallySpecifyAnimationLength)
	{
		return Super::GetEndTime();
	}

	float PlayRate = 1.0f; // Assume a flat playrate. 
	if (const UAnimMontage* Montage = Cast<UAnimMontage>(m_AnimationAsset))
	{
        if (m_AnimationMontageSection != NAME_None)
        {
            int32 sectionIndex = Montage->GetSectionIndex(m_AnimationMontageSection);
            if (sectionIndex != INDEX_NONE)
            {
                const FCompositeSection& section = Montage->CompositeSections[sectionIndex];
                float sequenceLength = (Montage->GetSectionLength(sectionIndex) * (1.0f / PlayRate));
                
                float endTime = GetStartTime() + sequenceLength;
                return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
            }
        }

        float endTime = GetStartTime() + (const_cast<UAnimMontage*>(Montage)->GetPlayLength() * (1.0f / PlayRate));
        return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
	}
    else if (const UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(m_AnimationAsset))
    {
        float endTime = GetStartTime() + (const_cast<UAnimSequenceBase*>(Sequence)->GetPlayLength() * (1.0f / PlayRate));
        return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
    }
    
    // fallback
    float endTime = GetStartTime() + 1.0f;
    return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
}

UAblAbilityTaskScratchPad* UAblPlayAnimationTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblPlayAnimationTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblPlayAnimationTaskScratchPad>(Context.Get());
}

TStatId UAblPlayAnimationTask::GetStatId() const
{
	 RETURN_QUICK_DECLARE_CYCLE_STAT(UAblPlayAnimationTask, STATGROUP_Able);
}

void UAblPlayAnimationTask::SetAnimationAsset(UAnimationAsset* Animation)
{
	check(Animation->IsA<UAnimSequenceBase>() || Animation->IsA<UAnimMontage>());
	m_AnimationAsset = Animation;
}

void UAblPlayAnimationTask::PlayAnimation(const TWeakObjectPtr<const UAblAbilityContext>& Context, const UAnimationAsset* AnimationAsset, const FName& MontageSection, AActor& TargetActor, UAblPlayAnimationTaskScratchPad& ScratchPad, USkeletalMeshComponent& SkeletalMeshComponent, float PlayRate) const
{
	switch (m_AnimationMode.GetValue())
	{
		case EAblPlayAnimationTaskAnimMode::SingleNode:
		{
			// Make a note of these so we can reset to Animation Blueprint mode.
			if (SkeletalMeshComponent.GetAnimationMode() == EAnimationMode::AnimationBlueprint)
			{
				ScratchPad.SingleNodeSkeletalComponents.Add(&SkeletalMeshComponent);
			}

			if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(AnimationAsset))
			{
				if (UAnimInstance* Instance = SkeletalMeshComponent.GetAnimInstance())
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Montage %s (Section %s) on Target %s"), 
                            *MontageAsset->GetName(), *MontageSection.ToString(), *TargetActor.GetName()));
					}
#endif
					float StartMontageAt = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TimeToStartMontageAt);

					Instance->Montage_Play(const_cast<UAnimMontage*>(MontageAsset), PlayRate, EMontagePlayReturnType::MontageLength, StartMontageAt, ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_StopAllMontages));

                    if (MontageSection != NAME_None)
                    {
#if !(UE_BUILD_SHIPPING)
                        if (!MontageAsset->IsValidSectionName(MontageSection))
                        {
                            PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Montage %s (Section %s) on Target %s Invalid Montage Section"),
                                *MontageAsset->GetName(), *MontageSection.ToString(), *TargetActor.GetName()));
                        }
#endif

                        Instance->Montage_JumpToSection(MontageSection, MontageAsset);
                    }
				}
			}
			else if (UAnimSingleNodeInstance* SingleNode = SkeletalMeshComponent.GetSingleNodeInstance()) // See if we already have an instance instantiated.
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Animation %s on Target %s"), *AnimationAsset->GetName(), *TargetActor.GetName()));
				}
#endif
				SingleNode->SetAnimationAsset(const_cast<UAnimationAsset*>(AnimationAsset), m_Loop, PlayRate);
				SingleNode->PlayAnim();
			}
			else // Nope, start a new one.
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Animation %s on Target %s"), *AnimationAsset->GetName(), *TargetActor.GetName()));
				}
#endif
				SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationSingleNode);
				SkeletalMeshComponent.SetAnimation(const_cast<UAnimationAsset*>(AnimationAsset));
				SkeletalMeshComponent.SetPlayRate(PlayRate);
				SkeletalMeshComponent.Play(m_Loop);
			}
		}
		break;
		case EAblPlayAnimationTaskAnimMode::AbilityAnimationNode:
		{
			if (SkeletalMeshComponent.GetAnimationMode() != EAnimationMode::AnimationBlueprint)
			{
				SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationBlueprint);
			}

			if (UAnimInstance* Instance = SkeletalMeshComponent.GetAnimInstance())
			{
				const UAnimSequence* AnimationSequence = Cast<UAnimSequence>(AnimationAsset);
				if (AnimationSequence)
				{
					if (FAnimNode_AbilityAnimPlayer* AbilityPlayerNode = GetAbilityAnimGraphNode(Context, &SkeletalMeshComponent))
					{
#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Playing Animation %s on Target %s using Ability Animation Node"), *AnimationAsset->GetName(), *TargetActor.GetName()));
						}
#endif
						AbilityPlayerNode->PlayAnimationSequence(AnimationSequence, PlayRate, m_BlendIn, m_BlendOut);

						if (UAblAbilityComponent* AbilityComponent = TargetActor.FindComponentByClass<UAblAbilityComponent>())
						{
							ScratchPad.AbilityComponents.Add(AbilityComponent);

							AbilityComponent->SetAbilityAnimationNode(AbilityPlayerNode);
						}
					}
					else
					{
						UE_LOG(LogAble, Warning, TEXT("Failed to find Ability Animation Node using State Machine Name %s and Node Name %s"), *m_StateMachineName.ToString(), *m_AbilityStateName.ToString());
					}
				}
				else
				{
					UE_LOG(LogAble, Warning, TEXT("Ability Animation Node only supports Animation Sequences. Unable to play %s"), *AnimationAsset->GetName());
				}
			}
		}
		break;
		case EAblPlayAnimationTaskAnimMode::DynamicMontage:
		{
			if (SkeletalMeshComponent.GetAnimationMode() != EAnimationMode::AnimationBlueprint)
			{
				SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationBlueprint);
			}

			if (UAnimInstance* Instance = SkeletalMeshComponent.GetAnimInstance())
			{
				float StartMontageAt = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TimeToStartMontageAt);
				float BlendOutTimeAt = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_BlendOutTriggerTime);
				int32 NumLoops = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_NumberOfLoops);
				FName SlotName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SlotName);

				if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(AnimationAsset))
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Playing Montage Animation %s on Target %s using Dynamic Montage"), *MontageAsset->GetName(), *TargetActor.GetName()));
					}
#endif
					Instance->Montage_Play(const_cast<UAnimMontage*>(MontageAsset), PlayRate, EMontagePlayReturnType::MontageLength, StartMontageAt, ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_StopAllMontages));
				}
				else if (const UAnimSequenceBase* SequenceAsset = Cast<UAnimSequenceBase>(AnimationAsset))
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Playing Slot Animation %s on Target %s on Slot %s using Dynamic Montage"), *SequenceAsset->GetName(), *TargetActor.GetName(), *SlotName.ToString()));
					}
#endif
					FAblBlendTimes DynamicMontageBlend = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_DynamicMontageBlend);
					Instance->PlaySlotAnimationAsDynamicMontage(const_cast<UAnimSequenceBase*>(SequenceAsset), SlotName, DynamicMontageBlend.m_BlendIn, DynamicMontageBlend.m_BlendOut, PlayRate, NumLoops, BlendOutTimeAt, StartMontageAt);
				}
			}
		}
		break;
		default:
		{
			checkNoEntry();
		}
		break;
	}

	if (m_StopAnimationOnInterrupt)
	{
		ScratchPad.SingleNodeSkeletalComponents.Add(&SkeletalMeshComponent);
	}
}

FAnimNode_AbilityAnimPlayer* UAblPlayAnimationTask::GetAbilityAnimGraphNode(const TWeakObjectPtr<const UAblAbilityContext>& Context, USkeletalMeshComponent* MeshComponent) const
{
	if (UAnimInstance* Instance = MeshComponent->GetAnimInstance())
	{
		FAnimInstanceProxy InstanceProxy(Instance);
		FName StateMachineName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_StateMachineName);
		FName AbilityStateName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AbilityStateName);

		const FAnimNode_StateMachine* StateMachineNode = InstanceProxy.GetStateMachineInstanceFromName(StateMachineName);
		if (StateMachineNode)
		{
			const FBakedAnimationStateMachine* BakedStateMachine = InstanceProxy.GetMachineDescription(InstanceProxy.GetAnimClassInterface(), StateMachineNode);

			if (BakedStateMachine)
			{
				for (const FBakedAnimationState& State : BakedStateMachine->States)
				{
					if (State.StateName == AbilityStateName)
					{
						for (const int32& PlayerNodeIndex : State.PlayerNodeIndices)
						{
							FAnimNode_AbilityAnimPlayer* AbilityPlayerNode = (FAnimNode_AbilityAnimPlayer*)InstanceProxy.GetNodeFromIndexUntyped(PlayerNodeIndex, FAnimNode_AbilityAnimPlayer::StaticStruct());
							if (AbilityPlayerNode)
							{
								return AbilityPlayerNode;
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}

void UAblPlayAnimationTask::OnAbilityTimeSet(const TWeakObjectPtr<const UAblAbilityContext>& Context)
{
	UAblPlayAnimationTaskScratchPad* ScratchPad = CastChecked<UAblPlayAnimationTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad)

	switch (m_AnimationMode.GetValue())
	{
	case EAblPlayAnimationTaskAnimMode::AbilityAnimationNode:
	{
		// Reset any Single Node instances that were previous AnimBlueprint mode.
		for (TWeakObjectPtr<USkeletalMeshComponent>& SkeletalComponent : ScratchPad->SingleNodeSkeletalComponents)
		{
			if (FAnimNode_AbilityAnimPlayer* AbilityNode = GetAbilityAnimGraphNode(Context, SkeletalComponent.Get()))
			{
				AbilityNode->SetAnimationTime(Context->GetCurrentTime() - GetStartTime());
			}
		}
	}
	case EAblPlayAnimationTaskAnimMode::SingleNode:
	{
		for (TWeakObjectPtr<USkeletalMeshComponent>& SkeletalComponent : ScratchPad->SingleNodeSkeletalComponents)
		{
            if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(m_AnimationAsset))
            {
                if (UAnimInstance* Instance = SkeletalComponent->GetAnimInstance())
                {
                    if (m_AnimationMontageSection != NAME_None)
                    {
						FName MontageName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AnimationMontageSection);
                        FAnimMontageInstance* MontageInstance = Instance->GetActiveInstanceForMontage(MontageAsset);
                        if (MontageInstance)
                        {
                            bool const bEndOfSection = (MontageInstance->GetPlayRate() < 0.f);
                            if (MontageInstance->JumpToSectionName(MontageName, bEndOfSection))
                            {
								const int32 SectionID = MontageAsset->GetSectionIndex(MontageName);

                                if (MontageAsset->IsValidSectionIndex(SectionID))
                                {
                                    FCompositeSection & CurSection = const_cast<UAnimMontage*>(MontageAsset)->GetAnimCompositeSection(SectionID);
                                    MontageInstance->SetPosition(CurSection.GetTime() + Context->GetCurrentTime() - GetStartTime());
                                }
                            }
                        }
                    }
                    else
                    {
                        Instance->Montage_SetPosition(MontageAsset, Context->GetCurrentTime() - GetStartTime());
                    }
                }
            }
            else
            {
                if (UAnimSingleNodeInstance* SingleNode = SkeletalComponent->GetSingleNodeInstance())
                {
                    SingleNode->SetPosition(Context->GetCurrentTime() - GetStartTime());
                }
            }
		}
	}
	break;
	default:
		break;
	}
}

void UAblPlayAnimationTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AnimationAsset, TEXT("Animation"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_DynamicMontageBlend, TEXT("Play Blend"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_PlayRate, TEXT("Play Rate"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TimeToStartMontageAt, TEXT("Time To Start At"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_BlendOutTriggerTime, TEXT("Blend Out Trigger Time"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_NumberOfLoops, TEXT("Number Of Loops"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_StopAllMontages, TEXT("Stop All Montages"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SlotName, TEXT("Slot Name"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_StateMachineName, TEXT("State Machine Name"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AbilityStateName, TEXT("Ability State Name"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AnimationMontageSection, TEXT("Montage Section"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_OnEndAnimationMontageSection, TEXT("Montage Section on End"));
}

#if WITH_EDITOR

FText UAblPlayAnimationTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlayAnimationTaskFormat", "{0}: {1}");
	FString AnimationName = TEXT("<null>");
	if (m_AnimationAsset)
	{
        AnimationName = m_AnimationAsset->GetName();

        if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(m_AnimationAsset))
        {
            if (m_AnimationMontageSection != NAME_None)
            {
                if(MontageAsset->IsValidSectionName(m_AnimationMontageSection))
                    AnimationName = FString::Format(TEXT("{0}({1})"), { m_AnimationAsset->GetName(), m_AnimationMontageSection.ToString() });
                else
                    AnimationName = FString::Format(TEXT("{0}({1})"), { m_AnimationAsset->GetName(), TEXT("<InvalidSection>") });
            }
        }
	}
	else if (m_AnimationAssetDelegate.IsBound())
	{
		AnimationName = TEXT("Dynamic");
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AnimationName));
}

FText UAblPlayAnimationTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString AnimationName = TEXT("NULL");
	if (m_AnimationAssetDelegate.IsBound())
	{
		AnimationName = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AnimationAssetDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AnimationName = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_AnimationAsset\" Filter=\"/Script/Engine.AnimationAsset\">{0}</>"), { m_AnimationAsset ? m_AnimationAsset->GetName() : AnimationName });
	}
	StringBuilder.AppendLineFormat(LOCTEXT("AblPlayAnimationTaskRichFmt", "\t- Animation: {0}"), FText::FromString(AnimationName));

	return StringBuilder.ToText();
}

EDataValidationResult UAblPlayAnimationTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    return result;
}

#endif

#undef LOCTEXT_NAMESPACE

