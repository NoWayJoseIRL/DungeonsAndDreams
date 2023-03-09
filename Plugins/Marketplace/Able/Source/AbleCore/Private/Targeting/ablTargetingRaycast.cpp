// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ablTargetingRaycast.h"

#include "ablAbility.h"
#include "ablAbilityDebug.h"
#include "ablSettings.h"
#include "Engine/World.h"

UAblTargetingRaycast::UAblTargetingRaycast(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Length(100.0f),
	m_OnlyWantBlockingObject(false)
{

}

UAblTargetingRaycast::~UAblTargetingRaycast()
{

}

void UAblTargetingRaycast::FindTargets(UAblAbilityContext& Context) const
{
	FAblAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Location);
	AActor* SourceActor = Location.GetSourceActor(Context);
    if (SourceActor == nullptr)
	{
        return;
	}

	UWorld* World = SourceActor->GetWorld();
	FTransform QueryTransform;

	float Length = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Length);

	FCollisionObjectQueryParams ObjectQuery;
	GetCollisionObjectParams(ObjectQuery);

	if (IsUsingAsync() && UAbleSettings::IsAsyncEnabled())
	{
		if (!Context.HasValidAsyncHandle())
		{
			Location.GetTransform(Context, QueryTransform);

			FVector Start = QueryTransform.GetLocation();
			FVector End = Start + QueryTransform.GetRotation().GetForwardVector() * Length;

			FTraceHandle AsyncHandle = World->AsyncLineTraceByObjectType(m_OnlyWantBlockingObject ? EAsyncTraceType::Single : EAsyncTraceType::Multi, Start, End, ObjectQuery);
			Context.SetAsyncHandle(AsyncHandle);
		}
		else
		{
			FTraceDatum Datum;
			if (World->QueryTraceData(Context.GetAsyncHandle(), Datum))
			{
				ProcessResults(Context, Datum.OutHits);
				FTraceHandle Empty;
				Context.SetAsyncHandle(Empty);
			}

			return;
		}
	}
	else
	{
		Location.GetTransform(Context, QueryTransform);

		FVector Start = QueryTransform.GetLocation();
		FVector End = Start + QueryTransform.GetRotation().GetForwardVector() * Length;

		TArray<FHitResult> HitResults;
		if (m_OnlyWantBlockingObject)
		{
			FHitResult outResult;
			if (World->LineTraceSingleByObjectType(outResult, Start, End, ObjectQuery))
			{
				HitResults.Add(outResult);
			}
		}
		else
		{
			World->LineTraceMultiByObjectType(HitResults, Start, End, ObjectQuery);
		}

		if (HitResults.Num() != 0)
		{
			ProcessResults(Context, HitResults);
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FVector Start = QueryTransform.GetLocation();
		FVector End = Start + QueryTransform.GetRotation().GetForwardVector() * Length;
		FAblAbilityDebug::DrawRaycastQuery(World, Start, End);
	}
#endif // UE_BUILD_SHIPPING
}

void UAblTargetingRaycast::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Length, "Length");

	Super::BindDynamicDelegates(Ability);
}

float UAblTargetingRaycast::CalculateRange() const
{
	if (m_CalculateAs2DRange)
	{
		return m_Location.GetOffset().Size2D() + m_Length;
	}

	return m_Location.GetOffset().Size() + m_Length;
}

void UAblTargetingRaycast::ProcessResults(UAblAbilityContext& Context, const TArray<struct FHitResult>& Results) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();

	for (const FHitResult& Result : Results)
	{
		if (AActor* ResultActor = Result.GetActor())
		{
			TargetActors.Add(ResultActor);
		}
	}

	// Run any extra filters.
	FilterTargets(Context);
}

#if WITH_EDITOR
void UAblTargetingRaycast::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
	FTransform QueryTransform;
	m_Location.GetTransform(Context, QueryTransform);
	FVector Start = QueryTransform.GetLocation();
	FVector End = Start + QueryTransform.GetRotation().GetForwardVector() * m_Length;
	FAblAbilityDebug::DrawRaycastQuery(Context.GetWorld(), Start, End);
}
#endif