// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablRayCastQueryTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablAbilityDebug.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "ablSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "Tasks/ablValidation.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblRayCastQueryTaskScratchPad::UAblRayCastQueryTaskScratchPad()
	: AsyncHandle(),
	AsyncProcessed(false)
{

}

UAblRayCastQueryTaskScratchPad::~UAblRayCastQueryTaskScratchPad()
{

}

UAblRayCastQueryTask::UAblRayCastQueryTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Length(100.0f),
	m_OnlyReturnBlockingHit(false),
	m_ReturnFaceIndex(false),
	m_ReturnPhysicalMaterial(false),
	m_QueryLocation(),
	m_UseEndLocation(false),
	m_QueryEndLocation(),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_CopyResultsToContext(true),
	m_AllowDuplicateEntries(false),
	m_ClearExistingTargets(false),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server),
	m_UseAsyncQuery(false)
{

}

UAblRayCastQueryTask::~UAblRayCastQueryTask()
{

}

void UAblRayCastQueryTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	FAblAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);
	AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
    if (SourceActor == nullptr)
	{
        return;
	}

	UWorld* World = SourceActor->GetWorld();

	FTransform QueryTransform;
	QueryLocation.GetTransform(*Context.Get(), QueryTransform);

	float Length = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Length);
	const bool OnlyReturnBlockingHit = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_OnlyReturnBlockingHit);
	const bool ReturnFaceIndex = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_ReturnFaceIndex);
	const bool ReturnPhysMaterial = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_ReturnPhysicalMaterial);

	const FVector RayStart = QueryTransform.GetLocation();
	FVector RayEnd = RayStart + QueryTransform.GetRotation().GetForwardVector() * Length;

	if (m_UseEndLocation)
	{
		FAblAbilityTargetTypeLocation QueryEndLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryEndLocation);
		FTransform QueryEndTransform;
		QueryEndLocation.GetTransform(*Context.Get(), QueryEndTransform);
		RayEnd = QueryEndTransform.GetLocation();
	}

    FCollisionObjectQueryParams ObjectQuery;
    for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnFaceIndex = ReturnFaceIndex;
	QueryParams.bReturnPhysicalMaterial = ReturnPhysMaterial;

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Starting Raycast from %s to %s."), *RayStart.ToString(), *RayEnd.ToString()));
	}
#endif

	if (m_UseAsyncQuery && UAbleSettings::IsAsyncEnabled())
	{
		UAblRayCastQueryTaskScratchPad* ScratchPad = Cast<UAblRayCastQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		ScratchPad->AsyncProcessed = false;
		if (OnlyReturnBlockingHit)
		{
			ScratchPad->AsyncHandle = World->AsyncLineTraceByObjectType(EAsyncTraceType::Single, RayStart, RayEnd, ObjectQuery, QueryParams);
		}
		else
		{
			ScratchPad->AsyncHandle = World->AsyncLineTraceByObjectType(EAsyncTraceType::Multi, RayStart, RayEnd, ObjectQuery, QueryParams);
		}
	}
	else
	{
		TArray<FHitResult> HitResults;
		FHitResult TraceResult;
		if (OnlyReturnBlockingHit)
		{
			if (World->LineTraceSingleByObjectType(TraceResult, RayStart, RayEnd, ObjectQuery, QueryParams))
			{
				HitResults.Add(TraceResult);
			}
		}
		else
		{
			World->LineTraceMultiByObjectType(HitResults, RayStart, RayEnd, ObjectQuery, QueryParams);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Raycast found %d results."), HitResults.Num()));
		}
#endif
		// Run any filters.
		if (m_Filters.Num())
		{
			TArray<FAblQueryResult> QueryResults;
			QueryResults.Reserve(HitResults.Num());

			for (const FHitResult& Hit : HitResults)
			{
				QueryResults.Add(FAblQueryResult(Hit));
			}

			// Run filters.
			for (UAblCollisionFilter* Filter : m_Filters)
			{
				Filter->Filter(Context, QueryResults);
			}

			// Fix up our raw List.
			HitResults.RemoveAll([&](const FHitResult& RHS)
			{
				return !QueryResults.Contains(FAblQueryResult(RHS));
			});
		}

		if (HitResults.Num())
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				// Quick distance print help to see if we hit ourselves.
				float DistanceToBlocker = HitResults[HitResults.Num() - 1].Distance;
				PrintVerbose(Context, FString::Printf(TEXT("Raycast blocking hit distance: %4.2f."), DistanceToBlocker));
			}
#endif

			if (m_CopyResultsToContext)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), HitResults.Num()));
				}
#endif
				CopyResultsToContext(HitResults, Context);
			}

			if (m_FireEvent)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Firing Raycast Event %s with %d results."), *m_Name.ToString(), HitResults.Num()));
				}
#endif
				Context->GetAbility()->OnRaycastEventBP(Context.Get(), m_Name, HitResults);
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FAblAbilityDebug::DrawRaycastQuery(World, RayStart, RayEnd);
	}
#endif
}

void UAblRayCastQueryTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

	if (IsAsyncFriendly() && UAbleSettings::IsAsyncEnabled())
	{
		UAblRayCastQueryTaskScratchPad* ScratchPad = Cast<UAblRayCastQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		if (!ScratchPad->AsyncProcessed && ScratchPad->AsyncHandle._Handle != 0)
		{
			FAblAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

			AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
            if (SourceActor == nullptr)
			{
                return;
			}

			UWorld* World = SourceActor->GetWorld();

			FTraceDatum Datum;
			if (World->QueryTraceData(ScratchPad->AsyncHandle, Datum))
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Async Raycast found %d results."), Datum.OutHits.Num()));
				}
#endif
				// Run any filters.
				if (m_Filters.Num())
				{
					TArray<FAblQueryResult> QueryResults;
					QueryResults.Reserve(Datum.OutHits.Num());
					for (const FHitResult& Hit : Datum.OutHits)
					{
						QueryResults.Add(FAblQueryResult(Hit));
					}

					// Run filters.
					for (UAblCollisionFilter* Filter : m_Filters)
					{
						Filter->Filter(Context, QueryResults);
					}

					// Fix up our raw List.
					Datum.OutHits.RemoveAll([&](const FHitResult& RHS)
					{
						return !QueryResults.Contains(FAblQueryResult(RHS));
					});
				}

				if (m_CopyResultsToContext)
				{
					CopyResultsToContext(Datum.OutHits, Context);
				}

				if (m_FireEvent)
				{
					Context->GetAbility()->OnRaycastEventBP(Context.Get(), m_Name, Datum.OutHits);
				}

				ScratchPad->AsyncProcessed = true;
			}

		}
	}
}

bool UAblRayCastQueryTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (IsAsyncFriendly() && UAbleSettings::IsAsyncEnabled())
	{
		UAblRayCastQueryTaskScratchPad* ScratchPad = Cast<UAblRayCastQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		return ScratchPad->AsyncProcessed;
	}
	else
	{
		return UAblAbilityTask::IsDone(Context);
	}
}

UAblAbilityTaskScratchPad* UAblRayCastQueryTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (IsAsyncFriendly() && UAbleSettings::IsAsyncEnabled())
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblRayCastQueryTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}
		
		return NewObject<UAblRayCastQueryTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblRayCastQueryTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblRayCastQueryTask, STATGROUP_Able);
}

void UAblRayCastQueryTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Length, TEXT("Length"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_OnlyReturnBlockingHit, TEXT("Only Return Blocking Hit"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ReturnFaceIndex, TEXT("Return Face Index"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ReturnPhysicalMaterial, TEXT("Return Physical Material"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_QueryLocation, TEXT("Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_QueryEndLocation, TEXT("End Location"));
}

void UAblRayCastQueryTask::CopyResultsToContext(const TArray<FHitResult>& InResults, const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (UAblAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		TArray<TWeakObjectPtr<AActor>> AdditionTargets;
		AdditionTargets.Reserve(InResults.Num());
		for (const FHitResult& Result : InResults)
		{
			AdditionTargets.Add(Result.GetActor());
		}

		AbilityComponent->AddAdditionTargetsToContext(Context, AdditionTargets, m_AllowDuplicateEntries, m_ClearExistingTargets);
	}
}

#if WITH_EDITOR

FText UAblRayCastQueryTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblRayCastQueryFormat", "{0}: {1}");
	FString ShapeDescription = FString::Printf(TEXT("%.1fm"), m_Length * 0.01f);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ShapeDescription));
}

EDataValidationResult UAblRayCastQueryTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_QueryLocation.GetSourceTargetType() == EAblAbilityTargetType::ATT_TargetActor)
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
                ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryDependency", "Trying to use Target Actor but there's no Ability Targeting or Query( with GetCopyResultsToContext )and a Dependency Defined on this Task: {0}. You need one of those conditions to properly use this target."), FText::FromString(AbilityContext->GetDisplayName())));
                result = EDataValidationResult::Invalid;
            }
        }
    }

    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnRaycastEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnRaycastEventBP_NotFound", "Function 'OnRaycastEventBP' not found: {0}"), FText::FromString(AbilityContext->GetDisplayName())));
            result = EDataValidationResult::Invalid;
        }
    }

    return result;
}

void UAblRayCastQueryTask::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
    FTransform QueryTransform;
    m_QueryLocation.GetTransform(Context, QueryTransform);
	FVector Start = QueryTransform.GetLocation();
	FVector End = Start + QueryTransform.GetRotation().GetForwardVector() * m_Length;
    FAblAbilityDebug::DrawRaycastQuery(Context.GetWorld(), Start, End);
}

#endif

#undef LOCTEXT_NAMESPACE