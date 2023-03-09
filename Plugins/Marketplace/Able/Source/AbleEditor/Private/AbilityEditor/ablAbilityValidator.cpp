// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/ablAbilityValidator.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "AbilityEditor/AblAbilityTaskValidator.h"
#include "ProfilingDebugging/ScopedTimers.h"

#define LOCTEXT_NAMESPACE "AbilityEditor"

class FAbilityTaskValidatorTask
{
public:
	// We make a copy of the Current time since it is written to during the sync update.
	FAbilityTaskValidatorTask(UAblAbilityTaskValidatorContext* InContext, const UAblAbilityTask* InTask, const FAblAbilityTaskValidatorArray* InValidators)
		: m_Context(InContext),
		m_Task(InTask),
		m_Validators(InValidators)
	{ }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (m_Validators && m_Task)
		{
			for (const TWeakObjectPtr<UAblAbilityTaskValidator>& Validator : m_Validators->Validators)
			{
				Validator->Validate(m_Context, m_Task);
			}
		}
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::AnyThread; }
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAbilityTaskValidatorTask, STATGROUP_TaskGraphTasks);
	}

private:
	TWeakObjectPtr<UAblAbilityTaskValidatorContext> m_Context;
	const UAblAbilityTask* m_Task;
	const FAblAbilityTaskValidatorArray* m_Validators;
};

UAblAbilityValidator::UAblAbilityValidator()
{

}

UAblAbilityValidator::~UAblAbilityValidator()
{

}

void UAblAbilityValidator::Validate(const UAblAbility& Ability)
{
	if (!m_Context.IsValid())
	{
		m_Context = NewObject<UAblAbilityTaskValidatorContext>(this);
	}

	if (m_TaskToValidatorArrayMap.Num() == 0)
	{
		BuildTaskValidatorMap();
	}

	m_Context->Initialize(&Ability);

	// Since we're a blueprint, the name gets appended with a variation marker _A, _B, _C, etc. Remove that for display purposes.
	FString AbilityName = Ability.GetClass()->GetName();
	int VariationMarker = -1;
	AbilityName.FindLastChar('_', VariationMarker);
	if (VariationMarker > 0)
	{
		AbilityName.RemoveAt(VariationMarker, AbilityName.Len() - VariationMarker);
	}

	m_Context->AddInfo(FText::Format(LOCTEXT("AblAbilityValidatorStart", "Validating ability {0}..."), FText::FromString(AbilityName)));
	
	double TotalTime = 0.0f;
	FDurationTimer Timer(TotalTime);
	Timer.Start();

	int32 TotalValidatorsRan = 0;

	// Helper if we use the Task Graph.
	FGraphEventArray LaunchedTasks;

	// Run Task specific validators.
	const TArray<UAblAbilityTask*>& Tasks = Ability.GetTasks();

	if (Tasks.Num())
	{
		for (UAblAbilityTask* Task : Tasks)
		{
			if (!Task)
			{
				continue;
			}

			if (const FAblAbilityTaskValidatorArray* ValidatorArray = m_TaskToValidatorArrayMap.Find(Task->GetClass()))
			{
				TotalValidatorsRan += ValidatorArray->Validators.Num();

				if (FPlatformMisc::NumberOfCores() > 2)
				{
					// Let's spread this work out to some other cores, since we could have N validators and some tests could be complex/expensive.
					LaunchedTasks.Add(TGraphTask<FAbilityTaskValidatorTask>::CreateTask().ConstructAndDispatchWhenReady(m_Context.Get(), Task, ValidatorArray));
				}
				else
				{
					for (const TWeakObjectPtr<UAblAbilityTaskValidator>& Validator : ValidatorArray->Validators)
					{
						Validator->Validate(m_Context, Task);
					}
				}
			}
		}

		// Run Generic Validators (these are attached to the UAblAbilityTask base class).
		// We could also toss these on the graph, but might as well do them now while we wait for other tasks to finish.
		if (const FAblAbilityTaskValidatorArray* ValidatorArray = m_TaskToValidatorArrayMap.Find(UAblAbilityTask::StaticClass()))
		{
			TotalValidatorsRan += ValidatorArray->Validators.Num();
			for (const TWeakObjectPtr<UAblAbilityTaskValidator>& Validator : ValidatorArray->Validators)
			{
				// Just pass in some task. It shouldn't be relied on.
				Validator->Validate(m_Context, Tasks[0]);
			}
		}

		// If we launched some tasks, wait for them to finish up.
		if (LaunchedTasks.Num() > 0)
		{
			FTaskGraphInterface::Get().WaitUntilTasksComplete(LaunchedTasks);
		}
	}

	Timer.Stop();

	m_Context->AddInfo(FText::FormatOrdered(LOCTEXT("AblAbilityValidatorEnd", "Finished validation in {0} seconds using {1} validators."), TotalTime, TotalValidatorsRan));
}

void UAblAbilityValidator::BuildTaskValidatorMap()
{
	for (TObjectIterator<UClass> ClassIter; ClassIter; ++ClassIter)
	{
		if (!ClassIter->IsChildOf(UAblAbilityTaskValidator::StaticClass()) || ClassIter->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		if (UAblAbilityTaskValidator* Validator = Cast<UAblAbilityTaskValidator>(ClassIter->GetDefaultObject()))
		{
			if (!Validator->GetTaskClass().GetDefaultObject())
			{
				UE_LOG(LogAbleEditor, Error, TEXT("Task Validator [%s] does not have a valid Task Class. Did you remember to override GetTaskClass?"), *(Validator->GetClass()->GetName()));
				continue;
			}

			if (FAblAbilityTaskValidatorArray* ValidatorArray = m_TaskToValidatorArrayMap.Find(Validator->GetTaskClass()))
			{
				ValidatorArray->Validators.AddUnique(Validator);
			}
			else
			{
				FAblAbilityTaskValidatorArray& NewValidator = m_TaskToValidatorArrayMap.Add(Validator->GetTaskClass());
				NewValidator.Validators.AddUnique(Validator);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE