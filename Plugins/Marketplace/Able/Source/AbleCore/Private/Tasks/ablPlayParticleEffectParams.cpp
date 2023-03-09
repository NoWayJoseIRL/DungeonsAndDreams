// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPlayParticleEffectParams.h"

#include "ablAbility.h"
#include "AbleCorePrivate.h"

FName UAblParticleEffectParam::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Particle_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

float UAblParticleEffectParamFloat::GetFloat(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Float);
}

void UAblParticleEffectParamFloat::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Float, "Float");
}

FLinearColor UAblParticleEffectParamColor::GetColor(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Color);
}

void UAblParticleEffectParamColor::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Color, "Color");
}

UMaterialInterface* UAblParticleEffectParamMaterial::GetMaterial(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Material);
}

void UAblParticleEffectParamMaterial::BindDynamicDelegates(UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Material, "Material");
}

FVector UAblParticleEffectParamVector::GetVector(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Vector);
}

void UAblParticleEffectParamVector::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Vector, "Vector");
}

bool UAblParticleEffectParamBool::GetBool(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Bool);
}

void UAblParticleEffectParamBool::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Bool, "Bool");
}

void UAblParticleEffectParamLocation::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Location"));
}

const FAblAbilityTargetTypeLocation UAblParticleEffectParamLocation::GetLocation(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);
}

