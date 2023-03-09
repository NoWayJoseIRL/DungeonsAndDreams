// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCustomEventTask.h"

#include "ablAbility.h"
#include "AbleCorePrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCustomEventTask::UAblCustomEventTask(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_EventName(NAME_None)
{

}

UAblCustomEventTask::~UAblCustomEventTask()
{

}

void UAblCustomEventTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Firing Custom Event %s."), *m_EventName.ToString()));
	}
#endif

	// Call our parent.
	Context->GetAbility()->OnCustomEventBP(Context.Get(), m_EventName);
}

TStatId UAblCustomEventTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblCustomEventTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblCustomEventTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblCustomEventTaskFormat", "{0}: {1}");
	FString EventName = m_EventName.IsNone() ? ("<none>") : m_EventName.ToString();
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(EventName));
}

EDataValidationResult UAblCustomEventTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    
    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnCustomEventBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("OnCustomEventBP_NotFound", "Function 'OnCustomEventBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}

#endif

#undef LOCTEXT_NAMESPACE
