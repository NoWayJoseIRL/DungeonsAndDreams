// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablOverlapWatcherTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "ablSettings.h"
#include "Engine/World.h"
#include "Tasks/ablValidation.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblOverlapWatcherTaskScratchPad::UAblOverlapWatcherTaskScratchPad()
    : AsyncHandle()
    , TaskComplete(false)
	, HasClearedInitialTargets(false)
{
}

UAblOverlapWatcherTaskScratchPad::~UAblOverlapWatcherTaskScratchPad()
{
}

UAblOverlapWatcherTask::UAblOverlapWatcherTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , m_FireEvent(false)
    , m_Name(NAME_None)
    , m_CopyResultsToContext(false)
    , m_AllowDuplicateEntries(false)
	, m_ClearExistingTargets(false)
	, m_ContinuallyClearTargets(false)
    , m_TaskRealm(EAblAbilityTaskRealm::ATR_ClientAndServer)
{
}

UAblOverlapWatcherTask::~UAblOverlapWatcherTask()
{
}

void UAblOverlapWatcherTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAblOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
	ScratchPad->AsyncHandle._Handle = 0;
	ScratchPad->TaskComplete = false;
	ScratchPad->HasClearedInitialTargets = false;
	ScratchPad->IgnoreActors.Empty();

    CheckForOverlaps(Context);
}

void UAblOverlapWatcherTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
    Super::OnTaskTick(Context, deltaTime);

    CheckForOverlaps(Context);
}

void UAblOverlapWatcherTask::CheckForOverlaps(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
    UAblOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAblOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
    check(ScratchPad);
    
    if (m_QueryShape)
    {
        // Step 1: Handle the last async query first
        if (m_QueryShape->IsAsync() && UAbleSettings::IsAsyncEnabled())
        {
            UWorld* QueryWorld = m_QueryShape->GetQueryWorld(Context);
            check(QueryWorld);

            if (QueryWorld->IsTraceHandleValid(ScratchPad->AsyncHandle, true))
            {
                FOverlapDatum Datum;
                if (QueryWorld->QueryOverlapData(ScratchPad->AsyncHandle, Datum))
                {
                    TArray<FAblQueryResult> Results;
                    m_QueryShape->ProcessAsyncOverlaps(Context, ScratchPad->QueryTransform, Datum.OutOverlaps, Results);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Printf(TEXT("Query found %d results."), Results.Num()));
                    }
#endif
                    ProcessResults(Results, Context);
                }
                else
                {
                    // Still a valid query pending, don't do another one
                    return;
                }
            }
        }

        // Do the next query
        if (m_QueryShape->IsAsync() && UAbleSettings::IsAsyncEnabled())
        {
            ScratchPad->AsyncHandle = m_QueryShape->DoAsyncQuery(Context, ScratchPad->QueryTransform);
        }
        else
        {
            TArray<FAblQueryResult> Results;
            m_QueryShape->DoQuery(Context, Results);

#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Printf(TEXT("Query found %d results."), Results.Num()));
            }
#endif

			ProcessResults(Results, Context);
        }
    }
}

bool UAblOverlapWatcherTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
    UAblOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAblOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
    check(ScratchPad);

    if (ScratchPad->TaskComplete)
	{
        return true;
	}

    return UAblAbilityTask::IsDone(Context);
}

UAblAbilityTaskScratchPad* UAblOverlapWatcherTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblOverlapWatcherTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

    return NewObject<UAblOverlapWatcherTaskScratchPad>(Context.Get());
}

TStatId UAblOverlapWatcherTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblOverlapWatcherTask, STATGROUP_Able);
}


void UAblOverlapWatcherTask::CopyResultsToContext(const TArray<FAblQueryResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context, bool ClearTargets) const
{
    if (UAblAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
    {
        TArray<TWeakObjectPtr<AActor>> AdditionTargets;
        AdditionTargets.Reserve(InResults.Num());
        for (const FAblQueryResult& Result : InResults)
        {
            AdditionTargets.Add(Result.Actor);
        }
        AbilityComponent->AddAdditionTargetsToContext(Context, AdditionTargets, m_AllowDuplicateEntries, ClearTargets);
    }
}


void UAblOverlapWatcherTask::ProcessResults(TArray<FAblQueryResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
    UAblOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAblOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
    check(ScratchPad);

    for (const UAblCollisionFilter* CollisionFilter : m_Filters)
    {
        CollisionFilter->Filter(Context, InResults);

#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), InResults.Num()));
        }
#endif
    }

	// We either haven't called clear targets at all yet, or we have some results and we want to continually call clear.
	bool NeedsClearCall = m_CopyResultsToContext && ( (m_ClearExistingTargets && !ScratchPad->HasClearedInitialTargets) || (InResults.Num() && m_ContinuallyClearTargets ) );

    // We could have filtered out all our entries, so check again if the array is empty.
    if (InResults.Num() || NeedsClearCall )
    {
        if (m_CopyResultsToContext)
        {
#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), InResults.Num()));
            }
#endif
            CopyResultsToContext(InResults, Context, NeedsClearCall);

			ScratchPad->HasClearedInitialTargets = true;
        }

        if (m_FireEvent)
        {
#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), InResults.Num()));
            }
#endif
            switch (Context->GetAbility()->OnCollisionEventBP(Context.Get(), m_Name, InResults))
            {
            case EAblCallbackResult::Complete:
                ScratchPad->TaskComplete = true;
                break;
            case EAblCallbackResult::IgnoreActors:
                for (FAblQueryResult& Result : InResults)
				{
                    ScratchPad->IgnoreActors.Add(Result.Actor);
				}
                break;
            }
        }
    }
}

#if WITH_EDITOR

FText UAblOverlapWatcherTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblWatchForOverlapTaskFormat", "{0}");
	return FText::FormatOrdered(FormatText, GetTaskName());
}

FText UAblOverlapWatcherTask::GetRichTextTaskSummary() const
{
    FTextBuilder StringBuilder;

    StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

    FString Summary = m_QueryShape != nullptr ? m_QueryShape->DescribeShape() : TEXT("NULL");
    StringBuilder.AppendLineFormat(LOCTEXT("AblWatchForOverlapTaskRichFmt", "\t- WatchForOverlap Shape: {0}"), FText::FromString(Summary));

    return StringBuilder.ToText();
}

EDataValidationResult UAblOverlapWatcherTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    for (const UAblCollisionFilter* CollisionFilter : m_Filters)
    {
        if (CollisionFilter == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullCollisionFilter", "Null Collision Filter: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }

    if (m_QueryShape != nullptr && m_QueryShape->IsTaskDataValid(AbilityContext, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
	{
        result = EDataValidationResult::Invalid;
	}

    if (m_QueryShape == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryShape", "No Query Shape Defined: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    else if (m_QueryShape->GetQueryLocation().GetSourceTargetType() == EAblAbilityTargetType::ATT_TargetActor)
    {
        if (AbilityContext->RequiresTarget())
        {
            // trust that the ability cannot run unless it has a target, so don't do any dependency validation
            if (AbilityContext->GetTargeting() == nullptr)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoTargeting", "No Targeting method Defined: {0} with RequiresTarget"), AssetName));
                result = EDataValidationResult::Invalid;
            }
        }
        else if (AbilityContext->GetTargeting() == nullptr)
        {
            // if we have a target actor, we should have a dependency task that copies results
            bool hasValidDependency = UAbleValidation::CheckDependencies(GetTaskDependencies());
            if (!hasValidDependency)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryDependency", "Trying to use Target Actor but there's no Ability Targeting or Query( with GetCopyResultsToContext )and a Dependency Defined on this Task: {0}. You need one of those conditions to properly use this target."), AssetName));
                result = EDataValidationResult::Invalid;
            }
        }
    }
    
    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnCollisionEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnCollisionEventBP_NotFound", "Function 'OnCollisionEventBP' not found: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }

    return result;
}

void UAblOverlapWatcherTask::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
    if (m_QueryShape != nullptr)
    {
        m_QueryShape->OnAbilityEditorTick(Context, DeltaTime);
    }
}

#endif

#undef LOCTEXT_NAMESPACE
