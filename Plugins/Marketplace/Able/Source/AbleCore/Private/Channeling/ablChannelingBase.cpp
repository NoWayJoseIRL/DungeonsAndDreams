// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Channeling/ablChannelingBase.h"

UAblChannelingBase::UAblChannelingBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Negate(false)
{

}

UAblChannelingBase::~UAblChannelingBase()
{

}

EAblConditionResults UAblChannelingBase::GetConditionResult(UAblAbilityContext& Context) const
{
	EAblConditionResults Result = CheckConditional(Context);

	if (m_Negate)
	{
		switch(Result)
		{
			case EAblConditionResults::ACR_Passed: Result = ACR_Failed; break;
			case EAblConditionResults::ACR_Failed: Result = ACR_Passed; break;
			default: break;
		}
	}

	return Result;
}

FName UAblChannelingBase::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Channeling_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

#if WITH_EDITOR
bool UAblChannelingBase::FixUpObjectFlags()
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