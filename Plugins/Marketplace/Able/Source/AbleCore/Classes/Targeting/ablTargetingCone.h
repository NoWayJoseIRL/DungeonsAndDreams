// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ablTargetingBase.h"
#include "Tasks/IAblAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablTargetingCone.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, meta = (DisplayName = "Cone", ShortToolTip = "A cone based targeting volume."))
class UAblTargetingCone : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingCone(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingCone();

	/* Find any Targets.*/
	virtual void FindTargets(UAblAbilityContext& Context) const override;

	/* Returns whether this query is a 2D (XY Plane only) query. */
	FORCEINLINE bool Is2DQuery() const { return m_2DQuery; }

	/* Returns whether this query uses 3D slice logic. */
	FORCEINLINE bool Is3DSlice() const { return m_3DSlice; }

	/* Bind Dynamic Delegates. */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;
#endif
private:
	/* Calculates the range of this targeting.*/
	virtual float CalculateRange() const override;

	/* Helper method to process all potential results. */
	void ProcessResults(UAblAbilityContext& Context, const TArray<struct FOverlapResult>& Results, float _FOV, float _Height, float _Length) const;

	/* The Field of View (Angle/Azimuth) of the cone, in degrees. Supports Angles greater than 180 degrees. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "FOV", ClampMin=1.0f, ClampMax=360.0f, AblBindableProperty))
	float m_FOV; // Azimuth

	UPROPERTY()
	FGetAblFloat m_FOVDelegate;

	/* Length of the Cone. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Length", ClampMin=0.1f, AblBindableProperty))
	float m_Length;

	UPROPERTY()
	FGetAblFloat m_LengthDelegate;

	/* Height of the Cone, ignored if using 2D/XY only. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Height", ClampMin=0.1f, AblBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAblFloat m_HeightDelegate;

	/* Whether to use a 2D (XY Plane) Cone, or 3D cone. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting", meta = (DisplayName = "Use 2D Cone"))
	bool m_2DQuery;

	/* 3D Slice acts similar to a cone, except it does not pinch in towards the origin, but is instead an equal height throughout the volume (like a slice of cake).*/
	UPROPERTY(EditInstanceOnly, Category = "Targeting", meta = (DisplayName = "Use 3D Slice"))
	bool m_3DSlice;
};

#undef LOCTEXT_NAMESPACE