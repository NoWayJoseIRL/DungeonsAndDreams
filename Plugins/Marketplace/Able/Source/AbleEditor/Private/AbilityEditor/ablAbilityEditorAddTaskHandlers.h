// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Templates/SharedPointer.h"

class FAblAbilityEditor;
class UAblAbilityTask;

/* Simple Handler that asks the user to select a UAnimationAsset whenever a Play Animation Task is created. */
class FAblPlayAnimationAddedHandler : public TSharedFromThis<FAblPlayAnimationAddedHandler>
{
public:
	FAblPlayAnimationAddedHandler();
	virtual ~FAblPlayAnimationAddedHandler();

	void Register();
	void OnTaskAdded(FAblAbilityEditor& Editor, UAblAbilityTask& Task);
};