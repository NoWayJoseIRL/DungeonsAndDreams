// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablBranchCondition.h"

#include "ablAbility.h"
#include "ablAbilityUtilities.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Tasks/ablBranchTask.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblBranchCondition::UAblBranchCondition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Negate(false),
	m_DynamicPropertyIdentifer()
{

}

UAblBranchCondition::~UAblBranchCondition()
{

}

FName UAblBranchCondition::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_BranchCondition_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

UAblBranchConditionOnInput::UAblBranchConditionOnInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_InputActions(),
	m_MustBeRecentlyPressed(false),
	m_RecentlyPressedTimeLimit(0.1f),
	m_UseEnhancedInput(false)
{

}

UAblBranchConditionOnInput::~UAblBranchConditionOnInput()
{

}

EAblConditionResults UAblBranchConditionOnInput::CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, UAblBranchTaskScratchPad& ScratchPad) const
{
	// Build our cache
	if (ScratchPad.CachedKeys.Num() == 0 && !m_UseEnhancedInput)
	{
		TArray<FName> InputActions = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_InputActions);
		for (const FName& Action : InputActions)
		{
			ScratchPad.CachedKeys.Append(FAblAbilityUtilities::GetKeysForInputAction(Action));
		}
	}

	if (UAblAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		if (!AbilityComponent->IsOwnerLocallyControlled())
		{
			// We can only check Input on local clients.
			return EAblConditionResults::ACR_Ignored;
		}
	}

	if (const APawn* Pawn = Cast<APawn>(Context->GetSelfActor()))
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			UEnhancedPlayerInput* EPI = Cast<UEnhancedPlayerInput>(PlayerController->PlayerInput);
			if (EPI && m_UseEnhancedInput)
			{
				for (const UInputAction* Action : m_EnhancedInputActions)
				{
					if (EPI->GetActionValue(Action).Get<bool>())
					{
						return EAblConditionResults::ACR_Passed;
					}
				}
			}
			else
			{
				for (const FKey& Key : ScratchPad.CachedKeys)
				{
					if (PlayerController->IsInputKeyDown(Key))
					{
						if (m_MustBeRecentlyPressed)
						{
							return PlayerController->GetInputKeyTimeDown(Key) <= m_RecentlyPressedTimeLimit ? EAblConditionResults::ACR_Passed: EAblConditionResults::ACR_Failed;
						}
						else
						{
							return EAblConditionResults::ACR_Passed;
						}
					}
				}
			}
		}
	}

	return EAblConditionResults::ACR_Failed;
}

void UAblBranchConditionOnInput::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_InputActions, TEXT("Input Actions"));
}

#if WITH_EDITOR
EDataValidationResult UAblBranchConditionOnInput::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_InputActions.Num() == 0 && m_EnhancedInputActions.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoInputActions", "No Input Actions Defined: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}

bool UAblBranchCondition::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));

	if (oldFlags != GetFlags())
	{
		Modify();

		return true;
	}

	return false;
}

#endif

UAblBranchConditionAlways::UAblBranchConditionAlways(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblBranchConditionAlways::~UAblBranchConditionAlways()
{

}

#if WITH_EDITOR
EDataValidationResult UAblBranchConditionAlways::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAblBranchConditionCustom::UAblBranchConditionCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}


UAblBranchConditionCustom::~UAblBranchConditionCustom()
{

}

EAblConditionResults UAblBranchConditionCustom::CheckCondition(const TWeakObjectPtr<const UAblAbilityContext>& Context, UAblBranchTaskScratchPad& ScratchPad) const
{
	check(Context.IsValid() && ScratchPad.BranchAbility);

	return Context.Get()->GetAbility()->CustomCanBranchToBP(Context.Get(), ScratchPad.BranchAbility) ? EAblConditionResults::ACR_Passed : EAblConditionResults::ACR_Failed;
}

#if WITH_EDITOR
EDataValidationResult UAblBranchConditionCustom::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    
    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CustomCanBranchToBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CustomCanBranchToBP_NotFound", "Function 'CustomCanBranchToBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    
    return result;
}
#endif

#undef LOCTEXT_NAMESPACE