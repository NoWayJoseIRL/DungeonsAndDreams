// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Channeling/ablChannelingConditions.h"

#include "ablAbility.h"
#include "ablAbilityContext.h"
#include "ablAbilityUtilities.h"
#include "AbleCorePrivate.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblChannelingInputConditional::UAblChannelingInputConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_UseEnhancedInput(false)
{

}

UAblChannelingInputConditional::~UAblChannelingInputConditional()
{

}

EAblConditionResults UAblChannelingInputConditional::CheckConditional(UAblAbilityContext& Context) const
{
	if (UAblAbilityComponent* AbilityComponent = Context.GetSelfAbilityComponent())
	{
		if (!AbilityComponent->IsOwnerLocallyControlled())
		{
			// We can only check Input on local clients (due to keybindings, etc), assume its valid unless Client tells us otherwise.
			return EAblConditionResults::ACR_Ignored;
		}
	}

#if WITH_EDITOR
	// UI Settings could change out from under us.
	m_InputKeyCache.Empty(m_InputKeyCache.Num());
#endif


	if (!m_InputKeyCache.Num() && !m_UseEnhancedInput)
	{
		for (const FName& InputAction : m_InputActions)
		{
			m_InputKeyCache.Append(FAblAbilityUtilities::GetKeysForInputAction(InputAction));
		}
	}

	if (const AActor* SelfActor = Context.GetSelfActor())
	{
		if (const APawn* Pawn = Cast<APawn>(SelfActor))
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
					for (const FKey& Key : m_InputKeyCache)
					{
						if (PlayerController->IsInputKeyDown(Key))
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

#if WITH_EDITOR
EDataValidationResult UAblChannelingInputConditional::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_InputActions.Num() == 0 && m_EnhancedInputActions.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoInputActions", "No Input actions for channel conditional: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAblChannelingVelocityConditional::UAblChannelingVelocityConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_VelocityThreshold(1.0f),
	m_UseXYSpeed(true)
{

}

UAblChannelingVelocityConditional::~UAblChannelingVelocityConditional()
{

}

void UAblChannelingVelocityConditional::BindDynamicDelegates(UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_VelocityThreshold, "Velocity Threshold");
}

EAblConditionResults UAblChannelingVelocityConditional::CheckConditional(UAblAbilityContext& Context) const
{
	bool success = false;
	if (const AActor* SelfActor = Context.GetSelfActor())
	{
		const FVector Velocity = SelfActor->GetVelocity();
		const float Threshold = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_VelocityThreshold);
		const float ThresholdSquared = Threshold * Threshold;

		if (m_UseXYSpeed)
		{
			success = Velocity.SizeSquared2D() < ThresholdSquared;
		}
		else
		{
			success = Velocity.SizeSquared() < ThresholdSquared;
		}
	}

	return success ? EAblConditionResults::ACR_Passed : EAblConditionResults::ACR_Failed;
}

#if WITH_EDITOR
EDataValidationResult UAblChannelingVelocityConditional::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_VelocityThreshold == 0.0f)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("InvalidVelocity", "Velocity threshold is zero, condition will never be true: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAblChannelingCustomConditional::UAblChannelingCustomConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_EventName(NAME_None)
{

}

UAblChannelingCustomConditional::~UAblChannelingCustomConditional()
{

}

EAblConditionResults UAblChannelingCustomConditional::CheckConditional(UAblAbilityContext& Context) const
{
	check(Context.GetAbility() != nullptr);
	return Context.GetAbility()->CheckCustomChannelConditionalBP(&Context, m_EventName);
}

#if WITH_EDITOR
EDataValidationResult UAblChannelingCustomConditional::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_EventName.IsNone())
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoEventName", "No Event Name For Custom Channel Conditional: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CheckCustomChannelConditionalBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CheckCustomChannelConditionalBP_NotFound", "Function 'CheckCustomChannelConditionalBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    
    return result;
}
#endif

#undef LOCTEXT_NAMESPACE