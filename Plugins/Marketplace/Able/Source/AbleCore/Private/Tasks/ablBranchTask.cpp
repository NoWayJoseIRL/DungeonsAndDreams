// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablBranchTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Tasks/ablBranchCondition.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblBranchTaskScratchPad::UAblBranchTaskScratchPad()
    : BranchAbility(nullptr),
    BranchConditionsMet(false)
{
}

UAblBranchTaskScratchPad::~UAblBranchTaskScratchPad()
{

}

UAblBranchTask::UAblBranchTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_BranchAbility(nullptr),
	m_MustPassAllConditions(false),
	m_CopyTargetsOnBranch(false),
    m_BranchOnTaskEnd(false)
{

}

UAblBranchTask::~UAblBranchTask()
{

}

void UAblBranchTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblBranchTaskScratchPad* ScratchPad = Cast<UAblBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

    TSubclassOf<UAblAbility> Ability = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_BranchAbility);
    ScratchPad->BranchAbility = Ability.GetDefaultObject();
	ScratchPad->BranchConditionsMet = false;
	ScratchPad->CachedKeys.Empty();

	if (!ScratchPad->BranchAbility)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
            PrintVerbose(Context, FString::Printf(TEXT("Branch Ability is null.")));
		}
#endif
		return;
	}

	bool BranchOnTaskEnd = ABL_GET_DYNAMIC_PROPERTY_VALUE_THREE(Context, m_BranchOnTaskEnd, EAblAbilityTaskResult::Successful);
	if (CheckBranchCondition(Context))
	{
        ScratchPad->BranchConditionsMet = true;

        // deferred until task end
        if (BranchOnTaskEnd)
		{ 
            return;
		}

		InternalDoBranch(Context);
	}
}

void UAblBranchTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

    UAblBranchTaskScratchPad* ScratchPad = Cast<UAblBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
    check(ScratchPad);

    if (ScratchPad->BranchConditionsMet)
	{ 
        return;
	}

    TSubclassOf<UAblAbility> Ability = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_BranchAbility);
    ScratchPad->BranchAbility = Ability.GetDefaultObject();
    if (!ScratchPad->BranchAbility)
    {
#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Branch Ability is null.")));
        }
#endif
        return;
    }

	bool BranchOnTaskEnd = ABL_GET_DYNAMIC_PROPERTY_VALUE_THREE(Context, m_BranchOnTaskEnd, EAblAbilityTaskResult::Successful);
	if (CheckBranchCondition(Context))
	{
        ScratchPad->BranchConditionsMet = true;

        // deferred until task end
        if (BranchOnTaskEnd)
		{ 
            return;
		}

		InternalDoBranch(Context);
	}
}

void UAblBranchTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
    Super::OnTaskEnd(Context, result);

	bool BranchOnTaskEnd = ABL_GET_DYNAMIC_PROPERTY_VALUE_THREE(Context, m_BranchOnTaskEnd, result);
    if (BranchOnTaskEnd)
    {
        UAblBranchTaskScratchPad* ScratchPad = Cast<UAblBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
        check(ScratchPad);

        if (ScratchPad->BranchConditionsMet)
        {
			InternalDoBranch(Context);
        }
    }
}

UAblAbilityTaskScratchPad* UAblBranchTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblBranchTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblBranchTaskScratchPad>(Context.Get());
}

TStatId UAblBranchTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblBranchTask, STATGROUP_Able);
}

void UAblBranchTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_BranchAbility, TEXT("Ability"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_MustPassAllConditions, TEXT("Must Pass All Conditions"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_CopyTargetsOnBranch, TEXT("Copy Targets on Branch"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_BranchOnTaskEnd, TEXT("Branch on Task End"));
}

void UAblBranchTask::InternalDoBranch(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	UAblBranchTaskScratchPad* ScratchPad = Cast<UAblBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
    check(ScratchPad);
#if !(UE_BUILD_SHIPPING)
    if (IsVerbose())
    {
		PrintVerbose(Context, FString::Printf(TEXT("Conditions passed. Branching to Ability %s"), *(ScratchPad->BranchAbility->GetName())));
    }
#endif
	if (ScratchPad->BranchAbility)
    {
		UAblAbilityContext* NewContext = UAblAbilityContext::MakeContext(ScratchPad->BranchAbility, Context->GetSelfAbilityComponent(), Context->GetOwner(), Context->GetInstigator());
		bool CopyTargetsOnBranch = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_CopyTargetsOnBranch);
        if (CopyTargetsOnBranch)
        {
            NewContext->GetMutableTargetActors().Append(Context->GetTargetActors());
        }

        Context->GetSelfAbilityComponent()->BranchAbility(NewContext);
    }
#if !(UE_BUILD_SHIPPING)
    else if (IsVerbose())
    {
        PrintVerbose(Context, FString::Printf(TEXT("Branching Failed. Branch Ability was null.")));
    }
#endif
}

bool UAblBranchTask::CheckBranchCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	UAblBranchTaskScratchPad* ScratchPad = Cast<UAblBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

	bool MustPassAllConditions = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_MustPassAllConditions);
	EAblConditionResults Result = EAblConditionResults::ACR_Failed;
	for (UAblBranchCondition* Condition : m_Conditions)
	{
		if (!Condition)
		{
			continue;
		}

		Result = Condition->CheckCondition(Context, *ScratchPad);
		if (Condition->IsNegated())
		{
			if (Result == EAblConditionResults::ACR_Passed)
			{
				Result = EAblConditionResults::ACR_Failed;
			}
			else if (Result == EAblConditionResults::ACR_Failed)
			{
				Result = EAblConditionResults::ACR_Passed;
			}
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			FString ConditionResult = TEXT("Passed");
			if (Result == EAblConditionResults::ACR_Ignored)
			{
				ConditionResult = TEXT("Ignored");
			}
			else if (Result == EAblConditionResults::ACR_Failed)
			{
				ConditionResult = TEXT("Failed");
			}
			PrintVerbose(Context, FString::Printf(TEXT("Condition %s returned %s."), *Condition->GetName(), *ConditionResult));
		}
#endif

		// Check our early out cases.
		if (MustPassAllConditions && Result == EAblConditionResults::ACR_Failed)
		{
			// Failed
			break;
		}
		else if (!MustPassAllConditions && Result == EAblConditionResults::ACR_Passed)
		{
			// Success
			break;
		}
	}

	return Result == EAblConditionResults::ACR_Passed;
}

#if WITH_EDITOR

FText UAblBranchTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblBranchTaskFormat", "{0}({1}): {2}");
	FString AbilityName = ("<null>");
	if (m_BranchAbilityDelegate.IsBound())
	{
		AbilityName = ("Dynamic");
	}
	else if (*m_BranchAbility)
	{
		if (UAblAbility* Ability = Cast<UAblAbility>(m_BranchAbility->GetDefaultObject()))
		{
			AbilityName = Ability->GetDisplayName();
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AbilityName));
}

FText UAblBranchTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString AbilityName = TEXT("NULL");
	if (m_BranchAbilityDelegate.IsBound())
	{
		AbilityName = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_BranchAbilityDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AbilityName = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_BranchAbility\" Filter=\"/Script/AbleCore.AblAbility\">{0}</>"), { m_BranchAbility ? m_BranchAbility->GetDefaultObjectName().ToString() : AbilityName });
	}
	StringBuilder.AppendLineFormat(LOCTEXT("AblBranchAbilityTaskRichFmt", "\t- Branch Ability: {0}"), FText::FromString(AbilityName));

	return StringBuilder.ToText();
}

EDataValidationResult UAblBranchTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    
    for (UAblBranchCondition* Condition : m_Conditions)
    {
		if (!Condition)
		{
			continue;
		}

        if (Condition->IsTaskDataValid(AbilityContext, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
		{
            result = EDataValidationResult::Invalid;
		}
    }
    return result;
}

bool UAblBranchTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	for (UAblBranchCondition* Condition : m_Conditions)
	{
		if (!Condition)
		{
			continue;
		}

		modified |= Condition->FixUpObjectFlags();
	}

	return modified;
}

#endif

#undef LOCTEXT_NAMESPACE
