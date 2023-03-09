// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/AnimGraphNode_AbilityAnimPlayer.h"

#include "AbleEditorPrivate.h"

#include "Animation/AnimLayerInterface.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/CompilerResultsLog.h"
#include "EdGraphSchema_K2_Actions.h"
#include "GraphEditorActions.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AnimationGraphSchema.h"
#include "EditorCategoryUtils.h"
#include "IAnimBlueprintCompilerHandlerCollection.h"
#include "IAnimBlueprintGeneratedClassCompiledData.h"
#include "IAnimBlueprintCompilationContext.h"

#define LOCTEXT_NAMESPACE "AbilityAnimNode"

// Action to add a ability animation player node to the graph
struct FNewAbilityAnimPlayerAction : public FEdGraphSchemaAction_K2NewNode
{
protected:
	FAssetData AssetInfo;
public:
	FNewAbilityAnimPlayerAction(const FAssetData& InAssetInfo, FText Title)
		: FEdGraphSchemaAction_K2NewNode(LOCTEXT("AbleCategory", "Able"), Title, FText::FromString(TEXT("Evaluates a sequence set by the Play Animation task to produce a pose.")), 0, FText::FromString(InAssetInfo.GetObjectPathString()))
	{
		AssetInfo = InAssetInfo;

		UAnimGraphNode_AbilityAnimPlayer* Template = NewObject<UAnimGraphNode_AbilityAnimPlayer>();
		NodeTemplate = Template;
	}

	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override
	{
		UAnimGraphNode_AbilityAnimPlayer* SpawnedNode = CastChecked<UAnimGraphNode_AbilityAnimPlayer>(FEdGraphSchemaAction_K2NewNode::PerformAction(ParentGraph, FromPin, Location, bSelectNewNode));

		return SpawnedNode;
	}
};


UAnimGraphNode_AbilityAnimPlayer::UAnimGraphNode_AbilityAnimPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FLinearColor UAnimGraphNode_AbilityAnimPlayer::GetNodeTitleColor() const
{
	return FLinearColor::Yellow;
}

FText UAnimGraphNode_AbilityAnimPlayer::GetTooltipText() const
{
	return LOCTEXT("AbilityAnimPlayerTooltip", "Allows for the dynamic playing of animations using the Play Animation task.");
}

FText UAnimGraphNode_AbilityAnimPlayer::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText Title = LOCTEXT("AbilityAnimPlayerTitle", "Play Ability Animation");
	return Title;
}

FText UAnimGraphNode_AbilityAnimPlayer::GetMenuCategory() const
{
	// We don't actually populate a menu context, but if we need to, change this in the future.
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Animation);
}

void UAnimGraphNode_AbilityAnimPlayer::BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	AnimBlueprint->FindOrAddGroup(Node.GetGroupName());
}

bool UAnimGraphNode_AbilityAnimPlayer::DoesSupportTimeForTransitionGetter() const
{
	return false;
}

void UAnimGraphNode_AbilityAnimPlayer::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	ActionRegistrar.AddBlueprintAction(UBlueprintNodeSpawner::Create(GetClass()));
}

bool UAnimGraphNode_AbilityAnimPlayer::IsActionFilteredOut(FBlueprintActionFilter const& Filter)
{
	for (UBlueprint* Blueprint : Filter.Context.Blueprints)
	{
		UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(Blueprint);
		if (!AnimBlueprint)
		{
			// We only work on Animation Blueprints.
			return true;
		}
	}

	return false;
}

void UAnimGraphNode_AbilityAnimPlayer::OnProcessDuringCompilation(IAnimBlueprintCompilationContext& InCompilationContext, IAnimBlueprintGeneratedClassCompiledData& OutCompiledData)
{
	UBlueprint* Blueprint = GetBlueprint();

	// Process Asset Player nodes to, if necessary cache off their node index for retrieval at runtime (used for evaluating Automatic Rule Transitions when using Layer nodes)
	auto ProcessGraph = [this, &OutCompiledData](UEdGraph* Graph)
	{
		// Make sure we do not process the default AnimGraph
		static const FName DefaultAnimGraphName("AnimGraph");
		if (Graph->GetFName() != DefaultAnimGraphName)
		{
			FString GraphName = Graph->GetName();
			// Also make sure we do not process any empty stub graphs
			if (!GraphName.Contains(ANIM_FUNC_DECORATOR))
			{
				if (Graph->Nodes.ContainsByPredicate([this, &OutCompiledData](UEdGraphNode* LambdaNode) { return LambdaNode->NodeGuid == NodeGuid; }))
				{
					if (int32* IndexPtr = OutCompiledData.GetAnimBlueprintDebugData().NodeGuidToIndexMap.Find(NodeGuid))
					{
						FGraphAssetPlayerInformation& Info = OutCompiledData.GetGraphAssetPlayerInformation().FindOrAdd(FName(*GraphName));
						Info.PlayerNodeIndices.AddUnique(*IndexPtr);
					}
				}
			}
		}
	};

	// Check for any definition of a layer graph
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		ProcessGraph(Graph);
	}

	// Check for any implemented AnimLayer interface graphs
	for (FBPInterfaceDescription& InterfaceDesc : Blueprint->ImplementedInterfaces)
	{
		// Only process Anim Layer interfaces
		if (InterfaceDesc.Interface->IsChildOf<UAnimLayerInterface>())
		{
			for (UEdGraph* Graph : InterfaceDesc.Graphs)
			{
				ProcessGraph(Graph);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE