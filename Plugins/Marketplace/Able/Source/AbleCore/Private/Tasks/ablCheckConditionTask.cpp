// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCheckConditionTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Tasks/ablBranchCondition.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCheckConditionTaskScratchPad::UAblCheckConditionTaskScratchPad()
    : ConditionMet(false)
{

}

UAblCheckConditionTaskScratchPad::~UAblCheckConditionTaskScratchPad()
{

}

UAblCheckConditionTask::UAblCheckConditionTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , m_ConditionEventName()
{

}

UAblCheckConditionTask::~UAblCheckConditionTask()
{

}

bool UAblCheckConditionTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
    if (!Super::IsDone(Context))
	{
        return false;
	}

    UAblCheckConditionTaskScratchPad* ScratchPad = Cast<UAblCheckConditionTaskScratchPad>(Context->GetScratchPadForTask(this));
    check(ScratchPad);

    return ScratchPad->ConditionMet;
}

void UAblCheckConditionTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblCheckConditionTaskScratchPad* ScratchPad = Cast<UAblCheckConditionTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

    ScratchPad->ConditionMet = CheckCondition(Context, *ScratchPad);
	if (ScratchPad->ConditionMet)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Conditions passed. Condition met %s"), *(GetName())));
		}
#endif		
	}
}

void UAblCheckConditionTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

    UAblCheckConditionTaskScratchPad* ScratchPad = CastChecked<UAblCheckConditionTaskScratchPad>(Context->GetScratchPadForTask(this));
    check(ScratchPad);

    if (ScratchPad->ConditionMet)
	{
        return;
	}

	if (CheckCondition(Context, *ScratchPad))
	{
        ScratchPad->ConditionMet = true;

#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Conditions passed. Condition met %s"), *(GetName())));
        }
#endif
	}
}

UAblAbilityTaskScratchPad* UAblCheckConditionTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblCheckConditionTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblCheckConditionTaskScratchPad>(Context.Get());
}

TStatId UAblCheckConditionTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblCheckConditionTask, STATGROUP_Able);
}

bool UAblCheckConditionTask::CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, const UAblCheckConditionTaskScratchPad& ScratchPad) const
{
    return Context.Get()->GetAbility()->CheckCustomConditionEventBP(Context.Get(), m_ConditionEventName);
}

#if WITH_EDITOR

FText UAblCheckConditionTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblCheckConditionTask", "{0}: {1}");
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromName(m_ConditionEventName));
}

#endif

#undef LOCTEXT_NAMESPACE
