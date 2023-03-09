// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ablTargetingBase.h"
#include "UObject/ObjectMacros.h"

#include "ablTargetingMisc.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, HideCategories=(Location, Targeting, Optimization), meta = (DisplayName = "Instigator", ShortToolTip = "Sets the Instigator as the target."))
class UAblTargetingInstigator : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingInstigator(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingInstigator();

	/* Returns the Instigator Context Target. */
	virtual void FindTargets(UAblAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Targeting, Optimization), meta = (DisplayName = "Self", ShortToolTip = "Sets Self as the target."))
class UAblTargetingSelf : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingSelf(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingSelf();

	/* Returns the Self Context Target. */
	virtual void FindTargets(UAblAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Targeting, Optimization), meta = (DisplayName = "Owner", ShortToolTip = "Sets Owner as the target."))
class UAblTargetingOwner : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingOwner(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingOwner();

	/* Returns the Owner Context Target. */
	virtual void FindTargets(UAblAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Optimization), meta = (DisplayName = "Custom", ShortToolTip = "Execute custom targeting logic."))
class UAblTargetingCustom : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingCustom(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingCustom();

	/* Returns the any found Targets returned by the Ability's CustomTargetingFindTargets method. */
	virtual void FindTargets(UAblAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};


UCLASS(EditInlineNew, HideCategories = (Location, Optimization), meta = (DisplayName = "Blackboard", ShortToolTip = "Grab a target from a Blackboard."))
class UAblTargetingBlackboard : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingBlackboard(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingBlackboard();

	/* Returns the any found Targets returned by the Ability's CustomTargetingFindTargets method. */
	virtual void FindTargets(UAblAbilityContext& Context) const override;

	/* What Keys to use when attempting to grab an Actor from the Blackboard */
	UPROPERTY(EditInstanceOnly, Category = "Blackboard", meta = (DisplayName = "Blackboard Keys"))
	TArray<FName> m_BlackboardKeys;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Optimization), meta = (DisplayName = "Location", ShortToolTip = "Grab a Location from the Ability."))
class UAblTargetingLocation : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingLocation(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingLocation();

	/* Populates the Location we're targeting by calling the Ability's GetTargetLocation method.*/
	virtual void FindTargets(UAblAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

#undef LOCTEXT_NAMESPACE