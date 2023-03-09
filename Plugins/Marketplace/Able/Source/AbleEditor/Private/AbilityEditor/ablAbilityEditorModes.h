// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Editor/Kismet/Public/BlueprintEditorModes.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditorModes"

// This is the list of IDs for Ability Editor modes
struct FAblAbilityEditorModes
{
	// App Name
	static const FName AblAbilityEditorName;

	// Mode constants
	static const FName AbilityBlueprintMode;
	static const FName AbilityTimelineMode;
	static FText GetLocalizedMode(const FName InMode)
	{
		static TMap< FName, FText > LocModes;

		if (LocModes.Num() == 0)
		{
			LocModes.Add(AbilityBlueprintMode, NSLOCTEXT("AblAbilityEditorModes", "AbilityBlueprintMode", "Graph"));
			LocModes.Add(AbilityTimelineMode, NSLOCTEXT("AblAbilityEditorModes", "AbilityTimelineMode", "Timeline"));
		}

		check(InMode != NAME_None);
		const FText* OutDesc = LocModes.Find(InMode);
		check(OutDesc);
		return *OutDesc;
	}
private:
	FAblAbilityEditorModes() {}
};


struct FAblAbilityTimelineSummoner : public FWorkflowTabFactory
{
public:
	FAblAbilityTimelineSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	// FWorkflowTabFactory interface
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	
	static const FName ID;
};

class FAppAbilityEditorTimelineMode : public FApplicationMode
{
public:
	FAppAbilityEditorTimelineMode(TSharedPtr<class FAblAbilityEditor> AbilityEditor);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	virtual void PostActivateMode() override;
	// End of FApplicationMode interface

protected:
	TWeakPtr<class FAblAbilityEditor> m_AbilityEditor;

	FWorkflowAllowedTabSet m_AbilityEditorTabFactories;
};


class FAppAbilityEditorBlueprintMode : public FBlueprintEditorApplicationMode
{
public:
	FAppAbilityEditorBlueprintMode(TSharedPtr<class FAblAbilityEditor> AbilityEditor);
	~FAppAbilityEditorBlueprintMode();

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	virtual void PostActivateMode() override;
	// End of FApplicationMode interface
private:
	TWeakPtr<class FAblAbilityEditor> m_AbilityEditor;

	FWorkflowAllowedTabSet m_AbilityEditorTabFactories;

};

#undef LOCTEXT_NAMESPACE