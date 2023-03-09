// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbleEditorEventManager.h"

#include "AbleEditorPrivate.h"

#include "Tasks/IAblAbilityTask.h"

TSharedPtr<FAbleEditorEventManager> FAbleEditorEventManager::m_Instance = nullptr;

FAbleEditorEventManager::FAbleEditorEventManager()
{

}

FAbleEditorEventManager::~FAbleEditorEventManager()
{

}

FAbleEditorEventManager& FAbleEditorEventManager::Get()
{
	if (!m_Instance.IsValid())
	{
		m_Instance = MakeShareable(new FAbleEditorEventManager());
	}
	check(m_Instance.IsValid());
	return *(m_Instance.Get());
}

FOnAblAbilityTaskCreated& FAbleEditorEventManager::OnAbilityTaskCreated(const TSubclassOf<class UAblAbilityTask>& TaskClass)
{
	if (!m_ClassToDelegateMap.Contains(TaskClass))
	{
		m_ClassToDelegateMap.Add(TaskClass, FOnAblAbilityTaskCreated());
	}
	return m_ClassToDelegateMap[TaskClass];
}

void FAbleEditorEventManager::BroadcastAbilityEditorInstantiated(class FAblAbilityEditor& AbilityEditor)
{
	m_AbilityEditorInstantiatedDelegate.Broadcast(AbilityEditor);
}

void FAbleEditorEventManager::BroadcastAnyAbilityTaskCreated(class FAblAbilityEditor& AbilityEditor, class UAblAbilityTask& Task)
{
	m_AnyAbilityTaskCreatedDelegate.Broadcast(AbilityEditor, Task);
}

void FAbleEditorEventManager::BroadcastAbilityTaskCreated(class FAblAbilityEditor& AbilityEditor, class UAblAbilityTask& Task)
{
	if (m_ClassToDelegateMap.Contains(Task.GetClass()))
	{
		m_ClassToDelegateMap[Task.GetClass()].Broadcast(AbilityEditor, Task);
	}
}
