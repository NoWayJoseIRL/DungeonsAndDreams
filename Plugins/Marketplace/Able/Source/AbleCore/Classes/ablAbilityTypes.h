// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AlphaBlend.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"

#include "ablAbilityTypes.generated.h"

UENUM(BlueprintType)
enum EAblAbilityTargetType
{
	ATT_Self UMETA(DisplayName = "Self"),
	ATT_Owner UMETA(DisplayName = "Owner"),
	ATT_Instigator UMETA(DisplayName = "Instigator"),
	ATT_TargetActor UMETA(DisplayName = "Target Actor"),
	ATT_Camera UMETA(DisplayName = "Camera"),
	ATT_Location UMETA(DisplayName = "Location"),
	ATT_World UMETA(DisplayName = "World")
};

/* Helper Struct for Blend Times. */
USTRUCT(BlueprintType)
struct ABLECORE_API FAblBlendTimes
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (DisplayName = "Blend In"))
	float m_BlendIn;

	// Blend out time (in seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (DisplayName = "Blend Out"))
	float m_BlendOut;

	FAblBlendTimes()
		: m_BlendIn(0.25f),
		m_BlendOut(0.25f)
	{}
};

/* A Simple struct for shared logic that takes in a various information and returns a Transform based on options selected by the user. */
USTRUCT(BlueprintType)
struct ABLECORE_API FAblAbilityTargetTypeLocation
{
	GENERATED_USTRUCT_BODY()
public:
	FAblAbilityTargetTypeLocation();

	void GetTargetTransform(const UAblAbilityContext& Context, int32 TargetIndex, FTransform& OutTransform) const;
	void GetTransform(const UAblAbilityContext& Context, FTransform& OutTransform) const;
	AActor* GetSourceActor(const UAblAbilityContext& Context) const;
	EAblAbilityTargetType GetSourceTargetType() const { return m_Source.GetValue(); }

	FORCEINLINE const FName& GetSocketName() const { return m_Socket; }
	FORCEINLINE const FVector& GetOffset() const { return m_Offset; }
	FORCEINLINE const FRotator& GetRotation() const { return m_Rotation; }
protected:
	/* The source to launch this targeting query from. All checks (distance, etc) will be in relation to this source. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Source"))
	TEnumAsByte<EAblAbilityTargetType> m_Source;

	/* An additional Offset to provide to our location. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Location Offset"))
	FVector m_Offset;

	/* Rotation to use for the location. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Rotation"))
	FRotator m_Rotation;

	/* Socket to use as the Base Transform or Location.*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Socket"))
	FName m_Socket;

	/* If true, the Socket rotation will be used as well as its location.*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Use Socket Rotation", EditCondition = "!m_UseActorRotation"))
	bool m_UseSocketRotation;

	/* If true, the Actor rotation will be used.*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Use Actor Rotation", EditCondition = "!m_UseSocketRotation"))
	bool m_UseActorRotation;
};

UENUM(BlueprintType)
enum EAblAbilityTaskResult
{
	// Task finished normally (e.g. reached its end time).
	Successful UMETA(DisplayName = "Successful"),

	// Ability is being branched to another ability.
	Branched UMETA(DisplayName = "Branched"),

	// Ability was interrupted.
	Interrupted UMETA(DisplayName = "Interrupted"),

    // Ability was decayed.
    Decayed UMETA(DisplayName = "Decayed")
};

UENUM(BlueprintType)
enum EAblAbilityTaskRealm
{
	ATR_Client = 0 UMETA(DisplayName = "Client"),
	ATR_Server UMETA(DisplayName = "Server"),
	ATR_ClientAndServer UMETA(DisplayName = "Client And Server"),

	ATR_TotalRealms UMETA(DisplayName = "Internal_DO_NOT_USE")
};

UENUM(BlueprintType)
enum EAblConditionResults
{
	ACR_Passed = 0 UMETA(DisplayName = "Passed"),
	ACR_Failed UMETA(DisplayName = "Failed"),
	ACR_Ignored UMETA(DisplayName = "Ignored")
};

UENUM(BlueprintType)
enum class EAblPlayCameraShakeStopMode : uint8
{
	DontStop,
	Stop,
	StopImmediately,
};

#define ABL_DECLARE_DYNAMIC_PROPERTY(Type, Property) TAttribute<Type> Property##Binding;
#define ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, Property) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context.Get(), Property) : Property)
#define ABL_GET_DYNAMIC_PROPERTY_VALUE_ENUM(Context, Property) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context.Get(), Property.GetValue()) : Property.GetValue())
#define ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, Property) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context, Property) : Property)
#define ABL_BIND_DYNAMIC_PROPERTY(Ability, Property, DisplayName) (Property##Delegate.BindUFunction(Ability, GetDynamicDelegateName(DisplayName)))

namespace UMAblAbility
{
	enum
	{
		// [PropertyMetadata] This property allows properties to be bind-able to a BP delegate, otherwise it uses the constant value defined by the user.
		// UPROPERTY(meta=(AblBindableProperty))
		AblBindableProperty,

		// [PropertyMetadata] Specifies the name of the BP Function to call by default for a binding property.
		// UPROPERTY(meta=(AblDefaultBinding))
		AblDefaultBinding,
	};
}

UCLASS(Abstract)
class ABLECORE_API UAblDynamicPropertySupport : public UObject
{
	GENERATED_BODY()
public:
	UAblDynamicPropertySupport(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblDynamicPropertySupport();

	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	virtual FString GetDynamicPropertyParentIdentifier() const;

	/* Get Dynamic Delegate Name. */
	virtual FName GetDynamicDelegateName(const FString& PropertyName) const;
protected:
	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};


DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(UAnimationAsset*, FGetAblAnimation, const UAblAbilityContext*, Context, UAnimationAsset*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(USoundBase*, FGetAblSound, const UAblAbilityContext*, Context, USoundBase*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(USoundAttenuation*, FGetAblAttenuation, const UAblAbilityContext*, Context, USoundAttenuation*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(USoundConcurrency*, FGetAblConcurrency, const UAblAbilityContext*, Context, USoundConcurrency*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(UParticleSystem*, FGetAblParticleSystem, const UAblAbilityContext*, Context, UParticleSystem*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(UNiagaraSystem*, FGetAblNiagaraSystem, const UAblAbilityContext*, Context, UNiagaraSystem*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(UMaterialInterface*, FGetAblMaterial, const UAblAbilityContext*, Context, UMaterialInterface*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(UTexture*, FGetAblTexture, const UAblAbilityContext*, Context, UTexture*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FVector, FGetAblVector, const UAblAbilityContext*, Context, FVector, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FLinearColor, FGetAblColor, const UAblAbilityContext*, Context, FLinearColor, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(float, FGetAblFloat, const UAblAbilityContext*, Context, float, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FGetAblBool, const UAblAbilityContext*, Context, bool, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(int32, FGetAblInt, const UAblAbilityContext*, Context, int32, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FAblBlendTimes, FGetAblBlendTimes, const UAblAbilityContext*, Context, FAblBlendTimes, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSubclassOf<UDamageType>, FGetDamageTypeClass, const UAblAbilityContext*, Context, TSubclassOf<UDamageType>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FAblAbilityTargetTypeLocation, FGetAblTargetLocation, const UAblAbilityContext*, Context, FAblAbilityTargetTypeLocation, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSubclassOf<UCameraShakeBase>, FGetCameraShakeClass, const UAblAbilityContext*, Context, TSubclassOf<UCameraShakeBase>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(EAblPlayCameraShakeStopMode, FGetCameraStopMode, const UAblAbilityContext*, Context, EAblPlayCameraShakeStopMode, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TArray<FName>, FGetNameArray, const UAblAbilityContext*, Context, TArray<FName>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(EAblAbilityTargetType, FGetAblTargetType, const UAblAbilityContext*, Context, EAblAbilityTargetType, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FName, FGetAblName, const UAblAbilityContext*, Context, FName, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FRadialDamageParams, FGetAblRadialDamageParams, const UAblAbilityContext*, Context, FRadialDamageParams, StaticValue);

// One offs
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(bool, FGetAblBoolWithResult, const UAblAbilityContext*, Context, bool, StaticValue, EAblAbilityTaskResult, Result);
#define ABL_GET_DYNAMIC_PROPERTY_VALUE_THREE(Context, Property, Result) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context.Get(), Property, Result) : Property)