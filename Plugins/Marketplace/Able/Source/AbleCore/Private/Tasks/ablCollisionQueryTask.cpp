// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCollisionQueryTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "ablSettings.h"
#include "Tasks/ablRayCastQueryTask.h"
#include "Tasks/ablCollisionQueryTask.h"
#include "Engine/World.h"
#include "Tasks/ablValidation.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCollisionQueryTaskScratchPad::UAblCollisionQueryTaskScratchPad()
	: AsyncHandle(),
	AsyncProcessed(false)
{

}

UAblCollisionQueryTaskScratchPad::~UAblCollisionQueryTaskScratchPad()
{

}

UAblCollisionQueryTask::UAblCollisionQueryTask(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_CopyResultsToContext(true),
	m_AllowDuplicateEntries(false),
	m_ClearExistingTargets(false),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_ClientAndServer)
{

}

UAblCollisionQueryTask::~UAblCollisionQueryTask()
{

}

void UAblCollisionQueryTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	if (m_QueryShape)
	{
		if (m_QueryShape->IsAsync() && UAbleSettings::IsAsyncEnabled())
		{
			UAblCollisionQueryTaskScratchPad* ScratchPad = Cast<UAblCollisionQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
			check(ScratchPad);
			ScratchPad->AsyncHandle = m_QueryShape->DoAsyncQuery(Context, ScratchPad->QueryTransform);
			ScratchPad->AsyncProcessed = false;
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

			if (Results.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
			{
				for (const UAblCollisionFilter* CollisionFilter : m_Filters)
				{
					CollisionFilter->Filter(Context, Results);

#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), Results.Num()));
					}
#endif
				}

				// We could have filtered out all our entries, so check again if the array is empty.
				if (Results.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
				{
					if (m_CopyResultsToContext)
					{

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), Results.Num()));
						}
#endif

						CopyResultsToContext(Results, Context);
					}

					if (m_FireEvent)
					{

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), Results.Num()));
						}
#endif

						Context->GetAbility()->OnCollisionEventBP(Context.Get(), m_Name, Results);
					}
				}

			}

		}
	}
}

void UAblCollisionQueryTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

	if (m_QueryShape && m_QueryShape->IsAsync() && UAbleSettings::IsAsyncEnabled())
	{
		UWorld* QueryWorld = m_QueryShape->GetQueryWorld(Context);
		check(QueryWorld);

		UAblCollisionQueryTaskScratchPad* ScratchPad = Cast<UAblCollisionQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);

		if (!ScratchPad->AsyncProcessed && QueryWorld->IsTraceHandleValid(ScratchPad->AsyncHandle, true))
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

				if (Results.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
				{
					for (const UAblCollisionFilter* CollisionFilter : m_Filters)
					{
						CollisionFilter->Filter(Context, Results);
#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), Results.Num()));
						}
#endif
					}

					if (Results.Num() || ( m_CopyResultsToContext && m_ClearExistingTargets ))
					{
						if (m_CopyResultsToContext)
						{
#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), Results.Num()));
							}
#endif
							CopyResultsToContext(Results, Context);
						}

						if (m_FireEvent)
						{
#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), Results.Num()));
							}
#endif
							Context->GetAbility()->OnCollisionEventBP(Context.Get(), m_Name, Results);
						}
					}
				}

				ScratchPad->AsyncProcessed = true;
			}
		}
	}
}

bool UAblCollisionQueryTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (m_QueryShape && m_QueryShape->IsAsync() && UAbleSettings::IsAsyncEnabled())
	{
		UAblCollisionQueryTaskScratchPad* ScratchPad = Cast<UAblCollisionQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		return ScratchPad->AsyncProcessed;
	}
	else
	{
		return UAblAbilityTask::IsDone(Context);
	}
}

UAblAbilityTaskScratchPad* UAblCollisionQueryTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if(m_QueryShape && m_QueryShape->IsAsync())
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblCollisionQueryTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblCollisionQueryTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblCollisionQueryTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblCollisionQueryTask, STATGROUP_Able);
}

void UAblCollisionQueryTask::CopyResultsToContext(const TArray<FAblQueryResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context) const
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

void UAblCollisionQueryTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	if (m_QueryShape)
	{
		m_QueryShape->BindDynamicDelegates(Ability);
	}

	for (UAblCollisionFilter* Filter : m_Filters)
	{
		if (Filter)
		{
			Filter->BindDynamicDelegates(Ability);
		}
	}
}

#if WITH_EDITOR

FText UAblCollisionQueryTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblCollisionQueryFormat", "{0}: {1}");
	FString ShapeDescription = TEXT("<none>");
	if (m_QueryShape)
	{
		ShapeDescription = m_QueryShape->DescribeShape();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ShapeDescription));
}

EDataValidationResult UAblCollisionQueryTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
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

void UAblCollisionQueryTask::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
    if (m_QueryShape != nullptr)
    {
        m_QueryShape->OnAbilityEditorTick(Context, DeltaTime);
    }
}

bool UAblCollisionQueryTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	if (m_QueryShape)
	{
		modified |= m_QueryShape->FixUpObjectFlags();
	}

	for (UAblCollisionFilter* Filter : m_Filters)
	{
		modified |= Filter->FixUpObjectFlags();
	}

	return modified;
}

#endif

#undef LOCTEXT_NAMESPACE