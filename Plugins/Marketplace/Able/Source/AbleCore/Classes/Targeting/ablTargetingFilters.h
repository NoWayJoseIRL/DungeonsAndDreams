// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityContext.h"
#include "UObject/ObjectMacros.h"

#include "ablTargetingFilters.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

class UAblAbilityContext;
struct FAblQueryResult;
class UAblTargetingBase;

/* Base class for all Targeting Filters. */
UCLASS(Abstract)
class ABLECORE_API UAblAbilityTargetingFilter : public UObject
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilter(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilter();

	/* Override and filter out whatever you deem invalid.*/
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const;

#if WITH_EDITOR
	/* Fix up our flags. */
	bool FixUpObjectFlags();
#endif
};

UENUM()
enum EAblTargetingFilterSort
{
	AblTargetFilterSort_Ascending = 0 UMETA(DisplayName = "Ascending"),
	AblTargetFilterSort_Descending UMETA(DisplayName = "Descending")
};

UCLASS(EditInlineNew, meta = (DisplayName = "Sort by Distance", ShortToolTip = "Sorts Targets from Nearest to Furthest (Ascending), or Furthest to Nearest (Descending)."))
class ABLECORE_API UAblAbilityTargetingFilterSortByDistance : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterSortByDistance(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterSortByDistance();

	/* Sort the Targets by filter in either ascending or descending mode. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;

protected:
	/* Helper method to return the location for our distance logic. */
	FVector GetSourceLocation(const UAblAbilityContext& Context, EAblAbilityTargetType SourceType) const;

	/* If true, the Distance logic will only use the 2D (XY Plane) distance for calculations.*/
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName="Use 2D Distance"))
	bool m_Use2DDistance;

	/* What direction to sort the results. Near to Far (Ascending) or Far to Near (Descending). */
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Sort Direction"))
	TEnumAsByte<EAblTargetingFilterSort> m_SortDirection;
};

UCLASS(EditInlineNew, hidecategories = (Filter), meta = (DisplayName = "Filter Self", ShortToolTip = "Filters out the Self Actor."))
class UAblAbilityTargetingFilterSelf : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterSelf(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterSelf();

	/* Filter out the Self Context Target. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;
};

UCLASS(EditInlineNew, hidecategories=(Filter), meta = (DisplayName = "Filter Owner", ShortToolTip = "Filters out the Owner Actor."))
class UAblAbilityTargetingFilterOwner : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterOwner(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterOwner();

	/* Filter out the Owner Context Target. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;
};

UCLASS(EditInlineNew, hidecategories = (Filter), meta = (DisplayName = "Filter Instigator", ShortToolTip = "Filters out the Instigator Actor."))
class UAblAbilityTargetingFilterInstigator : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterInstigator(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterInstigator();

	/* Filter out the Instigator Context Target. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;
};

UCLASS(EditInlineNew, hidecategories = (Filter), meta = (DisplayName = "Filter By Class", ShortToolTip = "Filters out any instances of the provided class, can also be negated to keep only instances of the provided class."))
class UAblAbilityTargetingFilterClass : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterClass(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterClass();

	/* Filter by the provided class. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;

protected:
	/* The Class to filter by. */
	UPROPERTY(EditInstanceOnly, Category = "Class Filter", meta = (DisplayName = "Class"))
	const UClass* m_Class;

	/* If true, the filter will keep only items that are of the provided class rather than filter them out. */
	UPROPERTY(EditInstanceOnly, Category = "Class Filter", meta = (DisplayName = "Negate"))
	bool m_Negate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Max Targets", ShortToolTip = "Limit results to N Targets."))
class UAblAbilityTargetingFilterMaxTargets : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterMaxTargets(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterMaxTargets();

	/* Keep all but N Targets.*/
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;
protected:
	/* The Maximum Amount of Targets allowed. */
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Max Targets", ClampMin = 1))
	int32 m_MaxTargets;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Custom", ShortToolTip = "Calls the Ability's IsValidForActor Blueprint Event. If the event returns true, the actor is kept. If false, it is discarded."))
class UAblAbilityTargetingFilterCustom : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterCustom(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterCustom();

	/* Call into the Ability's CustomFilter method. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;
protected:
	// Optional Name identifier for this event in case you are using IsValidForActor multiple times in the Ability.
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Event Name"))
	FName m_EventName;

	// If true, the event is run across multiple actors on various cores. This can help speed things up if the potential actor list is large, or the BP logic is complex.
	UPROPERTY(EditInstanceOnly, Category = "Optimize", meta = (DisplayName = "Use Async"))
	bool m_UseAsync;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Line Of Sight", ShortToolTip = "Casts a ray between the Target and the Source Location(Actor, Location, etc). If a blocking hit is found between the two, the target is discarded."))
class UAblAbilityTargetingFilterLineOfSight : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterLineOfSight(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterLineOfSight();

	/* Call into the Ability's CustomFilter method. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;
protected:
	// The Location to use as our source for our raycast.
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Source Location"))
	FAblAbilityTargetTypeLocation m_SourceLocation;

	// The Collision Channels to run the Raycast against.
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Collision Channels"))
	TArray<TEnumAsByte<ECollisionChannel>> m_CollisionChannels;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Unique Actors", ShortToolTip = "Reduce Collision results to only the unique results based on Actors hit rather than components hit."))
class UAblAbilityTargetingFilterUniqueActors : public UAblAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAblAbilityTargetingFilterUniqueActors(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityTargetingFilterUniqueActors();

	/* Filter the results. */
	virtual void Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const override;
};


#undef LOCTEXT_NAMESPACE