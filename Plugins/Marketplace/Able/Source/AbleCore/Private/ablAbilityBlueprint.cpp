// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ablAbilityBlueprint.h"
#include "ablAbilityBlueprintGeneratedClass.h"

UAblAbilityBlueprint::UAblAbilityBlueprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR

UClass* UAblAbilityBlueprint::GetBlueprintClass() const
{
	return UAblAbilityBlueprintGeneratedClass::StaticClass();
}


/** Returns the most base ability blueprint for a given blueprint (if it is inherited from another ability blueprint, returning null if only native / non-ability BP classes are it's parent) */
UAblAbilityBlueprint* UAblAbilityBlueprint::FindRootGameplayAbilityBlueprint(UAblAbilityBlueprint* DerivedBlueprint)
{
	UAblAbilityBlueprint* ParentBP = NULL;

	// Determine if there is a ability blueprint in the ancestry of this class
	for (UClass* ParentClass = DerivedBlueprint->ParentClass; ParentClass != UObject::StaticClass(); ParentClass = ParentClass->GetSuperClass())
	{
		if (UAblAbilityBlueprint* TestBP = Cast<UAblAbilityBlueprint>(ParentClass->ClassGeneratedBy))
		{
			ParentBP = TestBP;
		}
	}

	return ParentBP;
}

#endif