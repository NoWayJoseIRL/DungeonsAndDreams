// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Engine/Texture.h"
#include "UObject/ObjectMacros.h"
#include "Tasks/IAblAbilityTask.h"

#include "ablSetShaderParameterValue.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Base class for all our supported parameter types. */
UCLASS(Abstract)
class ABLECORE_API UAblSetParameterValue : public UObject
{
	GENERATED_BODY()
public:
	enum Type
	{
		None = 0,
		Scalar,
		Vector,
		Texture,

		TotalTypes
	};

	UAblSetParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Type(None)
	{ }
	virtual ~UAblSetParameterValue() { }

	FORCEINLINE Type GetType() const { return m_Type; }
	virtual const FString ToString() const { return FString(TEXT("Invalid")); }
	
	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability) {};

#if WITH_EDITOR
	/* Fix our flags. */
	bool FixUpObjectFlags();
#endif

protected:
	Type m_Type;

private:
	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Scalar Parameter"))
class ABLECORE_API UAblSetScalarParameterValue : public UAblSetParameterValue
{
	GENERATED_BODY()
public:
	UAblSetScalarParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Scalar(0.0f)
	{
		m_Type = UAblSetParameterValue::Scalar;
	}
	virtual ~UAblSetScalarParameterValue() { }

	/* Sets the Scalar value. */
	FORCEINLINE void SetScalar(float InScalar) { m_Scalar = InScalar; }
	
	/* Returns the Scalar value. */
	float GetScalar(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual const FString ToString() const { return FString::Printf(TEXT("%4.3f"), m_Scalar); }

	virtual void BindDynamicDelegates(class UAblAbility* Ability);
private:
	UPROPERTY(EditInstanceOnly, Category = "Value", meta = (DisplayName = "Scalar", AblBindableProperty))
	float m_Scalar;

	UPROPERTY()
	FGetAblFloat m_ScalarDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Vector Parameter"))
class ABLECORE_API UAblSetVectorParameterValue : public UAblSetParameterValue
{
	GENERATED_BODY()
public:
	UAblSetVectorParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Color()
	{
		m_Type = UAblSetParameterValue::Vector;
	}
	virtual ~UAblSetVectorParameterValue() { }

	/* Sets the Color value. */
	FORCEINLINE void SetColor(const FLinearColor& InVector) { m_Color = InVector; }
	
	/* Returns the Color value. */
	const FLinearColor GetColor(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability);

	virtual const FString ToString() const { return m_Color.ToString(); }
private:
	UPROPERTY(EditInstanceOnly, Category = "Value", meta = (DisplayName = "Color", AblBindableProperty))
	FLinearColor m_Color;

	UPROPERTY()
	FGetAblColor m_ColorDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Texture Parameter"))
class ABLECORE_API UAblSetTextureParameterValue : public UAblSetParameterValue
{
	GENERATED_BODY()
public:
	UAblSetTextureParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Texture(nullptr)
	{
		m_Type = UAblSetParameterValue::Texture;
	}
	virtual ~UAblSetTextureParameterValue() { }

	/* Sets the Texture value. */
	FORCEINLINE void SetTexture(/*const*/ UTexture* InTexture) { m_Texture = InTexture; }
	
	/* Returns the Texture value. */
	UTexture* GetTexture(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAblAbility* Ability);

	virtual const FString ToString() const { return m_Texture ? m_Texture->GetName() : Super::ToString(); }
private:
	UPROPERTY(EditInstanceOnly, Category = "Value", meta = (DisplayName = "Texture", AblBindableProperty))
	/*const*/ UTexture* m_Texture; // Fix this when the API returns const

	UPROPERTY()
	FGetAblTexture m_TextureDelegate;
};

#undef LOCTEXT_NAMESPACE