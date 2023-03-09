// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Animation/AnimNode_AbilityAnimPlayer.h"

#include "AnimGraphNode_AssetPlayerBase.h"

#include "AnimGraphNode_AbilityAnimPlayer.generated.h"

/* Ability Animation Player Node is a custom animation node that allows the blending of (up to) 2 animations at a time.
 * The Play Animation Task will directly feed in the Animations to play. */
UCLASS(MinimalAPI)
class UAnimGraphNode_AbilityAnimPlayer : public UAnimGraphNode_AssetPlayerBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_AbilityAnimPlayer Node;

	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) override;
	virtual bool DoesSupportTimeForTransitionGetter() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	virtual void OnProcessDuringCompilation(IAnimBlueprintCompilationContext& InCompilationContext, IAnimBlueprintGeneratedClassCompiledData& OutCompiledData) override;
	// End of UAnimGraphNode_Base interface

};