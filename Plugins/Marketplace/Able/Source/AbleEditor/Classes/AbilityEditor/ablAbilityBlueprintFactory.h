// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ablAbility.h"
#include "Engine/Blueprint.h"
#include "Factories/Factory.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "ablAbilityBlueprintFactory.generated.h"

/* Factory for Ability Blueprints. */
UCLASS(HideCategories = Object, MinimalAPI)
class UAblAbilityBlueprintFactory : public UFactory
{
	GENERATED_BODY()
public:
	UAblAbilityBlueprintFactory(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblAbilityBlueprintFactory();

	// The type of blueprint that will be created
	UPROPERTY(EditAnywhere, Category = AblAbilityBlueprintFactory)
	TEnumAsByte<EBlueprintType> BlueprintType;

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category = AblAbilityBlueprintFactory)
	TSubclassOf<UAblAbility> ParentClass;

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface	
};
