// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablValidation.h"

#include "Tasks/ablCollisionQueryTask.h"
#include "Tasks/ablRayCastQueryTask.h"
#include "Tasks/ablCollisionSweepTask.h"

bool UAbleValidation::CheckDependencies(const TArray<const UAblAbilityTask *>& taskDependencies)
{
    bool hasValidDependency = false;

    for (const UAblAbilityTask * taskDep : taskDependencies)
    {
		if (!taskDep)
		{
			continue;
		}

        if (const UAblRayCastQueryTask* task = Cast<UAblRayCastQueryTask>(taskDep))
        {
            if (task->GetCopyResultsToContext())
                hasValidDependency = true;
        }
        if (const UAblCollisionQueryTask* task = Cast<UAblCollisionQueryTask>(taskDep))
        {
            if (task->GetCopyResultsToContext())
                hasValidDependency = true;
        }
        if (const UAblCollisionSweepTask* task = Cast<UAblCollisionSweepTask>(taskDep))
        {
            if (task->GetCopyResultsToContext())
                hasValidDependency = true;
        }
    }

    return hasValidDependency;
}
