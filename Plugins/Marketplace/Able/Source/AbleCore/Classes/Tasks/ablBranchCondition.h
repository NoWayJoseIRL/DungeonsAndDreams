// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbilityContext.h"
#include "ablAbilityTypes.h"
#include "Engine/EngineTypes.h"
#include "InputAction.h"
#include "UObject/ObjectMacros.h"
#include "ablBranchCondition.generated.h"

class UAblAbility;
class UAblBranchTaskScratchPad;
class UPrimitiveComponent;

#define LOCTEXT_NAMESPACE "AblAbilityTask"

/* Base class for Branch Conditions. */
UCLASS(Abstract, EditInlineNew)
class ABLECORE_API UAblBranchCondition : public UObject
{
	GENERATED_BODY()
public:
	UAblBranchCondition(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblBranchCondition();

	/* Returns if the result for this condition should be negated. */
	FORCEINLINE bool IsNegated() const { return m_Negate; }

	/* Checks the condition and returns the result. */
	virtual EAblConditionResults CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, UAblBranchTaskScratchPad& ScratchPad) const { return EAblConditionResults::ACR_Failed; }

	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability) {};

#if WITH_EDITOR
	/* Data Validation Tests. */
    virtual EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) { return EDataValidationResult::Valid; }

	/* Fix our flags. */
	bool FixUpObjectFlags();
#endif
protected:
	// If true, it reverses the result of the logic check (true becomes false, false becomes true).
	UPROPERTY(EditInstanceOnly, Category = "Logic", meta = (DisplayName = "Negate"))
	bool m_Negate;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

UCLASS(EditInlineNew, meta=(DisplayName="On Input", ShortToolTip="Branch if the player presses the given input."))
class UAblBranchConditionOnInput : public UAblBranchCondition
{
	GENERATED_BODY()
public:
	UAblBranchConditionOnInput(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblBranchConditionOnInput();

	/* Checks the condition and returns the result. */
	virtual EAblConditionResults CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, UAblBranchTaskScratchPad& ScratchPad) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAblAbility* Ability);

#if WITH_EDITOR
	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;
#endif
protected:
	// The input actions to look for. This expects the action keys to be bound to an action in the Input Settings.
	UPROPERTY(EditInstanceOnly, Category = "Input", meta = (DisplayName = "Input Actions", AblBindableProperty))
	TArray<FName> m_InputActions;

	UPROPERTY()
	FGetNameArray m_InputActionsDelegate;

	// If true, the key must have been released before counting it as a press.
	UPROPERTY(EditInstanceOnly, Category = "Input", meta = (DisplayName = "Must Be Recently Pressed"))
	bool m_MustBeRecentlyPressed;

	// The time, in seconds, a button is allowed to be down to count as a "recent" press. Avoid 0.0 as floating point error makes that an unlikely value.
	UPROPERTY(EditInstanceOnly, Category = "Input", meta = (DisplayName = "Recently Pressed Time Limit", EditCondition="m_MustBeRecentlyPressed", ClampMin = 0.0f))
	float m_RecentlyPressedTimeLimit;

	UPROPERTY(EditAnywhere, Category = "Input|Enhanced", meta = (DisplayName = "Use Enhanced Input"))
	bool m_UseEnhancedInput;

	UPROPERTY(EditAnywhere, Category = "Input|Enhanced", meta = (DisplayName = "Enhanced Input Actions"))
	TArray<const UInputAction*> m_EnhancedInputActions;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Always", ShortToolTip = "Always branch."))
class UAblBranchConditionAlways : public UAblBranchCondition
{
	GENERATED_BODY()
public:
	UAblBranchConditionAlways(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblBranchConditionAlways();

	/* Checks the condition and returns the result. */
	virtual EAblConditionResults CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, UAblBranchTaskScratchPad& ScratchPad) const override { return EAblConditionResults::ACR_Passed; }

#if WITH_EDITOR
	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;
#endif
};

UCLASS(EditInlineNew, meta = (DisplayName = "Custom", ShortToolTip = "This will call into the owning Ability's CustomCanBranchTo Blueprint event to allow for any custom logic you wish to use."))
class UAblBranchConditionCustom : public UAblBranchCondition
{
	GENERATED_BODY()
public:
	UAblBranchConditionCustom(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblBranchConditionCustom();

	/* Checks the condition and returns the result. */
	virtual EAblConditionResults CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, UAblBranchTaskScratchPad& ScratchPad) const override;

#if WITH_EDITOR
	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;
#endif
};

#undef LOCTEXT_NAMESPACE