// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ablTargetingSphere.h"

#include "ablAbility.h"
#include "ablAbilityDebug.h"
#include "ablSettings.h"
#include "Engine/World.h"

UAblTargetingSphere::UAblTargetingSphere(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Radius(100.0f)
{

}

UAblTargetingSphere::~UAblTargetingSphere()
{

}

void UAblTargetingSphere::FindTargets(UAblAbilityContext& Context) const
{
	FAblAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Location);
	AActor* SourceActor = Location.GetSourceActor(Context);
    if (SourceActor == nullptr)
	{
        return;
	}

	UWorld* World = SourceActor->GetWorld();
	FTransform QueryTransform;

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Radius);

	FCollisionObjectQueryParams ObjectQuery;
	GetCollisionObjectParams(ObjectQuery);

	if (IsUsingAsync() && UAbleSettings::IsAsyncEnabled())
	{
		if (!Context.HasValidAsyncHandle()) // If we don't have a handle, create our query.
		{
			Location.GetTransform(Context, QueryTransform);

			FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

			FTraceHandle AsyncHandle = World->AsyncOverlapByObjectType(QueryTransform.GetLocation(), QueryTransform.GetRotation(), ObjectQuery, SphereShape);
			Context.SetAsyncHandle(AsyncHandle);
		}
		else // Poll for completion.
		{
			FOverlapDatum Datum;
			if (World->QueryOverlapData(Context.GetAsyncHandle(), Datum))
			{
				ProcessResults(Context, Datum.OutOverlaps);
				FTraceHandle Empty;
				Context.SetAsyncHandle(Empty); // Reset our handle.
			}

			return;
		}
	}
	else // Sync Query
	{
		Location.GetTransform(Context, QueryTransform);

		FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

		TArray<FOverlapResult> Results;
		if (World->OverlapMultiByObjectType(Results, QueryTransform.GetTranslation(), QueryTransform.GetRotation(), ObjectQuery, SphereShape))
		{
			ProcessResults(Context, Results);
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FAblAbilityDebug::DrawSphereQuery(World, QueryTransform, Radius);
	}
#endif // UE_BUILD_SHIPPING
}

void UAblTargetingSphere::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");

	Super::BindDynamicDelegates(Ability);
}

float UAblTargetingSphere::CalculateRange() const
{
	const float OffsetSize = m_CalculateAs2DRange ? m_Location.GetOffset().Size2D() : m_Location.GetOffset().Size();

	return m_Radius + OffsetSize;
}

void UAblTargetingSphere::ProcessResults(UAblAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();

	for (const FOverlapResult& Result : Results)
	{
		if (AActor* ResultActor = Result.GetActor())
		{
			TargetActors.Add(ResultActor);
		}
	}

	// Run filters.
	FilterTargets(Context);
}

#if WITH_EDITOR
void UAblTargetingSphere::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
	AActor* SourceActor = m_Location.GetSourceActor(Context);
	if (SourceActor != nullptr)
	{
		FTransform QueryTransform;
		m_Location.GetTransform(Context, QueryTransform);

		FAblAbilityDebug::DrawSphereQuery(Context.GetWorld(), QueryTransform, m_Radius);
	}
}
#endif