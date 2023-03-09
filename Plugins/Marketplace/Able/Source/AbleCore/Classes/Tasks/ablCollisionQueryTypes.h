// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Engine/EngineTypes.h"

#include "ablAbilityContext.h"
#include "ablAbilityTypes.h"
#include "Targeting/ablTargetingBase.h"

#include "UObject/ObjectMacros.h"

#include "ablCollisionQueryTypes.generated.h"

struct FAblQueryResult;

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Base class for all our Collision Query Shapes. */
UCLASS(Abstract)
class ABLECORE_API UAblCollisionShape : public UObject
{
	GENERATED_BODY()
public:
	UAblCollisionShape(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionShape();

	/* Perform the Synchronous Query.*/
	virtual void DoQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& OutResults) const { }
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, FTransform& OutQueryTransform) const { return FTraceHandle(); }

	/* Returns true, or false, if this Query is Async or not. */
	FORCEINLINE bool IsAsync() const { return m_UseAsyncQuery; }
	
	/* Returns the World this Query is occurring in.*/
	UWorld* GetQueryWorld(const TWeakObjectPtr<const UAblAbilityContext>& Context) const;

	/* Helper method to help process Async Query. */
	virtual void ProcessAsyncOverlaps(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& QueryTransform, const TArray<FOverlapResult>& Overlaps, TArray<FAblQueryResult>& OutResults) const;

	/* Returns Query Information */
    const FAblAbilityTargetTypeLocation& GetQueryLocation() const { return m_QueryLocation; }

	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability);

#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const { return FString(TEXT("None")); }

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const {}

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);

	/* Fix our flags. */
	bool FixUpObjectFlags();
#endif

protected:
	UPROPERTY(EditInstanceOnly, Category="Query", meta=(DisplayName="Query Location", AblBindableProperty))
	FAblAbilityTargetTypeLocation m_QueryLocation;

	UPROPERTY()
	FGetAblTargetLocation m_QueryLocationDelegate;

    UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channels"))
    TArray<TEnumAsByte<ECollisionChannel>> m_CollisionChannels;

	/* If true, the query is placed in the Async queue. This can help performance by spreading the query out by a frame or two. */
	UPROPERTY(EditInstanceOnly, Category = "Optimization", meta = (DisplayName = "Use Async Query"))
	bool m_UseAsyncQuery;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Box", ShortToolTip = "A box based query volume."))
class UAblCollisionShapeBox : public UAblCollisionShape
{
	GENERATED_BODY()
public:
	UAblCollisionShapeBox(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionShapeBox();

	/* Perform the Synchronous Query.*/
	virtual void DoQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& OutResults) const override;
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability);
#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Half Extents of the Box */
	UPROPERTY(EditInstanceOnly, Category = "Box", meta = (DisplayName = "Half Extents", AblBindableProperty))
	FVector m_HalfExtents;

	UPROPERTY()
	FGetAblVector m_HalfExtentsDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Sphere", ShortToolTip = "A sphere based query volume."))
class UAblCollisionShapeSphere : public UAblCollisionShape
{
	GENERATED_BODY()
public:
	UAblCollisionShapeSphere(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionShapeSphere();

	/* Perform the Synchronous Query.*/
	virtual void DoQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& OutResults) const override;
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability);
#if WITH_EDITOR
	/* Text name for shape. */	
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Radius of the Sphere */
	UPROPERTY(EditInstanceOnly, Category = "Sphere", meta = (DisplayName = "Radius", AblBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAblFloat m_RadiusDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Capsule", ShortToolTip = "A capsule based query volume."))
class UAblCollisionShapeCapsule : public UAblCollisionShape
{
	GENERATED_BODY()
public:
	UAblCollisionShapeCapsule(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionShapeCapsule();

	/* Perform the Synchronous Query.*/
	virtual void DoQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& OutResults) const override;
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability);

#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Height of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Height", AblBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAblFloat m_HeightDelegate;

	/* Radius of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Radius", AblBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAblFloat m_RadiusDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Cone", ShortToolTip = "A cone based query volume, supports angles > 180 degrees."))
class UAblCollisionShapeCone : public UAblCollisionShape
{
	GENERATED_BODY()
public:
	UAblCollisionShapeCone(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblCollisionShapeCone();

	/* Do the Synchronous Query.*/
	virtual void DoQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& OutResults) const override;
	
	/* Do the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAblAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Helper method to help process our Async Query*/
	virtual void ProcessAsyncOverlaps(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FTransform& QueryTransform, const TArray<FOverlapResult>& Overlaps, TArray<FAblQueryResult>& OutResults) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;
#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* The Field of View (Angle/Azimuth) of the cone, in degrees. Supports Angles greater than 180 degrees. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "FOV", ClampMin = 1.0f, ClampMax = 360.0f, AblBindableProperty))
	float m_FOV; // Azimuth

	UPROPERTY()
	FGetAblFloat m_FOVDelegate;

	/* Length of the Cone. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Length", ClampMin = 0.1f, AblBindableProperty))
	float m_Length;

	UPROPERTY()
	FGetAblFloat m_LengthDelegate;

	/* Height of the Cone */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Height", ClampMin = 0.1f, EditCondition="!m_Is2DQuery", AblBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAblFloat m_HeightDelegate;

	/* If true, the Height of the cone is ignored. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Is 2D Query"))
	bool m_Is2DQuery;

	/* 3D Slice acts similar to a cone, except it does not pinch in towards the origin, but is instead an equal height throughout the volume (like a slice of cake).*/
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Use 3D Slice"))
	bool m_3DSlice;
};

#undef LOCTEXT_NAMESPACE