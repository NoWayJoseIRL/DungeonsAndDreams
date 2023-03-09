// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityContext.h"
#include "ablAbilityTypes.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Targeting/ablTargetingBase.h"

#include "ablCollisionSweepTypes.generated.h"

struct FAblQueryResult;

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Base class for all our Sweep Shapes. */
UCLASS(Abstract)
class ABLECORE_API UAblCollisionSweepShape : public UObject
{
	GENERATED_BODY()
public:
	UAblCollisionSweepShape(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionSweepShape();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAblQueryResult>& OutResults) const { }
	
	/* Queue up the Async Sweep. */
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform) const { return FTraceHandle(); }
	
	/* Retrieve the Async results and process them. */
	virtual void GetAsyncResults(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTraceHandle& Handle, TArray<FAblQueryResult>& OutResults) const;

	/* Returns true if this shape is using Async. */
	FORCEINLINE bool IsAsync() const { return m_UseAsyncQuery; }

	/* Helper method to return the transform used in our Query. */
	void GetQueryTransform(const TWeakObjectPtr<const UAblAbilityContext>& Context, FTransform& OutTransform) const;

    const FAblAbilityTargetTypeLocation& GetSweepLocation() const { return m_SweepLocation; }

	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability);

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const { return FString(TEXT("None")); }

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const {}

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);

	/* Fix our flags. */
	bool FixUpObjectFlags();
#endif

protected:
	/* The location of our Query. */
	UPROPERTY(EditInstanceOnly, Category="Sweep", meta=(DisplayName="Location", AblBindableProperty))
	FAblAbilityTargetTypeLocation m_SweepLocation;

	UPROPERTY()
	FGetAblTargetLocation m_SweepLocationDelegate;

    UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channels"))
    TArray<TEnumAsByte<ECollisionChannel>> m_CollisionChannels;

	/* If true, only return the blocking hit. Otherwise return all hits, including the blocking hit.*/
	UPROPERTY(EditInstanceOnly, Category = "Sweep", meta = (DisplayName = "Only Return Blocking Hit"))
	bool m_OnlyReturnBlockingHit;

	/* If true, the query is placed in the Async queue. This can help performance by spreading the query out by a frame or two. */
	UPROPERTY(EditInstanceOnly, Category = "Optimization", meta = (DisplayName = "Use Async Query"))
	bool m_UseAsyncQuery;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Box", ShortToolTip = "A box based sweep volume."))
class ABLECORE_API UAblCollisionSweepBox : public UAblCollisionSweepShape
{
	GENERATED_BODY()
public:
	UAblCollisionSweepBox(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionSweepBox();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAblQueryResult>& OutResults) const override;
	
	/* Queue up the Async Sweep. */
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Half Extents of the Box */
	UPROPERTY(EditInstanceOnly, Category = "Box", meta = (DisplayName = "Half Extents", AblBindableProperty))
	FVector m_HalfExtents;

	UPROPERTY()
	FGetAblVector m_HalfExtentsDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Sphere", ShortToolTip = "A sphere based sweep volume."))
class ABLECORE_API UAblCollisionSweepSphere : public UAblCollisionSweepShape
{
	GENERATED_BODY()
public:
	UAblCollisionSweepSphere(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionSweepSphere();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAblQueryResult>& OutResults) const override;
	
	/* Queue up the Async Sweep. */
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Radius of the Sphere */
	UPROPERTY(EditInstanceOnly, Category = "Sphere", meta = (DisplayName = "Radius", AblBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAblFloat m_RadiusDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Capsule", ShortToolTip = "A capsule based sweep volume."))
class ABLECORE_API UAblCollisionSweepCapsule : public UAblCollisionSweepShape
{
	GENERATED_BODY()
public:
	UAblCollisionSweepCapsule(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionSweepCapsule();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAblQueryResult>& OutResults) const override;
	
	/* Queue up the Async Sweep.*/
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& SourceTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Radius of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Radius", AblBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAblFloat m_RadiusDelegate;

	/* Height of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Height", AblBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAblFloat m_HeightDelegate;
};

#undef LOCTEXT_NAMESPACE