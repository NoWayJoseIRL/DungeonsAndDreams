// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "AbilityEditor/AblAbilityTaskValidator.h"
#include "AbleEditorEventManager.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Runtime/Core/Public/Misc/ScopeLock.h"
#include "Tasks/ablCollisionQueryTask.h"
#include "Tasks/ablCollisionSweepTask.h"
#include "Tasks/ablPlayAnimationTask.h"
#include "Tasks/ablRayCastQueryTask.h"
#include "Tasks/IAblAbilityTask.h"

#define LOCTEXT_NAMESPACE "AbleEditor"

UAblAbilityTaskValidator::UAblAbilityTaskValidator()
{

}

UAblAbilityTaskValidator::~UAblAbilityTaskValidator()
{

}

UAblAbilityTaskValidatorContext::UAblAbilityTaskValidatorContext()
{

}

UAblAbilityTaskValidatorContext::~UAblAbilityTaskValidatorContext()
{

}

void UAblAbilityTaskValidatorContext::Initialize(const UAblAbility* InAbility)
{
	m_Ability = InAbility;

	m_OutputLines.Empty();
}

void UAblAbilityTaskValidatorContext::AddInfo(const FText& InInfo)
{
	FScopeLock Lock(&m_LogCS);
	m_OutputLines.Add(FAblAbilityValidatorLogLine(1 << (int8)EAblAbilityValidatorLogType::Log, InInfo));

	if (m_WriteToLog)
	{
		UE_LOG(LogAbleEditor, Log, TEXT("%s"), *InInfo.ToString());
	}
}


void UAblAbilityTaskValidatorContext::AddWarning(const FText& InWarning)
{
	FScopeLock Lock(&m_LogCS);
	m_OutputLines.Add(FAblAbilityValidatorLogLine(1 << (int8)EAblAbilityValidatorLogType::Warning, InWarning));

	if (m_WriteToLog)
	{
		UE_LOG(LogAbleEditor, Warning, TEXT("%s"), *InWarning.ToString());
	}
}

void UAblAbilityTaskValidatorContext::AddError(const FText& InError)
{
	FScopeLock Lock(&m_LogCS);
	m_OutputLines.Add(FAblAbilityValidatorLogLine(1 << (int8)EAblAbilityValidatorLogType::Error, InError));

	if (m_WriteToLog)
	{
		UE_LOG(LogAbleEditor, Error, TEXT("%s"), *InError.ToString());
	}
}

UAblAbilityLengthValidator::UAblAbilityLengthValidator()
{
	// We set the Task we're interested in as the root Ability Task. This ensures this task will always get run (and it should as it's not task specific).
	m_TaskClass = UAblAbilityTask::StaticClass();
}

UAblAbilityLengthValidator::~UAblAbilityLengthValidator()
{

}

void UAblAbilityLengthValidator::Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const
{
	const float AbilityLength = Context->GetAbility()->GetLength();
	const TArray<UAblAbilityTask*>& AllTasks = Context->GetAbility()->GetTasks();

	float LastTaskEndTime = 0.0f;
	for (const UAblAbilityTask* AbilityTask : AllTasks)
	{
		if (!AbilityTask)
		{
			continue;
		}

		if (AbilityTask->GetEndTime() > AbilityLength)
		{
			FText WarningMessage = FText::Format(LOCTEXT("AbilityValidatorTaskTooLong", "Task {0} extends past the length of the ability. Is this intended?"), AbilityTask->GetTaskName());
			Context->AddWarning(WarningMessage);
		}

		LastTaskEndTime = FMath::Max(AbilityTask->GetEndTime(), LastTaskEndTime);
	}

	if (AbilityLength - LastTaskEndTime > KINDA_SMALL_NUMBER)
	{
		FText WarningMessage = FText::FormatOrdered(LOCTEXT("AbilityValidatorEmptySpace", "There is empty space in the timeline. The last task finishes at {0} but the ability has a length of {1}. Is this intended?"), LastTaskEndTime, AbilityLength);
		Context->AddWarning(WarningMessage);
	}
}

UAblAbilityTaskOrderValidator::UAblAbilityTaskOrderValidator()
{
	m_TaskClass = UAblAbilityTask::StaticClass();
}

UAblAbilityTaskOrderValidator::~UAblAbilityTaskOrderValidator()
{

}

void UAblAbilityTaskOrderValidator::Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const
{
	const float AbilityLength = Context->GetAbility()->GetLength();
	const TArray<UAblAbilityTask*>& AllTasks = Context->GetAbility()->GetTasks();
	int j = 0;
	float TimeDifference = 0.0f;
	for (int i = 0; i < AllTasks.Num() - 1; ++i)
	{
		if (!AllTasks[i])
		{
			continue;
		}

		for (j = i + 1; j < AllTasks.Num(); ++j)
		{
			if (!AllTasks[j])
			{
				continue;
			}

			TimeDifference = AllTasks[j]->GetStartTime() - AllTasks[i]->GetStartTime();
			// The 0.0 check is just there to catch any constants people may be using.
			if (TimeDifference != 0.0f && TimeDifference < KINDA_SMALL_NUMBER)
			{
				Context->AddError(LOCTEXT("AbilityValidatorTasksOutOfOrder", "Tasks aren't in sequential order. Please resave the ability."));
				return; // We can early out.
			}
		}
	}
}

UAblAbilityTaskPlayAnimationAssetValidator::UAblAbilityTaskPlayAnimationAssetValidator()
{
	m_TaskClass = UAblPlayAnimationTask::StaticClass();
}

UAblAbilityTaskPlayAnimationAssetValidator::~UAblAbilityTaskPlayAnimationAssetValidator()
{

}

void UAblAbilityTaskPlayAnimationAssetValidator::Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const
{
	const UAblPlayAnimationTask* PlayAnimationTask = CastChecked<UAblPlayAnimationTask>(Task.Get());

	const UAnimationAsset* AnimationAsset = PlayAnimationTask->GetAnimationAsset();
	if (AnimationAsset == nullptr)
	{
		Context->AddError(LOCTEXT("AbilityValidatorPlayAnimationNoAsset", "Play Animation Task does not have a animation asset specified."));
	}
	else if (!AnimationAsset->IsA<UAnimSequenceBase>() && !AnimationAsset->IsA<UAnimMontage>())
	{
		Context->AddError(LOCTEXT("AbilityValidatorPlayAnimationWrongAssetType", "Play Animation Task has an invalid asset specified. Only Sequences and Montages are supported."));
	}

	if (PlayAnimationTask->GetAnimationMode() == EAblPlayAnimationTaskAnimMode::AbilityAnimationNode)
	{
		// None is our default value, so if that hasn't changed we need to tell the user.
		if (PlayAnimationTask->GetStateMachineName().IsNone())
		{
			Context->AddError(LOCTEXT("AbilityValidatorPlayAnimationInvalidStateMachine", "Play Animation Task is set to use Ability Animation Mode, but doesn't have a State Machine Name specified."));
		}

		if (PlayAnimationTask->GetAbilityStateName().IsNone())
		{
			Context->AddError(LOCTEXT("AbilityValidatorPlayAnimationInvalidAbilityNode", "Play Animation Task is set to use Ability Animation Mode, but doesn't have a Ability State Name specified."));
		}
	}
}

UAblAbilityTaskCollisionQueryValidator::UAblAbilityTaskCollisionQueryValidator()
{
	m_TaskClass = UAblCollisionQueryTask::StaticClass();
}

UAblAbilityTaskCollisionQueryValidator::~UAblAbilityTaskCollisionQueryValidator()
{

}

void UAblAbilityTaskCollisionQueryValidator::Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const
{
	const UAblCollisionQueryTask* QueryTask = CastChecked<const UAblCollisionQueryTask>(Task.Get());

	if (!QueryTask->GetFireEvent() && !QueryTask->GetCopyResultsToContext())
	{
		Context->AddWarning(LOCTEXT("AbilityValidatorCollisionQueryNoOutput", "Collision Query Task is set to not fire its event or copy its results to the context. This Task won't be able to do anything with its results."));
	}
}

UAblAbilityTaskCollisionSweepValidator::UAblAbilityTaskCollisionSweepValidator()
{
	m_TaskClass = UAblCollisionSweepTask::StaticClass();
}

UAblAbilityTaskCollisionSweepValidator::~UAblAbilityTaskCollisionSweepValidator()
{

}

void UAblAbilityTaskCollisionSweepValidator::Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const
{
	const UAblCollisionSweepTask* SweepTask = CastChecked<const UAblCollisionSweepTask>(Task.Get());

	if (!SweepTask->GetFireEvent() && !SweepTask->GetCopyResultsToContext())
	{
		Context->AddWarning(LOCTEXT("AbilityValidatorCollisionSweepNoOutput", "Collision Sweep Task is set to not fire its event or copy its results to the context. This Task won't be able to do anything with its results."));
	}
}

UAblAbilityTaskRaycastValidator::UAblAbilityTaskRaycastValidator()
{
	m_TaskClass = UAblRayCastQueryTask::StaticClass();
}

UAblAbilityTaskRaycastValidator::~UAblAbilityTaskRaycastValidator()
{

}

void UAblAbilityTaskRaycastValidator::Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const
{
	const UAblRayCastQueryTask* RaycastTask = CastChecked<const UAblRayCastQueryTask>(Task.Get());

	if (!RaycastTask->GetFireEvent() && !RaycastTask->GetCopyResultsToContext())
	{
		Context->AddWarning(LOCTEXT("AbilityValidatorRayCastNoOutput", "Raycast Task is set to not fire its event or copy its results to the context. This Task won't be able to do anything with its results."));
	}
}


UAblAbilityTaskDependencyValidator::UAblAbilityTaskDependencyValidator()
{
	m_TaskClass = UAblAbilityTask::StaticClass();
}

UAblAbilityTaskDependencyValidator::~UAblAbilityTaskDependencyValidator()
{

}

void UAblAbilityTaskDependencyValidator::Validate(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const TWeakObjectPtr<const UAblAbilityTask>& Task) const
{
	const TArray<UAblAbilityTask*>& AllTasks = Context->GetAbility()->GetTasks();
	for (const UAblAbilityTask* CurrentTask : AllTasks)
	{
		if (CurrentTask)
		{
			HasCircularDependency(Context, CurrentTask, CurrentTask);
		}
	}
}

bool UAblAbilityTaskDependencyValidator::HasCircularDependency(const TWeakObjectPtr<UAblAbilityTaskValidatorContext>& Context, const UAblAbilityTask* TaskToCheck, const UAblAbilityTask* CurrentTask) const
{
	static TArray<const UAblAbilityTask*> Stack;
	Stack.Push(CurrentTask);

	bool Result = false;
	if (CurrentTask->HasDependencies())
	{
		for (const UAblAbilityTask* Dependency : CurrentTask->GetTaskDependencies())
		{
			if (Dependency == TaskToCheck)
			{
				FText ErrorMessage = FText::FormatOrdered(LOCTEXT("AbilityValidatorTaskCirculeDependency", "Circular Dependency Detected for Task {0}, dependency stack:"), TaskToCheck->GetTaskName());
				Context->AddError(ErrorMessage);

				Result = true;
				break;
			}
			else
			{
				if (!Stack.Contains(Dependency) && HasCircularDependency(Context, TaskToCheck, Dependency))
				{
					Context->AddError(FText::FormatOrdered(LOCTEXT("AbilityValidatorTaskCircularDependencyStack", "Task {0} depends on {1}"), CurrentTask->GetTaskName(), Dependency->GetTaskName()));

					Result = true;
					break;
				}
			}
		}
	}

	Stack.Pop();
	return Result;
}

#undef LOCTEXT_NAMESPACE

