// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityContext.h"
#include "ablAbilityTypes.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"

#include "ablTargetingBase.generated.h"

#define LOCTEXT_NAMESPACE "AblAbilityTargeting"

class UPrimitiveComponent;
class UAblAbilityTargetingFilter;

/* Base class for all our Targeting volumes/types. */
UCLASS(Abstract, EditInlineNew)
class ABLECORE_API UAblTargetingBase : public UObject
{
	GENERATED_BODY()
public:
	UAblTargetingBase(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingBase();

	/* Override in child classes, this method should find any targets according to the targeting volume/rules.*/
	virtual void FindTargets(UAblAbilityContext& Context) const { };

	void GetCollisionObjectParams(FCollisionObjectQueryParams& outParams) const;
	
	/* Returns the Context Target Type to use as the source for any location-based logic. */
	FORCEINLINE EAblAbilityTargetType GetSource() const { return m_Location.GetSourceTargetType(); }

	/* Returns the Collision Channel to execute this query on. */
	FORCEINLINE ECollisionChannel GetCollisionChannel() const { return m_CollisionChannel.GetValue(); }

	/* Returns the Collision Channels to execute this query on. */
	FORCEINLINE const TArray<TEnumAsByte<ECollisionChannel>>& GetCollisionChannels() const { return m_CollisionChannels; }

	/* Returns true if Targeting is using an Async query. */
	FORCEINLINE bool IsUsingAsync() const { return m_UseAsync; }

	/* Returns the range of this Targeting query.*/
	FORCEINLINE float GetRange() const { return m_Range; }

#if WITH_EDITOR
	// UObject Overrides.
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	// Fix up our flags. 
	bool FixUpObjectFlags();

	/* Called by Ability Editor to allow any special logic. */
	virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const {}
#endif
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates. */
	virtual void BindDynamicDelegates(UAblAbility* Ability);
protected:
	/* Method for Child classes to override. This should calculate and return the range for the query. */
	virtual float CalculateRange() const { return 0.0f; }

	/* Runs all Targeting Filters. */
	void FilterTargets(UAblAbilityContext& Context) const;

	/* If true, the targeting range will be automatically calculated using shape, rotation, and offset information. This does not include socket offsets. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting|Range", meta = (DisplayName = "Auto-calculate Range"))
	bool m_AutoCalculateRange;

	/* Range is primarily used by AI and other systems that have a target they wish to check against to avoid the full cost of the query. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting|Range", meta = (DisplayName = "Range", EditCondition = "!m_AutoCalculateRange", AblBindableProperty))
	float m_Range;

	UPROPERTY()
	FGetAblFloat m_RangeDelegate;

	/* If true, when calculating the range, it will be calculated as a 2D distance (XY plane) rather than 3D. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting|Range", meta = (DisplayName = "Calculate as 2D", EditCondition = "m_AutoCalculateRange"))
	bool m_CalculateAs2DRange;

	/* Where to begin our targeting query from. */
	UPROPERTY(EditInstanceOnly, Category="Targeting", meta=(DisplayName="Query Location", AblBindableProperty))
	FAblAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAblTargetLocation m_LocationDelegate;

	/* The collision channel to use when we perform the query. (DEPRECATED) */
	UPROPERTY(EditInstanceOnly, Category = "Targeting", meta = (DisplayName = "Collision Channel"))
	TEnumAsByte<ECollisionChannel> m_CollisionChannel;

	/* The collision channels to use when we perform the query. */
	UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channels"))
	TArray<TEnumAsByte<ECollisionChannel>> m_CollisionChannels;

	/* Filters to run the initial results through. These are executed in order. */
	UPROPERTY(EditInstanceOnly, Instanced, Category = "Targeting", meta = (DisplayName = "Filters"))
	TArray<UAblAbilityTargetingFilter*> m_Filters;

	/* 
	*  If true, runs the targeting query on as an Async query rather than blocking the game thread. 
	*  This can lead to a performance increase but will cause a one frame delay before the query
	*  completes. If you don't need frame perfect execution - this is probably worth the small delay.
	*/
	UPROPERTY(EditInstanceOnly, Category = "Optimization", meta = (DisplayName = "Use Async"))
	bool m_UseAsync;
	
	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

#undef LOCTEXT_NAMESPACE
