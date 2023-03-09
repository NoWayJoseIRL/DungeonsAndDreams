// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ablTargetingBase.h"
#include "UObject/ObjectMacros.h"

#include "ablTargetingBox.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, meta=(DisplayName="Box", ShortToolTip="A box based targeting volume."))
class UAblTargetingBox : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingBox(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingBox();

	/* Find all the Targets within our query.*/
	virtual void FindTargets(UAblAbilityContext& Context) const override;

	/* Bind our Delegates. */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;
#endif

private:
	/* Calculate the range of our query. */
	virtual float CalculateRange() const override;

	/* Helper method to return an AABB that fits the Query's rotation.*/
	FVector GetAlignedBox(const UAblAbilityContext& Context, FTransform& OutQueryTransform) const;

	/* Process the Targeting Query.*/
	void ProcessResults(UAblAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const;

	/* Half Extents of the Box */
	UPROPERTY(EditInstanceOnly, Category = "Box", meta = (DisplayName = "Half Extents", AblBindableProperty))
	FVector m_HalfExtents;

	UPROPERTY()
	FGetAblVector m_HalfExtentsDelegate;
};

#undef LOCTEXT_NAMESPACE