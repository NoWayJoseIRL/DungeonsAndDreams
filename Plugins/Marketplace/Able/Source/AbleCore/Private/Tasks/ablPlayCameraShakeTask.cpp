// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPlayCameraShakeTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "GameFramework/PlayerController.h"

#if (!UE_BUILD_SHIPPING)
#include "ablAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblPlayerCameraShakeTaskScratchPad::UAblPlayerCameraShakeTaskScratchPad()
{
}

UAblPlayerCameraShakeTaskScratchPad::~UAblPlayerCameraShakeTaskScratchPad()
{
}

UAblPlayerCameraShakeTask::UAblPlayerCameraShakeTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , Shake(nullptr)
    , ShakeScale(1.0f)
    , ShakeInWorldSpace(false)
    , StopMode(EAblPlayCameraShakeStopMode::Stop)
{
}

UAblPlayerCameraShakeTask::~UAblPlayerCameraShakeTask()
{
}

void UAblPlayerCameraShakeTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

    UAblPlayerCameraShakeTaskScratchPad* ScratchPad = Cast<UAblPlayerCameraShakeTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	ScratchPad->Controllers.Empty();
	ScratchPad->ShakeClasses.Empty();

    TArray<TWeakObjectPtr<AActor>> TargetArray;
    GetActorsForTask(Context, TargetArray);

	TSubclassOf<UCameraShakeBase> ShakeClass = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, Shake);

	if (*ShakeClass == nullptr)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context,TEXT("Invalid Shake Class was supplied."));
		}
#endif
		return;
	}

	float ShakeScaleVal = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, ShakeScale);
	bool UseWorldSpace = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, ShakeInWorldSpace);

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
					ScratchPad->ShakeClasses.Add(ShakeClass);
                    
                    playerController->ClientStartCameraShake(ShakeClass, ShakeScaleVal, UseWorldSpace ? ECameraShakePlaySpace::World : ECameraShakePlaySpace::CameraLocal);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Controller {0} Played Camera Shake {1}"), { playerController->GetName(), ShakeClass->GetName() }));
                    }
#endif
                }
            }
        }
    }
}

void UAblPlayerCameraShakeTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	EAblPlayCameraShakeStopMode StopModeVal = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, StopMode);

	if (!Context.IsValid() || (StopModeVal == EAblPlayCameraShakeStopMode::DontStop))
	{
		return;
	}

	UAblPlayerCameraShakeTaskScratchPad* ScratchPad = Cast<UAblPlayerCameraShakeTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	check(ScratchPad->Controllers.Num() == ScratchPad->ShakeClasses.Num());

    for (int i = 0; i < ScratchPad->Controllers.Num(); ++i)
    {
		TWeakObjectPtr<APlayerController>& PC = ScratchPad->Controllers[i];
		TSubclassOf<UCameraShakeBase>& CamShake = ScratchPad->ShakeClasses[i];
        if (PC.IsValid())
        {
            PC->ClientStopCameraShake(CamShake, StopModeVal == EAblPlayCameraShakeStopMode::StopImmediately);

#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Format(TEXT("Controller {0} Played Camera Shake {1}"), { PC->GetName(), CamShake->GetName() }));
            }
#endif
        }
    }
}

UAblAbilityTaskScratchPad* UAblPlayerCameraShakeTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblPlayerCameraShakeTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblPlayerCameraShakeTaskScratchPad>(Context.Get());
}

TStatId UAblPlayerCameraShakeTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblPlayerCameraShakeTask, STATGROUP_Able);
}

void UAblPlayerCameraShakeTask::BindDynamicDelegates(UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, Shake, TEXT("Shake"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, ShakeScale, TEXT("Shake Scale"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, ShakeInWorldSpace, TEXT("Shake World Space"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, StopMode, TEXT("Stop Immediately on Exit"));
}

#if WITH_EDITOR

FText UAblPlayerCameraShakeTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlayerCameraShakeTaskFormat", "{0}: {1}");
    FString TargetName = FString::Format(TEXT("({0})"), { Shake ? Shake->GetName() : TEXT("<null>") });
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

#endif

#undef LOCTEXT_NAMESPACE