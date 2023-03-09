// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCollisionSweepTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "ablSettings.h"
#include "Tasks/ablValidation.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCollisionSweepTaskScratchPad::UAblCollisionSweepTaskScratchPad()
	: SourceTransform(FTransform::Identity),
	AsyncHandle(),
	AsyncProcessed(false)
{

}

UAblCollisionSweepTaskScratchPad::~UAblCollisionSweepTaskScratchPad()
{

}

UAblCollisionSweepTask::UAblCollisionSweepTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_CopyResultsToContext(true),
	m_AllowDuplicateEntries(false),
	m_ClearExistingTargets(false),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_ClientAndServer)
{

}

UAblCollisionSweepTask::~UAblCollisionSweepTask()
{

}

void UAblCollisionSweepTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	if (m_SweepShape)
	{
		UAblCollisionSweepTaskScratchPad* ScratchPad = Cast<UAblCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		m_SweepShape->GetQueryTransform(Context, ScratchPad->SourceTransform);
		ScratchPad->AsyncProcessed = false;
		ScratchPad->AsyncHandle._Handle = 0;
	}
}

void UAblCollisionSweepTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

	UAblCollisionSweepTaskScratchPad* ScratchPad = Cast<UAblCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	if (m_SweepShape && m_SweepShape->IsAsync() && 
		UAbleSettings::IsAsyncEnabled() && 
		UAblAbilityTask::IsDone(Context))
	{
		if (ScratchPad->AsyncHandle._Handle == 0)
		{
			// We're at the end of the our task, so we need to perform our sweep. If We're Async, we need to queue
			// the query up and then process it next frame.
			ScratchPad->AsyncHandle = m_SweepShape->DoAsyncSweep(Context, ScratchPad->SourceTransform);
		}
	}
}

void UAblCollisionSweepTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	TArray<FAblQueryResult> OutResults;
	if (m_SweepShape && Context.IsValid())
	{
		UAblCollisionSweepTaskScratchPad* ScratchPad = Cast<UAblCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);

		if (m_SweepShape->IsAsync() && UAbleSettings::IsAsyncEnabled())
		{
			m_SweepShape->GetAsyncResults(Context, ScratchPad->AsyncHandle, OutResults);
			ScratchPad->AsyncProcessed = true;
		}
		else
		{
			m_SweepShape->DoSweep(Context, ScratchPad->SourceTransform, OutResults);
		}
	}

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Sweep found %d results."), OutResults.Num()));
	}
#endif

	if (OutResults.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
	{
		for (const UAblCollisionFilter* CollisionFilter : m_Filters)
		{
			CollisionFilter->Filter(Context, OutResults);

#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), OutResults.Num()));
			}
#endif
		}

		if (OutResults.Num() || (m_CopyResultsToContext && m_ClearExistingTargets)) // Early out if we filtered everything out.
		{
			if (m_CopyResultsToContext)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), OutResults.Num()));
				}
#endif
				CopyResultsToContext(OutResults, Context);
			}

			if (m_FireEvent)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), OutResults.Num()));
				}
#endif
				Context->GetAbility()->OnCollisionEventBP(Context.Get(), m_Name, OutResults);
			}
		}
	}
}

UAblAbilityTaskScratchPad* UAblCollisionSweepTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblCollisionSweepTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblCollisionSweepTaskScratchPad>(Context.Get());
}

TStatId UAblCollisionSweepTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblCollisionSweepTask, STATGROUP_Able);
}

void UAblCollisionSweepTask::CopyResultsToContext(const TArray<FAblQueryResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (UAblAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		TArray<TWeakObjectPtr<AActor>> AdditionTargets;
		AdditionTargets.Reserve(InResults.Num());
		for (const FAblQueryResult& Result : InResults)
		{
			AdditionTargets.Add(Result.Actor);
		}
		AbilityComponent->AddAdditionTargetsToContext(Context, AdditionTargets, m_AllowDuplicateEntries, m_ClearExistingTargets);
	}
}

void UAblCollisionSweepTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	if (m_SweepShape)
	{
		m_SweepShape->BindDynamicDelegates(Ability);
	}

	for (UAblCollisionFilter* Filter : m_Filters)
	{
		if (Filter)
		{
			Filter->BindDynamicDelegates(Ability);
		}
	}
}

bool UAblCollisionSweepTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (m_SweepShape && m_SweepShape->IsAsync() && UAbleSettings::IsAsyncEnabled())
	{
		UAblCollisionSweepTaskScratchPad* ScratchPad = Cast<UAblCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		return ScratchPad->AsyncHandle._Handle != 0;
	}
	else
	{
		return UAblAbilityTask::IsDone(Context);
	}
}

#if WITH_EDITOR

FText UAblCollisionSweepTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblCollisionSweepFormat", "{0}: {1}");
	FString ShapeDescription = TEXT("<none>");
	if (m_SweepShape)
	{
		ShapeDescription = m_SweepShape->DescribeShape();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ShapeDescription));
}

EDataValidationResult UAblCollisionSweepTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    for (const UAblCollisionFilter* CollisionFilter : m_Filters)
    {
        if (CollisionFilter == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullCollisionFilter", "Null Collision Filter: {0}"), FText::FromString(AbilityContext->GetDisplayName())));
            result = EDataValidationResult::Invalid;
        }
    }

    if (m_SweepShape != nullptr && m_SweepShape->IsTaskDataValid(AbilityContext, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
        result = EDataValidationResult::Invalid;

    if (m_SweepShape == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoSweepShape", "No Sweep Shape Defined: {0}"), FText::FromString(AbilityContext->GetDisplayName())));
        result = EDataValidationResult::Invalid;
    }
    else if (m_SweepShape->GetSweepLocation().GetSourceTargetType() == EAblAbilityTargetType::ATT_TargetActor)
    {
        if (AbilityContext->RequiresTarget())
        {
            // trust that the ability cannot run unless it has a target, so don't do any dependency validation
            if (AbilityContext->GetTargeting() == nullptr)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoTargeting", "No Targeting method Defined: {0} with RequiresTarget"), FText::FromString(AbilityContext->GetDisplayName())));
                result = EDataValidationResult::Invalid;
            }
        }
        else if (AbilityContext->GetTargeting() == nullptr)
        {
            // if we have a target actor, we should have a dependency task that copies results
            bool hasValidDependency = UAbleValidation::CheckDependencies(GetTaskDependencies());
            if (!hasValidDependency)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryDependency", "Trying to use Target Actor but there's no Ability Targeting or Query( with GetCopyResultsToContext )and a Dependency Defined on this Task: {0}. You need one of those conditions to properly use this target."), FText::FromString(AbilityContext->GetDisplayName())));
                result = EDataValidationResult::Invalid;
            }
        }
    }

    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnCollisionEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnCollisionEventBP_NotFound", "Function 'OnCollisionEventBP' not found: {0}"), FText::FromString(AbilityContext->GetDisplayName())));
            result = EDataValidationResult::Invalid;
        }
    }

    return result;
}

void UAblCollisionSweepTask::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
    if (m_SweepShape != nullptr)
    {
        m_SweepShape->OnAbilityEditorTick(Context, DeltaTime);
    }
}

bool UAblCollisionSweepTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	if (m_SweepShape)
	{
		modified |= m_SweepShape->FixUpObjectFlags();
	}

	for (UAblCollisionFilter* Filter : m_Filters)
	{
		modified |= Filter->FixUpObjectFlags();
	}

	return modified;
}

#endif

#undef LOCTEXT_NAMESPACE