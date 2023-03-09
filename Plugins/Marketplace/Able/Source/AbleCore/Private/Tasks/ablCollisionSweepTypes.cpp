// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCollisionSweepTypes.h"

#include "ablAbility.h"
#include "ablAbilityDebug.h"
#include "AbleCorePrivate.h"
#include "Components/SkeletalMeshComponent.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCollisionSweepShape::UAblCollisionSweepShape(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_UseAsyncQuery(false)
{

}

UAblCollisionSweepShape::~UAblCollisionSweepShape()
{

}

void UAblCollisionSweepShape::GetAsyncResults(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTraceHandle& Handle, TArray<FAblQueryResult>& OutResults) const
{
	check(Context.IsValid());
	const UAblAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = m_SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}


	UWorld* World = SourceActor->GetWorld();
	check(World);

	FTraceDatum Datum;
	if (World->QueryTraceData(Handle, Datum))
	{
		for (const FHitResult& HitResult : Datum.OutHits)
		{
			OutResults.Add(FAblQueryResult(HitResult));
		}
	}
}

void UAblCollisionSweepShape::GetQueryTransform(const TWeakObjectPtr<const UAblAbilityContext>& Context, FTransform& OutTransform) const
{
	check(Context.IsValid());
	m_SweepLocation.GetTransform(*Context.Get(), OutTransform);
}

FName UAblCollisionSweepShape::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Sweep_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

void UAblCollisionSweepShape::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SweepLocation, TEXT("Sweep Location"));
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionSweepShape::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_CollisionChannels.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoCollisionChannels", "No Collision Channels: {0}, make sure you move over your old setting to the new array."), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}

bool UAblCollisionSweepShape::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));

	if (oldFlags != GetFlags())
	{
		Modify();

		return true;
	}

	return false;
}

#endif

UAblCollisionSweepBox::UAblCollisionSweepBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_HalfExtents(50.0f, 50.0f, 50.0f)
{

}

UAblCollisionSweepBox::~UAblCollisionSweepBox()
{

}

void UAblCollisionSweepBox::DoSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAblQueryResult>& OutResults) const
{
	check(Context.IsValid());
	FAblAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAblAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}

	FVector HalfExtents = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_HalfExtents);

	FTransform StartTransform, EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	FQuat SourceRotation = SourceTransform.GetRotation();
	FVector StartOffset = SourceRotation.GetForwardVector() * HalfExtents.X;

	StartTransform = SourceTransform * FTransform(StartOffset);

	FQuat EndRotation = EndTransform.GetRotation();
	FVector EndOffset = EndRotation.GetForwardVector() * HalfExtents.X;

	EndTransform *= FTransform(EndOffset);

	UWorld* World = SourceActor->GetWorld();
	check(World);

    FCollisionObjectQueryParams ObjectQuery;
    for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtents);
	if (m_OnlyReturnBlockingHit)
	{
		FHitResult SweepResult;
		if (World->SweepSingleByObjectType(SweepResult, StartTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			OutResults.Add(FAblQueryResult(SweepResult));
		}
	}
	else
	{
		TArray<FHitResult> SweepResults;
		if (World->SweepMultiByObjectType(SweepResults, StartTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			for (const FHitResult& SweepResult : SweepResults)
			{
				OutResults.Add(FAblQueryResult(SweepResult));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FVector AlignedBox = StartTransform.GetRotation().GetForwardVector() * HalfExtents.X;
		AlignedBox += StartTransform.GetRotation().GetRightVector() * HalfExtents.Y;
		AlignedBox += StartTransform.GetRotation().GetUpVector() * HalfExtents.Z;

		FAblAbilityDebug::DrawBoxSweep(World, StartTransform, EndTransform, AlignedBox);
	}
#endif
}

FTraceHandle UAblCollisionSweepBox::DoAsyncSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform) const
{
	check(Context.IsValid());
	FAblAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAblAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	FVector HalfExtents = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_HalfExtents);

	FTransform StartTransform, EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	FQuat SourceRotation = SourceTransform.GetRotation();
	FVector StartOffset = SourceRotation.GetForwardVector() * HalfExtents.X;

	StartTransform = SourceTransform * FTransform(StartOffset);

	FQuat EndRotation = EndTransform.GetRotation();
	FVector EndOffset = EndRotation.GetForwardVector() * HalfExtents.X;

	EndTransform *= FTransform(EndOffset);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtents);

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FVector AlignedBox = StartTransform.GetRotation().GetForwardVector() * HalfExtents.X;
		AlignedBox += StartTransform.GetRotation().GetRightVector() * HalfExtents.Y;
		AlignedBox += StartTransform.GetRotation().GetUpVector() * HalfExtents.Z;

		FAblAbilityDebug::DrawBoxSweep(World, StartTransform, EndTransform, AlignedBox);
	}
#endif

    FCollisionObjectQueryParams ObjectQuery;
    for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncSweepByObjectType(m_OnlyReturnBlockingHit ? EAsyncTraceType::Single : EAsyncTraceType::Multi, StartTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape);
}

void UAblCollisionSweepBox::BindDynamicDelegates(class UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_HalfExtents, "Half Extents");
}

#if WITH_EDITOR

const FString UAblCollisionSweepBox::DescribeShape() const
{
	return FString::Printf(TEXT("Box %.1fm x %.1fm x%.1fm"), m_HalfExtents.X * 2.0f * 0.01f, m_HalfExtents.Y * 2.0f * 0.01f, m_HalfExtents.Z * 2.0f * 0.01f);
}

void UAblCollisionSweepBox::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    FTransform StartTransform, EndTransform;
    m_SweepLocation.GetTransform(Context, StartTransform);
    m_SweepLocation.GetTransform(Context, EndTransform);

    FVector AlignedBox = StartTransform.GetRotation().GetForwardVector() * m_HalfExtents.X;
    AlignedBox += StartTransform.GetRotation().GetRightVector() * m_HalfExtents.Y;
    AlignedBox += StartTransform.GetRotation().GetUpVector() * m_HalfExtents.Z;

    FAblAbilityDebug::DrawBoxSweep(Context.GetWorld(), StartTransform, EndTransform, AlignedBox);
}

EDataValidationResult UAblCollisionSweepBox::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

UAblCollisionSweepSphere::UAblCollisionSweepSphere(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_Radius(50.0f)
{

}

UAblCollisionSweepSphere::~UAblCollisionSweepSphere()
{

}

void UAblCollisionSweepSphere::DoSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAblQueryResult>& OutResults) const
{
	check(Context.IsValid());
	FAblAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAblAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

    FCollisionObjectQueryParams ObjectQuery;
    for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	if (m_OnlyReturnBlockingHit)
	{
		FHitResult SweepResult;
		if (World->SweepSingleByObjectType(SweepResult, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape))
		{
			OutResults.Add(FAblQueryResult(SweepResult));
		}
	}
	else
	{
		TArray<FHitResult> SweepResults;
		if (World->SweepMultiByObjectType(SweepResults, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape))
		{
			for (const FHitResult& SweepResult : SweepResults)
			{
				OutResults.Add(FAblQueryResult(SweepResult));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FAblAbilityDebug::DrawSphereSweep(World, SourceTransform, EndTransform, Radius);
	}
#endif

}

FTraceHandle UAblCollisionSweepSphere::DoAsyncSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform) const
{
	check(Context.IsValid());
	FAblAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAblAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FAblAbilityDebug::DrawSphereSweep(World, SourceTransform, EndTransform, Radius);
	}
#endif

    FCollisionObjectQueryParams ObjectQuery;
    for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncSweepByObjectType(m_OnlyReturnBlockingHit ? EAsyncTraceType::Single : EAsyncTraceType::Multi, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape);
}

void UAblCollisionSweepSphere::BindDynamicDelegates(class UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");
}

#if WITH_EDITOR

const FString UAblCollisionSweepSphere::DescribeShape() const
{
	return FString::Printf(TEXT("Sphere %.1fm"), m_Radius * 2.0f * 0.01f);
}

void UAblCollisionSweepSphere::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    FTransform StartTransform, EndTransform;
    m_SweepLocation.GetTransform(Context, StartTransform);
    m_SweepLocation.GetTransform(Context, EndTransform);

    FAblAbilityDebug::DrawSphereSweep(Context.GetWorld(), StartTransform, EndTransform, m_Radius);
}

EDataValidationResult UAblCollisionSweepSphere::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

UAblCollisionSweepCapsule::UAblCollisionSweepCapsule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Radius(25.0f),
	m_Height(100.0f)
{

}

UAblCollisionSweepCapsule::~UAblCollisionSweepCapsule()
{

}

void UAblCollisionSweepCapsule::DoSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAblQueryResult>& OutResults) const
{
	check(Context.IsValid());
	FAblAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAblAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

    FCollisionObjectQueryParams ObjectQuery;
    for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);
	if (m_OnlyReturnBlockingHit)
	{
		FHitResult SweepResult;
		if (World->SweepSingleByObjectType(SweepResult, SourceTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			OutResults.Add(FAblQueryResult(SweepResult));
		}
	}
	else
	{
		TArray<FHitResult> SweepResults;
		if (World->SweepMultiByObjectType(SweepResults, SourceTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			for (const FHitResult& SweepResult : SweepResults)
			{
				OutResults.Add(FAblQueryResult(SweepResult));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FAblAbilityDebug::DrawCapsuleSweep(World, SourceTransform, EndTransform, Radius, Height);
	}
#endif

}

FTraceHandle UAblCollisionSweepCapsule::DoAsyncSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform) const
{
	check(Context.IsValid());
	FAblAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAblAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);

#if !UE_BUILD_SHIPPING
	if (FAblAbilityDebug::ShouldDrawQueries())
	{
		FAblAbilityDebug::DrawCapsuleSweep(World, SourceTransform, EndTransform, Radius, Height);
	}
#endif

    FCollisionObjectQueryParams ObjectQuery;
    for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncSweepByObjectType(m_OnlyReturnBlockingHit ? EAsyncTraceType::Single : EAsyncTraceType::Multi, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape);
}

void UAblCollisionSweepCapsule::BindDynamicDelegates(class UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Height, "Height");
}

#if WITH_EDITOR

const FString UAblCollisionSweepCapsule::DescribeShape() const
{
	return FString::Printf(TEXT("Capsule %.1fm x %.1fm"), m_Height * 0.01f, m_Radius * 2.0f * 0.01f);
}

void UAblCollisionSweepCapsule::OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    FTransform StartTransform, EndTransform;
    m_SweepLocation.GetTransform(Context, StartTransform);
    m_SweepLocation.GetTransform(Context, EndTransform);

    FAblAbilityDebug::DrawCapsuleSweep(Context.GetWorld(), StartTransform, EndTransform, m_Radius, m_Height);
}

EDataValidationResult UAblCollisionSweepCapsule::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

#undef LOCTEXT_NAMESPACE