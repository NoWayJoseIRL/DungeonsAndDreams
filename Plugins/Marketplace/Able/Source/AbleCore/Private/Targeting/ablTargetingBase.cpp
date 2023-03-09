// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ablTargetingBase.h"

#include "AbleCorePrivate.h"
#include "Camera/CameraActor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Targeting/ablTargetingFilters.h"
#include "ablAbility.h"

#define LOCTEXT_NAMESPACE "AblAbilityTargeting"

UAblTargetingBase::UAblTargetingBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_AutoCalculateRange(true),
	m_Range(0.0f),
	m_CalculateAs2DRange(true),
	m_Location(),
	m_CollisionChannel(ECollisionChannel::ECC_Pawn),
	m_UseAsync(false)
{

}

UAblTargetingBase::~UAblTargetingBase()
{

}

void UAblTargetingBase::GetCollisionObjectParams(FCollisionObjectQueryParams& outParams) const
{
	if (m_CollisionChannels.Num() == 0)
	{
		outParams.AddObjectTypesToQuery(m_CollisionChannel.GetValue());
	}
	else
	{
		for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
		{
			outParams.AddObjectTypesToQuery(Channel.GetValue());
		}
	}
}

void UAblTargetingBase::FilterTargets(UAblAbilityContext& Context) const
{
	for (UAblAbilityTargetingFilter* TargetFilter : m_Filters)
	{
		TargetFilter->Filter(Context, *this);
	}
}

FName UAblTargetingBase::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Targeting_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

void UAblTargetingBase::BindDynamicDelegates(UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Query Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Range, "Range");
}

#if WITH_EDITOR

void UAblTargetingBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (m_AutoCalculateRange)
	{
		m_Range = CalculateRange();
	}
}

bool UAblTargetingBase::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));

	bool modified = oldFlags != GetFlags();

	if (modified)
	{
		Modify();
	}

	for (UAblAbilityTargetingFilter* Filter : m_Filters)
	{
		modified |= Filter->FixUpObjectFlags();
	}

	return modified;
}

#endif

#undef LOCTEXT_NAMESPACE
