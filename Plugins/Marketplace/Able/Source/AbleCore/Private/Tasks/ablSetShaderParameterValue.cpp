// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablSetShaderParameterValue.h"

#include "ablAbility.h"
#include "AbleCorePrivate.h"

FName UAblSetParameterValue::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Shader_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

float UAblSetScalarParameterValue::GetScalar(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Scalar);
}

void UAblSetScalarParameterValue::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Scalar, "Scalar");
}

const FLinearColor UAblSetVectorParameterValue::GetColor(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Color);
}

void UAblSetVectorParameterValue::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Color, "Color");
}

UTexture* UAblSetTextureParameterValue::GetTexture(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Texture);
}

void UAblSetTextureParameterValue::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Texture, "Texture");
}

#if WITH_EDITOR
bool UAblSetParameterValue::FixUpObjectFlags()
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