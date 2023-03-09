// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"

class FAblAbilityEditor;
class UAblAbility;

/* Use this delegate to register any custom Task handlers / anything else you wish to do when the Ability Editor is opened. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAbilityEditorInstantiated, class FAblAbilityEditor& /*AbilityEditor*/);

/* This delegate will fire whenever an Ability task is added to the current Ability being edited. 
 * This is just here as a catch all. The preferred method for doing any extraneous actions when a Task is created is to 
 * use the specific OnAbilityTaskCreated delegate which is only called when a specific Task is instantiated. 
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAnyAblAbilityTaskCreated, class FAblAbilityEditor& /*AbilityEditor*/, class UAblAbilityTask& /*Task*/);

/* Called when a specific type of Task is called and the delegate has been registered using the OnAbilityTaskCreated(...) getter. 
 * This is useful if you need to do any extra processing, or want to toss up an extra options window, etc for specific tasks. 
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAblAbilityTaskCreated, class FAblAbilityEditor& /*AbilityEditor*/, class UAblAbilityTask& /*Task*/);

/* The AbleEditorEventManager is a singleton that allows other classes to hook into Able related events without having to touch the Able code itself. */
class FAbleEditorEventManager
{
public:
	static FAbleEditorEventManager& Get();

	virtual ~FAbleEditorEventManager();

	/* Returns the OnAbilityEditorInstantiated delegate. */
	FOnAbilityEditorInstantiated& OnAbilityEditorInstantiated() { return m_AbilityEditorInstantiatedDelegate; }

	/* Returns the OnAnyAbilityTaskCreated delegate. */
	FOnAnyAblAbilityTaskCreated& OnAnyAbilityTaskCreated() { return m_AnyAbilityTaskCreatedDelegate; }

	/* Returns the OnAbilityTaskCreated delegate for the provided Task class. */
	FOnAblAbilityTaskCreated& OnAbilityTaskCreated(const TSubclassOf<class UAblAbilityTask>& TaskClass);
private:
	FAbleEditorEventManager();

	friend class FAblAbilityEditor; // Normally I'm really not fond of friend classes - but it makes sense in this case as we don't want these methods public.
	
	/* Broadcasts the OnAbilityEditorInstantiated event.*/
	void BroadcastAbilityEditorInstantiated(class FAblAbilityEditor& AbilityEditor);

	/* Broadcasts the OnAnyAbilityTaskCreated event along with the Task that caused the event. */
	void BroadcastAnyAbilityTaskCreated(class FAblAbilityEditor& AbilityEditor, class UAblAbilityTask& Task);

	/* Broadcasts the OnAbilityTaskCreated event for the class of the provided Task, as well as passing along the Task that caused the event. */
	void BroadcastAbilityTaskCreated(class FAblAbilityEditor& AbilityEditor, class UAblAbilityTask& Task);

	/* OnAbilityEditorInstantiated Delegate. */
	FOnAbilityEditorInstantiated m_AbilityEditorInstantiatedDelegate;

	/* OnAnyAbilityTaskCreated Delegate. */
	FOnAnyAblAbilityTaskCreated m_AnyAbilityTaskCreatedDelegate;

	/* Map of Task Class to OnAbilityTaskCreated Delegate.*/
	TMap<TSubclassOf<UAblAbilityTask>, FOnAblAbilityTaskCreated> m_ClassToDelegateMap;

	/* Our Event Manager Instance. */
	static TSharedPtr<FAbleEditorEventManager> m_Instance;
};