// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPlayForcedFeedbackTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ForceFeedbackEffect.h"

#if (!UE_BUILD_SHIPPING)
#include "ablAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblPlayForcedFeedbackTaskScratchPad::UAblPlayForcedFeedbackTaskScratchPad()
{

}

UAblPlayForcedFeedbackTaskScratchPad::~UAblPlayForcedFeedbackTaskScratchPad()
{

}

UAblPlayForcedFeedbackTask::UAblPlayForcedFeedbackTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , ForceFeedbackEffect(nullptr)
    , StartTagName()
    , bLooping(false)
    , bIgnoreTimeDilation(true)
    , bPlayWhilePaused(false)
    , StopTagName()
    , StopOnTaskExit(true)
{
}

UAblPlayForcedFeedbackTask::~UAblPlayForcedFeedbackTask()
{
}

void UAblPlayForcedFeedbackTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

    if (ForceFeedbackEffect == nullptr)
	{
        return;
	}

    UAblPlayForcedFeedbackTaskScratchPad* ScratchPad = Cast<UAblPlayForcedFeedbackTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	ScratchPad->Controllers.Empty();

    TArray<TWeakObjectPtr<AActor>> TargetArray;
    GetActorsForTask(Context, TargetArray);

    for (TWeakObjectPtr<AActor>& Target : TargetArray)
    {
        if (Target.IsValid())
        {
            APawn* pawn = Cast<APawn>(Target);
            if (IsValid(pawn))
            {
                APlayerController* playerController = Cast<APlayerController>(pawn->GetController());
                if (IsValid(playerController))
                {
                    ScratchPad->Controllers.Add(playerController);
                    
                    FForceFeedbackParameters parameters;
                    parameters.Tag = StartTagName;
                    parameters.bLooping = bLooping;
                    parameters.bIgnoreTimeDilation = bIgnoreTimeDilation;
                    parameters.bPlayWhilePaused = bPlayWhilePaused;
                    playerController->ClientPlayForceFeedback(ForceFeedbackEffect, parameters);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Controller {0} Played Force Feedback {1}"), { playerController->GetName(), ForceFeedbackEffect->GetName() }));
                    }
#endif
                }
            }
        }
    }
}

void UAblPlayForcedFeedbackTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (!Context.IsValid() || !StopOnTaskExit)
	{
		return;
	}

	UAblPlayForcedFeedbackTaskScratchPad* ScratchPad = Cast<UAblPlayForcedFeedbackTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

    for (TWeakObjectPtr<APlayerController>& PC : ScratchPad->Controllers)
    {
        if (PC.IsValid())
        {
            PC->ClientStopForceFeedback(ForceFeedbackEffect, StopTagName);

#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Format(TEXT("Controller {0} Played Force Feedback {1}"), { PC->GetName(), ForceFeedbackEffect->GetName() }));
            }
#endif
        }
    }
}

UAblAbilityTaskScratchPad* UAblPlayForcedFeedbackTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblPlayForcedFeedbackTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblPlayForcedFeedbackTaskScratchPad>(Context.Get());
}

TStatId UAblPlayForcedFeedbackTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblPlayForcedFeedbackTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblPlayForcedFeedbackTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlayForcedFeedbackTaskFormat", "{0}: {1}");
    FString TargetName = FString::Format(TEXT("({0})"), { ForceFeedbackEffect ? ForceFeedbackEffect->GetName() : TEXT("<null>") });
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

#endif

#undef LOCTEXT_NAMESPACE