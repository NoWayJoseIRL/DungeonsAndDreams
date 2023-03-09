// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablIgnoreInputTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"

#if (!UE_BUILD_SHIPPING)
#include "ablAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblIgnoreInputTaskScratchPad::UAblIgnoreInputTaskScratchPad()
{

}

UAblIgnoreInputTaskScratchPad::~UAblIgnoreInputTaskScratchPad()
{

}

UAblIgnoreInputTask::UAblIgnoreInputTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , m_MoveInput(true)
    , m_LookInput(true)
    , m_Input(false)
{
}

UAblIgnoreInputTask::~UAblIgnoreInputTask()
{
}

void UAblIgnoreInputTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

    if (!m_MoveInput && !m_LookInput && !m_Input)
	{
        return;
	}

    UAblIgnoreInputTaskScratchPad* ScratchPad = Cast<UAblIgnoreInputTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	ScratchPad->Pawns.Empty();

    TArray<TWeakObjectPtr<AActor>> TargetArray;
    GetActorsForTask(Context, TargetArray);

    for (TWeakObjectPtr<AActor>& Target : TargetArray)
    {
        if (Target.IsValid())
        {
            APawn* pawn = Cast<APawn>(Target);
            if (IsValid(pawn))
            {
                AController* controller = pawn->GetController();
                if (IsValid(controller))
                {
                    ScratchPad->Pawns.Add(pawn);

                    if (m_MoveInput)
                    {
                        controller->SetIgnoreMoveInput(true);

#if !(UE_BUILD_SHIPPING)
                        if (IsVerbose())
                        {
                            PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreMoveInput(+)"), { controller->GetName() }));
                        }
#endif
                    }

                    if(m_LookInput)
                    {
                        controller->SetIgnoreLookInput(true);

#if !(UE_BUILD_SHIPPING)
                        if (IsVerbose())
                        {
                            PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreLookInput(+)"), { controller->GetName() }));
                        }
#endif
                    }

                    if (m_Input)
                    {
                        pawn->DisableInput(nullptr);

#if !(UE_BUILD_SHIPPING)
                        if (IsVerbose())
                        {
                            PrintVerbose(Context, FString::Format(TEXT("Pawn {0} DisableInput(+)"), { pawn->GetName() }));
                        }
#endif
                    }
                }
            }
        }
    }
}

void UAblIgnoreInputTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (!Context.IsValid())
	{
		return;
	}

	UAblIgnoreInputTaskScratchPad* ScratchPad = Cast<UAblIgnoreInputTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

    for (TWeakObjectPtr<APawn>& Target : ScratchPad->Pawns)
    {
        if (Target.IsValid())
        {
            AController* controller = Target->GetController();
            if (IsValid(controller))
            {
                if (m_MoveInput)
                {
                    controller->SetIgnoreMoveInput(false);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreMoveInput(-)"), { Target->GetName() }));
                    }
#endif
                }

                if (m_LookInput)
                {
                    controller->SetIgnoreLookInput(false);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreLookInput(-)"), { Target->GetName() }));
                    }
#endif
                }

                if (m_Input)
                {
                    Target->EnableInput(nullptr);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Pawn {0} DisableInput(-)"), { Target->GetName() }));
                    }
#endif
                }
            }
        }
    }
}

UAblAbilityTaskScratchPad* UAblIgnoreInputTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblIgnoreInputTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblIgnoreInputTaskScratchPad>(Context.Get());
}

TStatId UAblIgnoreInputTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblIgnoreInputTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblIgnoreInputTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblTIgnoreInputTaskFormat", "{0}: {1}");
    FString TargetName = FString::Format(TEXT("Ignore({0} {1} {2})"), { m_MoveInput ? TEXT("Move") : TEXT(""), m_LookInput ? TEXT("Look") : TEXT(""), m_Input ? TEXT("Input") : TEXT("") });
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

void UAblIgnoreInputTask::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	
}

#endif

#undef LOCTEXT_NAMESPACE