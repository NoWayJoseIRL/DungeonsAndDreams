// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Tasks/ablCustomTask.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "ablAbilityBlueprintLibrary.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

class UAblAbility;
struct FAblAbilityInstance;
class UAblAbilityContext;
class UAblCustomTaskScratchPad;

UCLASS()
class ABLECORE_API UAblAbilityBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	* Returns an instance (CDO) of the provided Ability Class.
	*
	* @param Ability SubClass to grab the CDO from.
	*
	* @return the default class instance.
	*/
	UFUNCTION(BlueprintPure, Category="Able|Ability")
	static UAblAbility* GetAbilityObjectFromClass(TSubclassOf<UAblAbility> Class);

	/**
	* Activates the Ability using the provided context and returns the result.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return the start result returned by the Ability.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	static EAblAbilityStartResult ActivateAbility(UAblAbilityContext* Context);

	/**
	* Creates an Ability Context with the provided information.
	*
	* @param Ability The Ability to execute.
	* @param AbilityComponent The Ability Component that will be running this Context.
	* @param Owner The "owner" to set as the Owner Target Type.
	* @param Instigator The "Instigator" to set as the Instigator Target Type.
	*
	* @return the Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad).
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	static UAblAbilityContext* CreateAbilityContext(const UAblAbility* Ability, UAblAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator);

	/**
	* Creates an Ability Context with the provided information.
	*
	* @param Ability The Ability to execute.
	* @param AbilityComponent The Ability Component that will be running this Context.
	* @param Owner The "owner" to set as the Owner Target Type.
	* @param Instigator The "Instigator" to set as the Instigator Target Type.
	* @param Target Sets the "Target Location" to the provided value.
	*
	* @return the Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad).
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	static UAblAbilityContext* CreateAbilityContextWithLocation(const UAblAbility* Ability, UAblAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator, FVector Location);

	/**
	* Returns true if the provided start result is considered successful.
	*
	* @param Result The result enum returned by another Ability method (CanActivate, Activate, etc).
	*
	* @return true if the result was a success, false if it failed for whatever reason.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	static bool IsSuccessfulStartResult(EAblAbilityStartResult Result);

	/**
	* Returns whether debugging drawing of collision queries is enabled or not.
	*
	* @return true if debug drawing is enabled.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Debug")
	static bool GetDrawCollisionQueries();

	/**
	* Toggles the viewing of collision queries within Able. Returns the new value.
	*
	* @param Enable whether to enable or disable debug drawing.
	*
	* @return whether or not debug drawing is now enabled.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Debug")
	static bool SetDrawCollisionQueries(bool Enable);

	/**
	* Creates a ScratchPad based on the UAblCustomScratchPad, for use by Custom Tasks.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param ScratchPadClass The Task Scratchpad Subclass to instantiate.
	*
	* @return the created Custom Scratchpad Object.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Custom Task")
	static UAblCustomTaskScratchPad* CreateCustomScratchPad(UAblAbilityContext* Context, TSubclassOf<UAblCustomTaskScratchPad> ScratchPadClass);
};

#undef LOCTEXT_NAMESPACE
