// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "HAL/CriticalSection.h"
#include "Tasks/IAblAbilityTask.h"
#include "Templates/SubclassOf.h"

#include "AblAbilityTaskValidator.generated.h"

class UAblAbility;

UENUM()
enum class EAblAbilityValidatorLogType : uint8
{
	Log = 0,
	Warning,
	Error,

	TotalTypes
};

/* Simple struct that represents a single line of output in our Validator results. */
USTRUCT()
struct FAblAbilityValidatorLogLine
{
	GENERATED_USTRUCT_BODY()
public:
	FAblAbilityValidatorLogLine()
		: LogFlags(0),
		Text()
	{

	}

	FAblAbilityValidatorLogLine(int8 InFlags, const FText& InText)
		: LogFlags(InFlags),
		Text(InText)
	{

	}

	UPROPERTY()
	int8 LogFlags;

	UPROPERTY()
	FText Text;
};

/* The Context of our Validator. This class holds all the relevant information for our Validators to be able to execute. */
UCLASS(Transient)
class UAblAbilityTaskValidatorContext : public UObject
{
	GENERATED_BODY()
public:
	UAblAbilityTaskValidatorContext();
	virtual ~UAblAbilityTaskValidatorContext();
	
	/* Initializes the Context and resets the output. */
	void Initialize(const UAblAbility* InAbility);

	/* If true, the output will also be copied to the UE4 Logging system. */
	void SetWriteToLog(bool InWriteToLog) { m_WriteToLog = InWriteToLog; }

	/* Adds an Info line to the output. */
	void AddInfo(const FText& InInfo);

	/* Adds a Warning line to the output. */
	void AddWarning(const FText& InWarning);

	/* Adds an Error line to the output. */
	void AddError(const FText& InError);

	/* Returns the Ability we are validating. */
	FORCEINLINE const TWeakObjectPtr<const UAblAbility>& GetAbility() const { return m_Ability; }

	/* Returns the Output of the validation. */
	const TArray<FAblAbilityValidatorLogLine>& GetOutput() const { return m_OutputLines; }

	/* Returns the number of lines written by validators. */
	int32 GetTotalOutputLines() const { return m_OutputLines.Num(); }
private:
	/* The Ability to Validate. */
	UPROPERTY(Transient)
	TWeakObjectPtr<const UAblAbility> m_Ability;

	/* Whether or not to copy the results to the UE4 logging system. */
	UPROPERTY(Transient)
	bool m_WriteToLog;
	
	/* Our output from the Validators. */
	UPROPERTY(Transient)
	TArray<FAblAbilityValidatorLogLine> m_OutputLines;

	/* Critical Section since Validators can access this object from other threads. */
	FCriticalSection m_LogCS;
};

/* A Validator is a Unit/Integration test (both are supported) that checks the associated Task or Ability for some data/behavior that could cause problems
 * and alerts the user to allow them to fix things.
 *
 * Validators are bound to specific Task classes, so they only run if an Ability contains that Task.
 * If you want to write an Ability specific (rather than task specific) validator, simply have your child class set m_TaskClass to UAblAbilityTask::Static_Class(), which
 * will ensure that no matter what Tasks are in an Ability - your test will be called. */
UCLASS(Abstract, Transient)
class ABLEEDITOR_API UAblAbilityTaskValidator : public UObject
{
	GENERATED_BODY()
public:
	UAblAbilityTaskValidator();
	virtual ~UAblAbilityTaskValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const { };

	const TSubclassOf<UAblAbilityTask>& GetTaskClass() const { return m_TaskClass; }
protected:
	UPROPERTY(transient)
	TSubclassOf<UAblAbilityTask> m_TaskClass;
};

/* A sample of a Validator. This simply checks the length of an Ability and reports if there is any dead space
*  or if a task runs past the end of the length. Both of those behaviors are benign and may be intended, but we should warn
*  the designer anyway.
*/
UCLASS(Transient)
class ABLEEDITOR_API UAblAbilityLengthValidator : public UAblAbilityTaskValidator
{
	GENERATED_BODY()
public:
	UAblAbilityLengthValidator();
	virtual ~UAblAbilityLengthValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const override;
};

/* Another simple Validator that just makes sure all our tasks are in proper order.*/
UCLASS(Transient)
class ABLEEDITOR_API UAblAbilityTaskOrderValidator : public UAblAbilityTaskValidator
{
	GENERATED_BODY()
public:
	UAblAbilityTaskOrderValidator();
	virtual ~UAblAbilityTaskOrderValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const override;
};

/* An Example of a Task specific Validator. This checks if we have a valid animation asset, and other critical fields, in any Play Animation tasks.*/
UCLASS(Transient)
class ABLEEDITOR_API UAblAbilityTaskPlayAnimationAssetValidator : public UAblAbilityTaskValidator
{
	GENERATED_BODY()
public:
	UAblAbilityTaskPlayAnimationAssetValidator();
	virtual ~UAblAbilityTaskPlayAnimationAssetValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const override;
};

/* Checks fields in Collision Query Tasks*/
UCLASS(Transient)
class ABLEEDITOR_API UAblAbilityTaskCollisionQueryValidator : public UAblAbilityTaskValidator
{
	GENERATED_BODY()
public:
	UAblAbilityTaskCollisionQueryValidator();
	virtual ~UAblAbilityTaskCollisionQueryValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const override;
};

/* Checks fields in Collision Sweep Tasks*/
UCLASS(Transient)
class ABLEEDITOR_API UAblAbilityTaskCollisionSweepValidator : public UAblAbilityTaskValidator
{
	GENERATED_BODY()
public:
	UAblAbilityTaskCollisionSweepValidator();
	virtual ~UAblAbilityTaskCollisionSweepValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const override;
};

/* Checks fields in Raycast Tasks*/
UCLASS(Transient)
class ABLEEDITOR_API UAblAbilityTaskRaycastValidator : public UAblAbilityTaskValidator
{
	GENERATED_BODY()
public:
	UAblAbilityTaskRaycastValidator();
	virtual ~UAblAbilityTaskRaycastValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const override;
};

/* Validate our Task Dependencies, checking for circular dependencies and the like. */
UCLASS(Transient)
class ABLEEDITOR_API UAblAbilityTaskDependencyValidator : public UAblAbilityTaskValidator
{
	GENERATED_BODY()
public:
	UAblAbilityTaskDependencyValidator();
	virtual ~UAblAbilityTaskDependencyValidator();

	virtual void Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const override;
private:
	bool HasCircularDependency(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const UAblAbilityTask* TaskToCheck, const UAblAbilityTask* CurrentTask) const;
};