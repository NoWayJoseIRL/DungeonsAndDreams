// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ablTargetingBase.h"
#include "UObject/ObjectMacros.h"
#include "Tasks/IAblAbilityTask.h"

#include "ablTargetingCapsule.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, meta = (DisplayName = "Capsule", ShortToolTip = "A capsule based targeting volume."))
class UAblTargetingCapsule : public UAblTargetingBase
{
	GENERATED_BODY()
public:
	UAblTargetingCapsule(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblTargetingCapsule();

	/* Find all the Targets within our query.*/
	virtual void FindTargets(UAblAbilityContext& Context) const override;

	virtual void BindDynamicDelegates(class UAblAbility* Ability) override;

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAblAbilityContext& Context, float DeltaTime) const override;
#endif

private:
	/* Calculate the range of our Query.*/
	virtual float CalculateRange() const override;

	/* Process the results of our Query. */
	void ProcessResults(UAblAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const;

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

#undef LOCTEXT_NAMESPACE