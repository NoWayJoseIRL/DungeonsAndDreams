// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "Engine/Blueprint.h"
#include "Framework/Commands/UICommandList.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "AssetTools/Public/AssetTypeCategories.h"

class UAblAbility;
class FAblAbilityEditor;
class IAssetTools;
class IAssetTypeActions;
class FAblPlayAnimationAddedHandler;
class FAbleEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	uint32 GetAbleAssetCategory() const { return m_AbleAssetCategory; }

	TSharedRef<FAblAbilityEditor> CreateAbilityEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UBlueprint* Blueprint, bool bShouldOpenInDefaultsMode = false);
	TSharedRef<FAblAbilityEditor> CreateAbilityEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< UBlueprint* >& BlueprintsToEdit, bool bShouldOpenInDefaultsMode = true);

	/** Get all ability editor instances */
	TArray<TSharedRef<FAblAbilityEditor>> GetAbilityEditors() const;

private:
	void RegisterAssetTypes(IAssetTools& AssetTools);
	void RegisterSettings();
	void UnregisterSettings();

	EAssetTypeCategories::Type m_AbleAssetCategory;

	TArray<TSharedPtr<IAssetTypeActions>> m_CreatedAssetTypeActions;

	TSharedPtr<FAblPlayAnimationAddedHandler> m_PlayAnimationTaskHandler;

	/** List of all blueprint editors that were created. */
	TArray<TWeakPtr<FAblAbilityEditor>> m_AbilityEditors;
};
