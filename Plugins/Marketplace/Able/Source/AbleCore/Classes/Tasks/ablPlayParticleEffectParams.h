// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ablTargetingBase.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablPlayParticleEffectParams.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Base class for all our Particle Effect Parameters. */
UCLASS(Abstract)
class ABLECORE_API UAblParticleEffectParam : public UObject
{
	GENERATED_BODY()
public:
	UAblParticleEffectParam(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer), m_ParameterName(NAME_None) { }
	virtual ~UAblParticleEffectParam() { }

	FORCEINLINE const FName GetParameterName() const { return m_ParameterName; }
	FORCEINLINE bool GetEnableTickUpdate() const { return m_EnableTickUpdate; }

	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) {};

private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta=(DisplayName="Property Name"))
	FName m_ParameterName;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;

	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta=(DisplayName = "Enable Tick Update"))
	bool m_EnableTickUpdate;
};

UCLASS(EditInlineNew, meta=(DisplayName="Context Actor", ShortToolTip="Reference a Context Actor"))
class UAblParticleEffectParamContextActor : public UAblParticleEffectParam
{
	GENERATED_BODY()
public:
	UAblParticleEffectParamContextActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), m_ContextActor(EAblAbilityTargetType::ATT_Self) { }
	virtual ~UAblParticleEffectParamContextActor() { }

	FORCEINLINE EAblAbilityTargetType GetContextActorType() const { return m_ContextActor.GetValue(); }
private:
	UPROPERTY(EditInstanceOnly, Category="Parameter", meta=(DisplayName="Context Actor"))
	TEnumAsByte<EAblAbilityTargetType> m_ContextActor;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Location", ShortToolTip = "Reference a Location (can use a Context Actor as reference)"))
class UAblParticleEffectParamLocation: public UAblParticleEffectParam
{
	GENERATED_BODY()
public:
	UAblParticleEffectParamLocation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { }
	virtual ~UAblParticleEffectParamLocation() { } 

	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;

	const FAblAbilityTargetTypeLocation GetLocation(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;
private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Location", AblBindableProperty))
	FAblAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAblTargetLocation m_LocationDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Float", ShortToolTip = "Set a float parameter"))
class UAblParticleEffectParamFloat : public UAblParticleEffectParam
{
    GENERATED_BODY()
public:
    UAblParticleEffectParamFloat(const FObjectInitializer& ObjectInitializer) \
        : Super(ObjectInitializer)
        , m_Float(0.0f)
    {
    }
    virtual ~UAblParticleEffectParamFloat() { }

	float GetFloat(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;
private:
    UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Float", AblBindableProperty))
    float m_Float;

	UPROPERTY()
	FGetAblFloat m_FloatDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Color", ShortToolTip = "Set a color parameter"))
class UAblParticleEffectParamColor : public UAblParticleEffectParam
{
    GENERATED_BODY()
public:
    UAblParticleEffectParamColor(const FObjectInitializer& ObjectInitializer) \
        : Super(ObjectInitializer)
        , m_Color(FLinearColor::White) 
    {
    }
    virtual ~UAblParticleEffectParamColor() { }

	FLinearColor GetColor(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;
private:
    UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Color", AblBindableProperty))
    FLinearColor m_Color;

	UPROPERTY()
	FGetAblColor m_ColorDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Material", ShortToolTip = "Set a material parameter (Only works with Cascade systems, and not Niagara)"))
class UAblParticleEffectParamMaterial : public UAblParticleEffectParam
{
    GENERATED_BODY()
public:
    UAblParticleEffectParamMaterial(const FObjectInitializer& ObjectInitializer) \
        : Super(ObjectInitializer)
        , m_Material(nullptr)
    {
    }
    virtual ~UAblParticleEffectParamMaterial() { }

	class UMaterialInterface* GetMaterial(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;
private:
    UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Material", AblBindableProperty))
    class UMaterialInterface* m_Material;

	UPROPERTY()
	FGetAblMaterial m_MaterialDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Vector", ShortToolTip = "Set a Vector parameter"))
class UAblParticleEffectParamVector : public UAblParticleEffectParam
{
	GENERATED_BODY()
public:
	UAblParticleEffectParamVector(const FObjectInitializer& ObjectInitializer) \
		: Super(ObjectInitializer)
		, m_Vector(FVector::ZeroVector)
	{
	}
	virtual ~UAblParticleEffectParamVector() { }

	FVector GetVector(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;
private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Vector", AblBindableProperty))
	FVector m_Vector;

	UPROPERTY()
	FGetAblVector m_VectorDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Bool", ShortToolTip = "Set a Bool parameter"))
class UAblParticleEffectParamBool : public UAblParticleEffectParam
{
	GENERATED_BODY()
public:
	UAblParticleEffectParamBool(const FObjectInitializer& ObjectInitializer) \
		: Super(ObjectInitializer)
		, m_Bool(false)
	{
	}
	virtual ~UAblParticleEffectParamBool() { }

	bool GetBool(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;
private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Bool", AblBindableProperty))
	bool m_Bool;

	UPROPERTY()
	FGetAblBool m_BoolDelegate;
};


#undef LOCTEXT_NAMESPACE