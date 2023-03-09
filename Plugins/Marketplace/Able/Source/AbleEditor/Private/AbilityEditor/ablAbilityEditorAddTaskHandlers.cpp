// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/ablAbilityEditorAddTaskHandlers.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/SAbilityAnimationSelector.h"
#include "ablAbility.h"
#include "AbleEditorEventManager.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"

#include "Tasks/ablPlayAnimationTask.h"
FAblPlayAnimationAddedHandler::FAblPlayAnimationAddedHandler()
{

}

FAblPlayAnimationAddedHandler::~FAblPlayAnimationAddedHandler()
{

}

void FAblPlayAnimationAddedHandler::Register()
{
	FOnAblAbilityTaskCreated& OnTaskCreated = FAbleEditorEventManager::Get().OnAbilityTaskCreated(UAblPlayAnimationTask::StaticClass());
	OnTaskCreated.AddSP(this, &FAblPlayAnimationAddedHandler::OnTaskAdded);
}

void FAblPlayAnimationAddedHandler::OnTaskAdded(FAblAbilityEditor& Editor, UAblAbilityTask& Task)
{
	if (UAblPlayAnimationTask* AnimationTask = Cast<UAblPlayAnimationTask>(&Task))
	{
		TSharedRef<SAblAbilityAnimationSelector> ModalWindow = SNew(SAblAbilityAnimationSelector);
		if (ModalWindow->DoModal() && ModalWindow->GetAnimationAsset().IsValid())
		{
			AnimationTask->SetAnimationAsset(ModalWindow->GetAnimationAsset().Get());

			if (ModalWindow->GetResizeAbilityLength())
			{
				if (UAblAbility* Ability = Editor.GetAbility())
				{
					// Fix this when getting the animation length isn't a non-const call. 
					if (/*const*/ UAnimMontage* Montage = Cast<UAnimMontage>(ModalWindow->GetAnimationAsset().Get()))
					{
						Ability->SetLength(Montage->CalculateSequenceLength());
					}
					else if (/*const*/ UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(ModalWindow->GetAnimationAsset().Get()))
					{
						Ability->SetLength(Sequence->GetPlayLength());
					}
				}
			}
		}
	}
}

