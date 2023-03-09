// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AbilityEditor/AblAbilityTaskValidator.h"
#include "Tasks/IAblAbilityTask.h"

#include "ablAbilityValidator.generated.h"

class UAblAbility;

USTRUCT()
struct FAblAbilityTaskValidatorArray
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UAblAbilityTaskValidator>> Validators;
};

/* An Ability Validator validates an Ability against all registered Validators. The results can be retrieved from the Context once completed. */
UCLASS(Transient)
class UAblAbilityValidator : public UObject
{
	GENERATED_BODY()
public:
	UAblAbilityValidator();
	virtual ~UAblAbilityValidator();

	void Validate(const UAblAbility& Ability);
	const UAblAbilityTaskValidatorContext& GetContext() const { return *m_Context.Get(); }
private:
	void BuildTaskValidatorMap();

	UPROPERTY(Transient)
	TWeakObjectPtr<UAblAbilityTaskValidatorContext> m_Context;

	UPROPERTY(Transient)
	TMap<TSubclassOf<UAblAbilityTask>, FAblAbilityTaskValidatorArray> m_TaskToValidatorArrayMap;
};