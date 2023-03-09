// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditorAddTaskHandlers.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbilityEditor/AblAbilityTaskDetails.h"
#include "AbilityEditor/AblAbilityThumbnailRenderer.h"
#include "AbilityEditor/AssetTypeActions_ablAbilityBlueprint.h"
#include "ablAbility.h"
#include "ablAbilityBlueprint.h"
#include "AbleEditorEventManager.h"
#include "AbleStyle.h"
#include "ablSettings.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Editor/UnrealEd/Classes/ThumbnailRendering/ThumbnailManager.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Misc/SlowTask.h"
#include "Styling/AppStyle.h"
#include "Tasks/ablCustomTask.h"
#include "Editor.h"
#include "UObject/UObjectGlobals.h"

#include "IAbleEditor.h"

DEFINE_LOG_CATEGORY(LogAbleEditor);

#define LOCTEXT_NAMESPACE "AbleEditor"



void FAbleEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	m_AbleAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Able")), LOCTEXT("AbleAssetCategory", "Able"));

	FAbleStyle::Initialize();

	RegisterAssetTypes(AssetTools);
	RegisterSettings();

	UThumbnailManager::Get().RegisterCustomRenderer(UAblAbility::StaticClass(), UAblAbilityThumbnailRenderer::StaticClass());
	UThumbnailManager::Get().RegisterCustomRenderer(UAblAbilityBlueprint::StaticClass(), UAblAbilityThumbnailRenderer::StaticClass());

	m_PlayAnimationTaskHandler = MakeShareable(new FAblPlayAnimationAddedHandler());
	m_PlayAnimationTaskHandler->Register();
}


void FAbleEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UAblAbilityBlueprint::StaticClass());
		UThumbnailManager::Get().UnregisterCustomRenderer(UAblAbility::StaticClass());
	}


	FAbleStyle::Shutdown();

	UnregisterSettings();

	// Unregister any customized layout objects.
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("AblAbilityTask");
	}

	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (TSharedPtr<IAssetTypeActions>& Action : m_CreatedAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
		}
	}
	m_CreatedAssetTypeActions.Empty();
}

void FAbleEditorModule::RegisterAssetTypes(IAssetTools& AssetTools)
{
	// Register any asset types
	
	// Ability Blueprint
	TSharedRef<IAssetTypeActions> AbilityBlueprint = MakeShareable(new FAssetTypeActions_AblAbilityBlueprint(m_AbleAssetCategory));
	AssetTools.RegisterAssetTypeActions(AbilityBlueprint);
	m_CreatedAssetTypeActions.Add(AbilityBlueprint);
}

void FAbleEditorModule::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Able",
			LOCTEXT("RuntimeSettingsName", "Able"),
			LOCTEXT("RuntimeSettingsDescription", "Configure the Able plugin"),
			GetMutableDefault<UAbleSettings>());

		SettingsModule->RegisterSettings("Editor", "ContentEditors", "AbilityEditor",
			LOCTEXT("AbilityEditorSettingsName", "Ability Editor"),
			LOCTEXT("AbilityEditorSettingsDescription", "Configure the Ability Editor"),
			GetMutableDefault<UAblAbilityEditorSettings>());
	}
}

void FAbleEditorModule::UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "ContentEditors", "AbilityEditor");
		SettingsModule->UnregisterSettings("Project", "Plugins", "Able");
	}
}

TSharedRef<FAblAbilityEditor> FAbleEditorModule::CreateAbilityEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UBlueprint* Blueprint, bool bShouldOpenInDefaultsMode)
{
	TArray<UBlueprint*> BlueprintsToEdit = { Blueprint };
	return CreateAbilityEditor(Mode, InitToolkitHost, BlueprintsToEdit, bShouldOpenInDefaultsMode);
}

TSharedRef<FAblAbilityEditor> FAbleEditorModule::CreateAbilityEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< UBlueprint* >& BlueprintsToEdit, bool bShouldOpenInDefaultsMode)
{
	TSharedRef< FAblAbilityEditor > NewAbilityEditor(new FAblAbilityEditor());

	NewAbilityEditor->InitAbilityEditor(Mode, InitToolkitHost, BlueprintsToEdit, bShouldOpenInDefaultsMode);

	m_AbilityEditors.Add(NewAbilityEditor);

	return NewAbilityEditor;
}

IMPLEMENT_MODULE(FAbleEditorModule, AbleEditor)