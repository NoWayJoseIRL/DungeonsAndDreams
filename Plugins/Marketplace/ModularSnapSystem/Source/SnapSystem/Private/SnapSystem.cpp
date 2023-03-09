// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#include "SnapSystem.h"
#include "SnapSystemStyle.h"
#include "SnapSystemCommands.h"
#include "MSSModeEdMode.h"
#include "SMSSViewportToolBarComboMenu.h"


#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Input/SNumericEntryBox.h"


//#include "EditorStyle.h"


#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "Editor/GroupActor.h"
#include "EditorModeTools.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMeshSocket.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Selection.h"

#include "USnapSystemSettings.h"
#include "ISettingsModule.h"
#include "Misc/ConfigCacheIni.h"

#include "Internationalization/Regex.h"
#include "Internationalization/Text.h"

#include "Components/SplineComponent.h"
#include "ComponentVisualizer.h"

#include "Misc/ConfigCacheIni.h"

#include "EditorModeRegistry.h"

#include  "DrawDebugHelpers.h"

#include "Runtime/Launch/Resources/Version.h"

#include "CopyPasteSockets.h"
#include "ExportImportSockets.h"

#include "ContentBrowserModule.h"
#include "Components/InstancedStaticMeshComponent.h"


static const FName SnapSystemTabName("SnapSystem");

const FName FSnapSystemModule::NAME_NoSnap(TEXT("nosnap"));

#define LOCTEXT_NAMESPACE "FSnapSystemModule"



void FSnapSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FSnapSystemStyle::Initialize();
	FSnapSystemStyle::ReloadTextures();

	FSnapSystemCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSnapSystemCommands::Get().SnapSystemEnable,
		FExecuteAction::CreateRaw(this, &FSnapSystemModule::PluginButtonClicked),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FSnapSystemModule::IsSnapEnabled)
	);

	PluginCommands->MapAction(
		FSnapSystemCommands::Get().ShowSockets,
		FExecuteAction::CreateRaw(this, &FSnapSystemModule::PluginShowSockets),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FSnapSystemModule::IsSocketsShown)
	);

	PluginCommands->MapAction(
		FSnapSystemCommands::Get().SnapSystemOptions,
		FExecuteAction::CreateRaw(this, &FSnapSystemModule::OptionsButtonClicked),
		FCanExecuteAction()

	);


	PluginCommands->MapAction(
		FSnapSystemCommands::Get().CopySockets,
		FExecuteAction::CreateStatic(&UCopyPasteSockets::CopySockets),
		FCanExecuteAction()

	);

	PluginCommands->MapAction(
		FSnapSystemCommands::Get().PasteSockets,
		FExecuteAction::CreateStatic(&UCopyPasteSockets::PasteSockets),
		FCanExecuteAction()

	);







	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());


	{
		FLevelEditorModule::FLevelEditorMenuExtender ViewMenuExtender;
		ViewMenuExtender = FLevelEditorModule::FLevelEditorMenuExtender::CreateRaw(this, &FSnapSystemModule::OnExtendLevelEditorViewMenu);
		auto& MenuExtenders = LevelEditorModule.GetAllLevelEditorToolbarViewMenuExtenders();
		MenuExtenders.Add(ViewMenuExtender);
	}



	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
#if ENGINE_MAJOR_VERSION == 5 
		ToolbarExtender->AddToolBarExtension("LocationGridSnap", EExtensionHook::Before, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FSnapSystemModule::AddToolbarExtension));
#else
		ToolbarExtender->AddToolBarExtension("LocalToWorld", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FSnapSystemModule::AddToolbarExtension));
#endif
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	}


	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "SnapSystem",
			LOCTEXT("SnapSystemSettingsName", "Snap System"),
			LOCTEXT("SnapSystemSettingsDescription", "Configure the Snap System Plugin here. Keyboard shortcuts are configured under \"Editor Preferences->Keyboard Shortcuts->SnapSystem Plugin\""),
			GetMutableDefault<UUSnapSystemSettings>());
	}

	//{
	//	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	//	ContentBrowserModule.GetAllAssetContextMenuExtenders().Add(FContentBrowserMenuExtender_SelectedPaths::CreateLambda([this](const TArray<FString>& SelectedPaths)
	//		{
	//			TSharedRef<FExtender> Extender(new FExtender());

	//			Extender->AddMenuExtension(
	//				"ImportedAssetActions",
	//				EExtensionHook::Before,
	//				TSharedPtr<FUICommandList>(),
	//				FMenuExtensionDelegate::CreateRaw(this, &FSnapSystemModule::ExportMenuBuilderFunc, SelectedPaths)
	//			);

	//			UE_LOG(LogTemp, Warning, TEXT("Extender->AddMenuExtension"));
	//			return Extender;
	//		}));
	//}
	
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray< FContentBrowserMenuExtender_SelectedAssets >& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

		CBMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda([this](const TArray<FAssetData>& SelectedAssets)
			{
				TSharedRef<FExtender> Extender(new FExtender());
				
				bool bShouldExtendAssetActions = true;
				for (const FAssetData& Asset : SelectedAssets)
				{

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
					if (Asset.AssetClassPath.GetAssetName() != UStaticMesh::StaticClass()->GetFName())
#else
					if (Asset.AssetClass != UStaticMesh::StaticClass()->GetFName())
#endif
					{
						bShouldExtendAssetActions = false;
						break;
					}
				}

				if (bShouldExtendAssetActions) 
				{
					Extender->AddMenuExtension(
						"GetAssetActions",
						EExtensionHook::After,
						TSharedPtr<FUICommandList>(PluginCommands),
						FMenuExtensionDelegate::CreateLambda(
							[SelectedAssets](FMenuBuilder& MenuBuilder)
							{
								MenuBuilder.AddSubMenu(
									FText::FromString("Snap System"),
									FText::FromString("Snap System"),
									FNewMenuDelegate::CreateLambda([SelectedAssets](FMenuBuilder& InMenuBuilder)
										{
											InMenuBuilder.AddMenuEntry(
												FSnapSystemCommands::Get().ExportSockets->GetLabel(),
												FSnapSystemCommands::Get().ExportSockets->GetDescription(),
												FSlateIcon(),
												FUIAction(FExecuteAction::CreateStatic(&UExportImportSockets::ExportSockets, SelectedAssets), FCanExecuteAction())
											);
											InMenuBuilder.AddMenuEntry(
												FSnapSystemCommands::Get().ImportSockets->GetLabel(),
												FSnapSystemCommands::Get().ImportSockets->GetDescription(),
												FSlateIcon(),
												FUIAction(FExecuteAction::CreateStatic(&UExportImportSockets::ImportSockets, SelectedAssets), FCanExecuteAction())
											);
											InMenuBuilder.AddMenuEntry(
												FSnapSystemCommands::Get().DeleteSockets->GetLabel(),
												FSnapSystemCommands::Get().DeleteSockets->GetDescription(),
												FSlateIcon(),
												FUIAction(FExecuteAction::CreateStatic(&UExportImportSockets::DeleteSockets, SelectedAssets), FCanExecuteAction())
											);


										}),
									false, 
									FSlateIcon(FSnapSystemStyle::GetStyleSetName(),"SnapSystem.SnapSystemIcon"));
							})
					);
				}

				return Extender;
			}
		));
		ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();

	}


	GEditor->OnBeginObjectMovement().AddRaw(this, &FSnapSystemModule::PluginMoveStarted);
	GEditor->OnActorMoved().AddRaw(this, &FSnapSystemModule::PluginActorMoved);

	GEditor->OnEndObjectMovement().AddRaw(this, &FSnapSystemModule::PluginObjectMoved);
	GEditor->OnLevelActorAdded().AddRaw(this, &FSnapSystemModule::PluginActorAdded);


	bMoveStarted = false;
	bActorAdded = false;


	FEditorModeRegistry::Get().RegisterMode<FMSSModeEdMode>(FMSSModeEdMode::EM_MSSModeEdModeId, LOCTEXT("MSSModeEdModeName", "Modular Snap System"), FSlateIcon(), false);

	USelection::SelectionChangedEvent.AddRaw(this, &FSnapSystemModule::PluginSelectionChanged);

}


TSharedRef<FExtender> FSnapSystemModule::OnExtendLevelEditorViewMenu(const TSharedRef<FUICommandList> CommandList)
{
	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension("Snapping", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FSnapSystemModule::AddMenuExtension));
	return Extender;
}

void FSnapSystemModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FSnapSystemCommands::Get().SnapSystemOptions);
}

void FSnapSystemModule::AddToolbarExtension(FToolBarBuilder& ToolbarBuilder)
{
	
	ToolbarBuilder.BeginSection("SnapSystemPlugin");
	//ToolbarBuilder.BeginBlockGroup();

	//ToolbarBuilder.AddToolBarButton(FSnapSystemCommands::Get().SnapSystemEnable);
#if ENGINE_MAJOR_VERSION == 5 
	FName ToolBarStyle = "EditorViewportToolBar";
#else
	FName ToolBarStyle = "ViewportMenu";
#endif

	ToolbarBuilder.AddWidget(SNew(SMSSViewportToolBarComboMenu)
		.Cursor(EMouseCursor::Default)
		.Style(ToolBarStyle)
		.IsChecked_Raw(this, &FSnapSystemModule::IsSnapChecked)
		.OnCheckStateChanged_Raw(this, &FSnapSystemModule::HandleToggleSnap)
		.Label_Raw(this, &FSnapSystemModule::GetSearchDistLabel)
		.OnGetMenuContent_Raw(this, &FSnapSystemModule::FillSnapMenu)
		.ToggleButtonToolTip(FSnapSystemCommands::Get().SnapSystemEnable->GetDescription())
		.MenuButtonToolTip(LOCTEXT("SnapPluginOptions", "Snap Plugin Options"))
		.Icon(FSnapSystemCommands::Get().SnapSystemEnable->GetIcon())
		
		, FName(TEXT("SnapSystem")));

	//ToolbarBuilder.EndBlockGroup();
	ToolbarBuilder.EndSection();

}
ECheckBoxState FSnapSystemModule::IsSnapChecked() const
{
	return GetDefault<UUSnapSystemSettings>()->bIsEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
void FSnapSystemModule::HandleToggleSnap(ECheckBoxState InState)
{
	PluginButtonClicked();
}
FText FSnapSystemModule::GetSearchDistLabel() const
{

	const float SearchDist = GetDefault<UUSnapSystemSettings>()->SearchDist;
	FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.MaximumFractionalDigits = 0;

	return  FText::AsNumber(SearchDist, &NumberFormattingOptions);
}

TSharedRef<SWidget> FSnapSystemModule::FillSnapMenu()
{
	const UUSnapSystemSettings* Settings = GetDefault<UUSnapSystemSettings>();
	const bool bShouldCloseWindowAfterMenuSelection = true;

	FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.MaximumFractionalDigits = 0;

	FMenuBuilder SnapMenuBuilder(bShouldCloseWindowAfterMenuSelection, PluginCommands);

	SnapMenuBuilder.BeginSection("SnapPluginOptions", LOCTEXT("SnapPluginOptions", "Snap Plugin Options"));



	SnapMenuBuilder.AddWidget(
		SNew(SVerticalBox)
		.IsEnabled_Raw(this, &FSnapSystemModule::IsSnapEnabled)



		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 2.0f, 60.0f, 2.0f))
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SnapPlugin_SnapDist", "Snap Search Distance"))
		]
		+ SVerticalBox::Slot()
		.Padding(FMargin(8.0f, 4.0f))
		.AutoHeight()

		[
			SNew(SNumericEntryBox<float>)
			.ToolTipText(LOCTEXT("SnapPlugin_SnapDistTooltip", "How far to look for a matching socket"))
			.Value(
				TAttribute<TOptional<float>>::Create(TAttribute<TOptional<float>>::FGetter::CreateStatic([] {
					const auto* Settings = GetDefault<UUSnapSystemSettings>();
					return TOptional<float>(Settings->SearchDist);
			}))
			)
			.OnValueChanged(
				SNumericEntryBox<float>::FOnValueChanged::CreateStatic([](float Val) {
					auto* Settings = GetMutableDefault<UUSnapSystemSettings>();
					Settings->SearchDist = Val;

				})
			)
			.OnValueCommitted(
				SNumericEntryBox<float>::FOnValueCommitted::CreateStatic([](float Val, ETextCommit::Type Type) {
					auto* Settings = GetMutableDefault<UUSnapSystemSettings>();
					Settings->SearchDist = Val;
					Settings->SaveConfig();

				})
			)
			.MinValue(1.f)
			.MaxValue(HALF_WORLD_MAX)
			.MinSliderValue(50.f) 
			.MaxSliderValue(5000.f) 
			.SliderExponent(5.f)
			.SliderExponentNeutralValue(50.f)
			.Delta(5.f)
			.AllowSpin(true)
		],
		FText::GetEmpty()
	);



	SnapMenuBuilder.AddSubMenu(
		FSnapSystemCommands::Get().ShowSockets->GetLabel(),
		FSnapSystemCommands::Get().ShowSockets->GetDescription(),
		FNewMenuDelegate::CreateLambda([](FMenuBuilder& InMenuBuilder)
		{

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
			const UEnum* EnumPtr = FindObject<UEnum>(nullptr, TEXT("/Script/SnapSystem.EShowSocketMode"), true); 
#else
			const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EShowSocketMode"), true); 
#endif
			if (EnumPtr) {
				for (int32 i = 0; i < EShowSocketMode::MAX; i++)
				{
					EnumPtr->GetDisplayNameTextByIndex(i); 

					InMenuBuilder.AddMenuEntry(
						EnumPtr->GetDisplayNameTextByIndex(i), 
						EnumPtr->GetToolTipTextByIndex(i),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda([i]() {
								auto* Settings = GetMutableDefault<UUSnapSystemSettings>();
								Settings->ShowSocketMode = static_cast<EShowSocketMode::Type>(i);
								Settings->SaveConfig();
								
							}),
							FCanExecuteAction(),
							FIsActionChecked::CreateLambda([i]() {
								const auto* Settings = GetDefault<UUSnapSystemSettings>();
								return i == Settings->ShowSocketMode;
							})
						),
						NAME_None,
						EUserInterfaceActionType::RadioButton);
				}
			}
	
		}),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FSnapSystemModule::PluginShowSockets),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FSnapSystemModule::IsSocketsShown)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	
	SnapMenuBuilder.AddMenuEntry(
		LOCTEXT("SnapPlugin_SnapOpenOnly", "Snap Open Only"),
		LOCTEXT("SnapPlugin_SnapOpenOnlyTooltip", "Check if a socket is \"open\" before considering it for snapping\nOpen socket = not blocked by some other component\nOnly components in TargetComponentClasses list are considered as blockers\nCheck support thread on UE forum for a tutorial"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([]() {
				auto* Settings = GetMutableDefault<UUSnapSystemSettings>();
				Settings->bSnapOpenOnly = !Settings->bSnapOpenOnly;
				Settings->SaveConfig();
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([]() {
				const auto* Settings = GetDefault<UUSnapSystemSettings>();
				return Settings->bSnapOpenOnly;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	SnapMenuBuilder.AddMenuSeparator();
	SnapMenuBuilder.AddMenuEntry(
		FSnapSystemCommands::Get().CopySockets,
		NAME_None);

	SnapMenuBuilder.AddMenuEntry(
		FSnapSystemCommands::Get().PasteSockets,
		NAME_None);


	SnapMenuBuilder.AddMenuSeparator();
	SnapMenuBuilder.AddMenuEntry(
		FSnapSystemCommands::Get().SnapSystemOptions,
		NAME_None, 
		LOCTEXT("SnapPlugin_Options", "Advanced Settings..."), 
		LOCTEXT("SnapPlugin_OptionsTooltip", "Access to all settings"),
		FSlateIcon());
	
	SnapMenuBuilder.AddMenuEntry(
		LOCTEXT("SnapPlugin_Help", "Online help and news..."), 
		LOCTEXT("SnapPlugin_HelpTooltip", "Link to the UE4 forum page of the plugin"), 
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([]() {
				const FString DestinationURL = "https://forums.unrealengine.com/unreal-engine/marketplace/1482720-mss-modular-snap-system-plugin";
				FPlatformProcess::LaunchURL(*DestinationURL, NULL, NULL);
				
			})
		)
	);
	SnapMenuBuilder.EndSection();


	return SnapMenuBuilder.MakeWidget();
}



void FSnapSystemModule::OptionsButtonClicked()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", "SnapSystem");

}


void FSnapSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (ContentBrowserExtenderDelegateHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		CBMenuExtenderDelegates.RemoveAll(
			[this](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
			{
				return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle;
			}
		);
	}

	FSnapSystemStyle::Shutdown();

	FSnapSystemCommands::Unregister();

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "SnapSystem");
	}



	FEditorModeRegistry::Get().UnregisterMode(FMSSModeEdMode::EM_MSSModeEdModeId);

}




void FSnapSystemModule::PluginButtonClicked()
{

	auto* Settings = GetMutableDefault<UUSnapSystemSettings>();
	Settings->bIsEnabled = !Settings->bIsEnabled;

	Settings->SaveConfig();

	if (Settings->bIsEnabled)
	{
		GLevelEditorModeTools().ActivateMode(FMSSModeEdMode::EM_MSSModeEdModeId);
	}
	else
	{
		GLevelEditorModeTools().DeactivateMode(FMSSModeEdMode::EM_MSSModeEdModeId);
	}
}



bool FSnapSystemModule::IsSnapEnabled() const
{
	return GetDefault<UUSnapSystemSettings>()->bIsEnabled;
}


void FSnapSystemModule::PluginShowSockets()
{

	auto* Settings = GetMutableDefault<UUSnapSystemSettings>();
	Settings->bShowSockets = !Settings->bShowSockets;

	Settings->SaveConfig();

	if(GEditor)GEditor->RedrawLevelEditingViewports(false);

}

bool FSnapSystemModule::IsSocketsShown() const
{
	return GetDefault<UUSnapSystemSettings>()->bShowSockets;
}

void FSnapSystemModule::PluginMoveStarted(UObject& InObject)
{

	if (!InObject.HasAnyFlags(RF_Transient) && !InObject.GetOutermost()->HasAnyFlags(RF_Transient))
	{
		//UE_LOG(LogTemp, Warning, TEXT("PluginMoveStarted %s (%s)"), *InObject.GetName(), (InObject.GetFlags()&RF_Transient ? TEXT("TRANSIENT") : TEXT("Normal")));


		if (!bMoveStarted)
		{
			if (AActor* A = Cast<AActor>(&InObject))
			{
				SavedT = A->GetActorTransform();
			}
			else if (USceneComponent* C = Cast<USceneComponent>(&InObject))
			{
				SavedT = C->GetComponentTransform();
			}
		}
		bMoveStarted = true;
	}

}




void FSnapSystemModule::PluginObjectMoved(UObject& InObject)
{

	if (!InObject.HasAnyFlags(RF_Transient) && !InObject.GetOutermost()->HasAnyFlags(RF_Transient))
	{
		//UE_LOG(LogTemp, Log, TEXT("PluginObjectMoved (%s)"), (InObject.GetFlags()&RF_Transient ? TEXT("TRANSIENT") : TEXT("Normal")));

		if (bMoveStarted && GEditor->GetSelectedActorCount() > 0 && IsSnapEnabled())
		{
			if (!SkipSnap())
			{

				if (AActor* A = Cast<AActor>(&InObject))
				{
					RelativeT = A->GetActorTransform();
				}
				else if (USceneComponent* C = Cast<USceneComponent>(&InObject))
				{
					RelativeT = C->GetComponentTransform();
				}

				bool bSplinePointEdited = false;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
				if (FMSSModeEdMode* MssMode = static_cast<FMSSModeEdMode*>(GLevelEditorModeTools().FindMode(FMSSModeEdMode::EM_MSSModeEdModeId)))
#else
				if (FMSSModeEdMode * MssMode = static_cast<FMSSModeEdMode*>(GLevelEditorModeTools().GetActiveMode(FMSSModeEdMode::EM_MSSModeEdModeId)))
#endif
				{

					bSplinePointEdited = MssMode->IsSplineEdited();
				}

				if (bSplinePointEdited)
				{
					//handled by editor mode	
				}
				else if (GEditor->GetSelectedComponentCount() == 0)
				{
					SnapSel();
				}
				else
				{
					SnapSelComp();
				}
			}

			bMoveStarted = false;
			bActorAdded = false;
		}
	}


}

void FSnapSystemModule::PluginActorAdded(AActor * InObject)
{
	//UE_LOG(LogTemp, Warning, TEXT("PluginActorAdded %s(%s) in world %s(%s)"), *InObject->GetName(), InObject->GetFlags()&RF_Transient ? TEXT("TRANSIENT") : TEXT("Normal"), *InObject->GetOutermost()->GetName(), InObject->GetOutermost()->GetFlags()&RF_Transient? TEXT("TRANSIENT") : TEXT("Normal"));

	if (InObject && !InObject->HasAnyFlags(RF_Transient) && !InObject->GetOutermost()->HasAnyFlags(RF_Transient))
	{
		//UE_LOG(LogTemp, Warning, TEXT("PluginActorAdded %s(%s) in world %s(%s)"), *InObject->GetName(), InObject->GetFlags()&RF_Transient ? TEXT("TRANS") : TEXT(""), *InObject->GetOutermost()->GetName(), InObject->GetOutermost()->GetFlags()&RF_Transient ? TEXT("TRANS") : TEXT(""));
		bActorAdded = true;
	}
}

void FSnapSystemModule::PluginActorMoved(AActor * InObject)
{
	//UE_LOG(LogTemp, Warning, TEXT("PluginActorMoved %s (%s)"), *InObject->GetName(), (InObject->GetFlags()&RF_Transient ? TEXT("TRANSIENT") : TEXT("Normal")));

	if (InObject && !InObject->HasAnyFlags(RF_Transient) && !InObject->GetOutermost()->HasAnyFlags(RF_Transient))
	{ 
		//UE_LOG(LogTemp, Warning, TEXT("PluginActorMoved %s(%s) in world %s(%s)"), *InObject->GetName(), InObject->GetFlags()&RF_Transient ? TEXT("TRANS") : TEXT(""), *InObject->GetOutermost()->GetName(), InObject->GetOutermost()->GetFlags()&RF_Transient ? TEXT("TRANS") : TEXT(""));

		if (GEditor->GetSelectedActorCount()>0 && IsSnapEnabled() && !bMoveStarted && bActorAdded && InObject->IsSelected())

		{
			if (!SkipSnap())
			{
				SnapSel();
			}
			bActorAdded = false;
		}
	}
}

void FSnapSystemModule::PluginSelectionChanged(UObject* InObject)
{

	bool bModeActive = GLevelEditorModeTools().IsModeActive(FMSSModeEdMode::EM_MSSModeEdModeId);
	bool bPluginEnabled = GetDefault<UUSnapSystemSettings>()->bIsEnabled;

	if (bPluginEnabled && !bModeActive)
	{
		GLevelEditorModeTools().ActivateMode(FMSSModeEdMode::EM_MSSModeEdModeId);

	}
	else if(!bPluginEnabled && bModeActive)
	{
		GLevelEditorModeTools().DeactivateMode(FMSSModeEdMode::EM_MSSModeEdModeId);

	}

	//selection related stuff like control click or shift click are considered "move starters", 
	//this will make difference between that and real move 
	bMoveStarted = false;

	//cache is safe until selection change
	SocketCache.Reset();
	SplineSocketCache.Reset();



}


void FSnapSystemModule::SnapSel()
{
	TSet<const AActor*> Attached;
	FSnapSocket MyS, OtherS;

	if (SearchSel(MyS, OtherS, Attached))
	{
		if (UPrimitiveComponent* MySComp = MyS.Comp.Get())
		{
			if (UPrimitiveComponent* OtherSComp = OtherS.Comp.Get())
			{
				FTransform T = (FTransform(FRotator(0, 180.0f, 0)) * OtherS.T).GetRelativeTransform(MyS.T);

				FSnapParameters Params;
				if (!GetDefault<UUSnapSystemSettings>()->bIgnoreSocketParams)
				{
					MakeParams(Params, MyS, OtherS);
				}

				bool bIgnoreScale = GetDefault<UUSnapSystemSettings>()->bIgnoreSplineSocketScale && (MySComp->IsA(USplineComponent::StaticClass()) || OtherSComp->IsA(USplineComponent::StaticClass()));
				UpdateTransform(Params, T, bIgnoreScale);


				T = T * MyS.T;

				//if (!T.IsRotationNormalized())
				//{
				//	UE_LOG(LogTemp, Warning, TEXT("ROTATION NOT NORMALIZED"));
				//}

				const bool bSnapAway = !GetDefault<UUSnapSystemSettings>()->bNoSnapback || !EqualsTransform(RelativeT.GetRelativeTransform(MyS.T) * T, SavedT);


				if (bSnapAway)
				{
					TArray<AGroupActor*> Groups;


					for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
					{
						AActor* A = Cast<AActor>(*It);

						if (A && !Attached.Contains(A))
						{

							if (GEditor->GetSelectedActorCount() == 1 &&
								Params.bAttach  && OtherSComp->GetOwner())
							{
								AActor* ParentActor = OtherSComp->GetOwner();
								if (ParentActor && ParentActor == A->GetAttachParentActor())
								{
									A->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false));
								}
							}

							//to avoid accumulating errors
							FTransform TT = A->GetActorTransform().GetRelativeTransform(MyS.T) * T;
							TT.SetLocation(TT.GetLocation().GridSnap(0.001f));

							//					A->SetActorTransform(A->GetActorTransform().GetRelativeTransform(MyS.T) * T);
							A->SetActorTransform(TT);


							if (GEditor->GetSelectedActorCount() == 1
								&& Params.bAttach && OtherSComp->GetOwner())
							{
								AActor* ParentActor = OtherSComp->GetOwner();
								if (ParentActor && ParentActor != A->GetAttachParentActor())
								{
									bool bAttachToSocket = !(OtherSComp->IsA(UStaticMeshComponent::StaticClass())) && GetDefault<UUSnapSystemSettings>()->bAttachToSocket;

									A->AttachToComponent(OtherSComp, FAttachmentTransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false), bAttachToSocket ? OtherS.FullName : NAME_None);
								}
							}

							else if (GetDefault<UUSnapSystemSettings>()->bAttachToParent &&  OtherSComp->GetOwner())
							{
								AActor* ParentActor = OtherSComp->GetOwner()->GetAttachParentActor();
								if (ParentActor && ParentActor != A->GetAttachParentActor())
								{
									A->AttachToActor(ParentActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false), NAME_None);
								}
							}

							if (AGroupActor* GroupActor = AGroupActor::GetRootForActor(A))
							{
								if (!GroupActor->IsSelectedInEditor())
								{
									Groups.AddUnique(GroupActor);
								}
							}

						}
					}

					for (AGroupActor* GroupActor : Groups)
					{
						FTransform TT = GroupActor->GetActorTransform().GetRelativeTransform(MyS.T) * T;
						TT.SetLocation(TT.GetLocation().GridSnap(0.001f));
						GroupActor->SetActorTransform(TT);
					}

					if (GetDefault<UUSnapSystemSettings>()->bSnapPivot)
					{

						SetPivot(OtherS.T, true);
					}
					//else if (GEditor->GetSelectedActorCount() > 1 || bActorAdded && !bMoveStarted)
					//{

					//	SetPivot((FTransform(GEditor->GetPivotLocation()).GetRelativeTransform(MyS.T) * T), false);
					//}
					else
					{

						SetPivot((FTransform(GEditor->GetPivotLocation()).GetRelativeTransform(MyS.T) * T), false);
					}
				}
			}
		}

	}

	SplineSocketCache.Reset();
}

bool FSnapSystemModule::SearchSel(FSnapSocket& MyS, FSnapSocket& OtherS, TSet<const AActor*>& Attached)
{
	
	const float SearchDist = GetDefault<UUSnapSystemSettings>()->SearchDist;
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(180.0 - GetDefault<UUSnapSystemSettings>()->MaxAngle));
	const bool SearchAttached = GetDefault<UUSnapSystemSettings>()->bSnapAttachedToo;

	bool bFound = false;
	float FoundDist = SearchDist * 2.0f;



	bool bIgnoreName =  GetDefault<UUSnapSystemSettings>()->bIgnoreSocketName;
	const bool bUseNoSnapTag = GetDefault<UUSnapSystemSettings>()->bUseNoSnapTag;

	Attached.Empty();

	TSet<const AActor*> ActorsToIgnore;
	TArray<UPrimitiveComponent*> ComponentsToIgnore;

	TSet<const AActor*> SnapSourceActors;

	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			ActorsToIgnore.Add(Actor);

			GetAllAttachedActors(Actor, Attached);
			ActorsToIgnore.Append(Attached);

			SnapSourceActors.Add(Actor);

			if(SearchAttached)
			{
				SnapSourceActors.Append(Attached);
			}
		}
	}

	for (auto It = SnapSourceActors.CreateConstIterator(); It; ++It)
	{

		if (const AActor* Mesh = Cast<AActor>(*It))
		{

			TArray<FSnapSocket> Sockets;

			TArray<UActorComponent*> SourceComps;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
			SourceComps = Mesh->GetComponentsByClass(UActorComponent::StaticClass());
#else
			Mesh->GetComponents(SourceComps);
#endif


			for (UActorComponent* ActorComp : SourceComps)
			{
				if (GetDefault<UUSnapSystemSettings>()->SourceComponentClasses.Contains(FSoftClassPath(ActorComp->GetClass())))
				{
					if (!bUseNoSnapTag || !ActorComp->ComponentHasTag(NAME_NoSnap))
					{
						GetAllSockets(Sockets, ActorComp);
					}
				}
			}


			for (FSnapSocket& S : Sockets)
			{

				if (SearchSocket(S, FoundDist, ActorsToIgnore, ComponentsToIgnore, MyS, OtherS))
				{
					bFound = true;
				}

			}			
		}
	}



	return bFound;
}

bool FSnapSystemModule::GetAllSockets(TArray<FSnapSocket>& Sockets, UActorComponent* ActorComp)
{
	bool bFound = false;

	if (USplineComponent* SplineComp = Cast<USplineComponent>(ActorComp))
	{
		FString Name;
		FString Rest;
		if (!SplineComp->GetName().Split("_", &Name, &Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
		{
			Name = SplineComp->GetName();
		}


		Sockets.Emplace(FSnapSocket(SplineComp, Name, Rest, FTransform(FRotator(0, 180.0f, 0)) * SplineComp->GetTransformAtSplinePoint(0, ESplineCoordinateSpace::World, true), FName(*(SplineComp->GetName() + TEXT("_0")))));
		Sockets.Emplace(FSnapSocket(SplineComp, Name, Rest, SplineComp->GetTransformAtSplinePoint(SplineComp->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World, true), FName(*(SplineComp->GetName() + TEXT("_1")))));


		bFound = true;
	}
	else if (UInstancedStaticMeshComponent* ISMComp = Cast<UInstancedStaticMeshComponent>(ActorComp))
	{
		TArray<FComponentSocketDescription> OutSockets;
		ISMComp->QuerySupportedSockets(OutSockets);
		int32 Count = ISMComp->GetInstanceCount();

		if (OutSockets.Num() > 0 && Count > 0)
		{
			TArray<FTransform> IT;
			for (int32 I = 0; I < Count; I++)
			{
				FTransform T;
				ISMComp->GetInstanceTransform(I, T, true);
				IT.Add(T);
			}

			for (const FComponentSocketDescription& SocketDesc : OutSockets)
			{
				if (SocketDesc.Type == EComponentSocketType::Socket)
				{
					FString Name;
					FString Rest;
					if (!SocketDesc.Name.ToString().Split("_", &Name, &Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
					{
						Name = SocketDesc.Name.ToString();
					}

					FTransform T = ISMComp->GetSocketTransform(SocketDesc.Name, ERelativeTransformSpace::RTS_Component);


					for (int32 I = 0; I < Count; I++)
					{
						Sockets.Emplace(FSnapSocket(ISMComp, Name, Rest, T * IT[I], SocketDesc.Name, I));
						bFound = true;
					}
				}
			}
		}
	}
	else if (UPrimitiveComponent* SourceComp = Cast<UPrimitiveComponent>(ActorComp))
	{
		TArray<FComponentSocketDescription> OutSockets;
		SourceComp->QuerySupportedSockets(OutSockets);

		for (const FComponentSocketDescription& SocketDesc : OutSockets)
		{
			if (SocketDesc.Type == EComponentSocketType::Socket)
			{
				FString Name;
				FString Rest;
				if (!SocketDesc.Name.ToString().Split("_", &Name, &Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
				{
					Name = SocketDesc.Name.ToString();
				}

				FTransform T = SourceComp->GetSocketTransform(SocketDesc.Name);

				Sockets.Emplace(FSnapSocket(SourceComp, Name, Rest, T, SocketDesc.Name));
				bFound = true;
			}
		}
	}

	return bFound;
}

void FSnapSystemModule::SnapSelComp()
{
	TArray<const AActor*> Attached;
	FSnapSocket MyS, OtherS;

	if (SearchSelComp(MyS, OtherS, Attached))
	{
		if (UPrimitiveComponent* MySComp = MyS.Comp.Get())
		{
			if (UPrimitiveComponent* OtherSComp = OtherS.Comp.Get())
			{
				FTransform T = (FTransform(FRotator(0, 180.0f, 0)) * OtherS.T).GetRelativeTransform(MyS.T);

				FSnapParameters Params;
				if (!GetDefault<UUSnapSystemSettings>()->bIgnoreSocketParams)
				{
					MakeParams(Params, MyS, OtherS);
				}

				bool bIgnoreScale = GetDefault<UUSnapSystemSettings>()->bIgnoreSplineSocketScale && (MySComp->IsA(USplineComponent::StaticClass()) || OtherSComp->IsA(USplineComponent::StaticClass()));
				UpdateTransform(Params, T, bIgnoreScale);


				T = T * MyS.T;

				bool bSnapAway = !GetDefault<UUSnapSystemSettings>()->bNoSnapback || !(RelativeT.GetRelativeTransform(MyS.T) * T).Equals(SavedT, 0.01f);

				if (bSnapAway)
				{

					USelection* ComponentSelection = GEditor->GetSelectedComponents();

					// Only move the parent-most component(s) that are selected 
					// Otherwise, if both a parent and child are selected and the delta is applied to both, the child will actually move 2x delta
					TInlineComponentArray<USceneComponent*> ComponentsToMove;
					for (FSelectedEditableComponentIterator EditableComponentIt(GEditor->GetSelectedEditableComponentIterator()); EditableComponentIt; ++EditableComponentIt)
					{
						USceneComponent* SceneComponent = Cast<USceneComponent>(*EditableComponentIt);
						if (SceneComponent)
						{
							USceneComponent* SelectedComponent = Cast<USceneComponent>(*EditableComponentIt);

							// Check to see if any parent is selected
							bool bParentAlsoSelected = false;
							USceneComponent* Parent = SelectedComponent->GetAttachParent();
							while (Parent != nullptr)
							{
								if (ComponentSelection->IsSelected(Parent))
								{
									bParentAlsoSelected = true;
									break;
								}

								Parent = Parent->GetAttachParent();
							}

							// If no parent of this component is also in the selection set, move it!
							if (!bParentAlsoSelected)
							{
								ComponentsToMove.Add(SelectedComponent);
							}
						}
					}

					// Now actually apply the delta to the appropriate component(s)
					for (USceneComponent* SceneComp : ComponentsToMove)
					{
						SceneComp->SetWorldTransform(SceneComp->GetComponentTransform().GetRelativeTransform(MyS.T) * T);
					}

					if (GetDefault<UUSnapSystemSettings>()->bSnapPivot)
					{
						SetPivot(OtherS.T, true);
					}

					//else if (GEditor->GetSelectedActorCount() > 1 || bActorAdded && !bMoveStarted)
					//{

					//	SetPivot((FTransform(GEditor->GetPivotLocation()).GetRelativeTransform(MyS.T) * T), false);
					//}
				}
			}
		}

	}

	SplineSocketCache.Reset();
}

bool FSnapSystemModule::SearchSelComp(FSnapSocket& MyS, FSnapSocket& OtherS, TArray<const AActor*>& Attached)
{
	bool bFound = false;
	const float SearchDist = GetDefault<UUSnapSystemSettings>()->SearchDist;

	float FoundDist = SearchDist * 2.0f;
	const bool bIgnoreName = GetDefault<UUSnapSystemSettings>()->bIgnoreSocketName;
	const bool bUseNoSnapTag = GetDefault<UUSnapSystemSettings>()->bUseNoSnapTag;

	const float MinDot = FMath::Cos(FMath::DegreesToRadians(180.0 - GetDefault<UUSnapSystemSettings>()->MaxAngle));

	Attached.Empty();

	TArray<FSnapSocket> Sockets;
	TSet<const AActor*> ActorsToIgnore;
	TArray<UPrimitiveComponent*> ComponentsToIgnore;

	for (FSelectionIterator ComponentIt(GEditor->GetSelectedComponentIterator()); ComponentIt; ++ComponentIt)
	{
		UPrimitiveComponent* SceneComponent = Cast<UPrimitiveComponent>(*ComponentIt);
		if (SceneComponent)
		{
			ComponentsToIgnore.Add(SceneComponent);
		}
	}


	for (FSelectedEditableComponentIterator EditableComponentIt(GEditor->GetSelectedEditableComponentIterator()); EditableComponentIt; ++EditableComponentIt)
	{
		USceneComponent* SceneComponent = Cast<USceneComponent>(*EditableComponentIt);
		if (SceneComponent)
		{
			USceneComponent* SelectedComponent = Cast<USceneComponent>(*EditableComponentIt);
			if(GetDefault<UUSnapSystemSettings>()->SourceComponentClasses.Contains(FSoftClassPath(SelectedComponent->GetClass())))
			{
				if (!bUseNoSnapTag || !SelectedComponent->ComponentHasTag(NAME_NoSnap))
				{
					GetAllSockets(Sockets, SelectedComponent);
				}
			}
		}
	}

	for (FSnapSocket& S : Sockets)
	{
		if (SearchSocket(S, FoundDist, ActorsToIgnore, ComponentsToIgnore, MyS, OtherS))
		{
			bFound = true;
		}
	}

	return bFound;
}

bool FSnapSystemModule::MatchName(const FString & Name1, const FString & Name2) const
{

	int Pol1 = (Name1.EndsWith(TEXT("+")) ? 1 : (Name1.EndsWith(TEXT("-")) ? -1 : 0));
	int Pol2 = (Name2.EndsWith(TEXT("+")) ? 1 : (Name2.EndsWith(TEXT("-")) ? -1 : 0));

	FString Name1a = Name1;
	if (Pol1 != 0)
	{
		Name1a = Name1.Left(Name1.Len() - 1);
	}
	FString Name2a = Name2;
	if (Pol2 != 0)
	{
		Name2a = Name2.Left(Name2.Len() - 1);
	}

	return Name1a.Equals(Name2a, ESearchCase::IgnoreCase) && (Pol1 * Pol2 <= 0);
}


bool FSnapSystemModule::SocketParams(const FString& pRest, FSnapParameters& Params)
{
	bool bFound = false;
	bool bEnd = false;

	FString Left, Right, Rest;
	Rest = pRest;

	const FRegexPattern myPattern(TEXT("^(R(X|Y|Z)?([0-9.-]+))$|^(A)$|^(S(X|Y|Z)?([0-9.-]+))$"));



	do {
		if (!Rest.Split("_", &Left, &Right))
		{
			Left = Rest;
			bEnd = true;
		}
		else
		{
			Rest = Right;
		}
		
		Left.ToUpperInline();
		FRegexMatcher myMatcher(myPattern, Left);
		
		
		if (myMatcher.FindNext())
		{
			if (myMatcher.GetCaptureGroup(1).Len()>0)
			{
				if (myMatcher.GetCaptureGroup(2) == TEXT("X") || myMatcher.GetCaptureGroup(2).Len() == 0)
				{
					Params.SnapAngle = FCString::Atof(*myMatcher.GetCaptureGroup(3));
					Params.SnapAngles.X = FCString::Atof(*myMatcher.GetCaptureGroup(3));
				}
				else if(myMatcher.GetCaptureGroup(2) == TEXT("Y"))
				{
					Params.SnapAngles.Y = FCString::Atof(*myMatcher.GetCaptureGroup(3));
				}
				else if (myMatcher.GetCaptureGroup(2) == TEXT("Z"))
				{
					Params.SnapAngles.Z = FCString::Atof(*myMatcher.GetCaptureGroup(3));
				}
				Params.bSnap = true;
				bFound = true;
			}
			else if (myMatcher.GetCaptureGroup(4) == TEXT("A"))
			{
				Params.bAttach = true;
				bFound = true;
			}
			else if (myMatcher.GetCaptureGroup(5).Len()>0)
			{
				if (myMatcher.GetCaptureGroup(6) == TEXT("X") || myMatcher.GetCaptureGroup(6).Len() == 0)
				{
					Params.SnapScale.X = FCString::Atof(*myMatcher.GetCaptureGroup(7));
				}
				else if (myMatcher.GetCaptureGroup(6) == TEXT("Y"))
				{
					Params.SnapScale.Y = FCString::Atof(*myMatcher.GetCaptureGroup(7));
				}
				else if (myMatcher.GetCaptureGroup(6) == TEXT("Z"))
				{
					Params.SnapScale.Z = FCString::Atof(*myMatcher.GetCaptureGroup(7));
				}
				Params.bScale = true;
				bFound = true;
			}

		}

	} while (!bEnd);

	return bFound;
}

bool FSnapSystemModule::SphereOverlapComponents(UObject* WorldContextObject, const FVector SpherePos, float SphereRadius, const TSet<const AActor*>& ActorsToIgnore, const TArray<UPrimitiveComponent*>& ComponentsToIgnore, TArray<FOverlapResult>& Overlaps/*TArray<UPrimitiveComponent*>& OutComponents*/)
{
	//OutComponents.Empty();
	Overlaps.Empty();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SphereOverlapComponents), false);
	Params.AddIgnoredActors(ActorsToIgnore.Array());
	Params.AddIgnoredComponents(ComponentsToIgnore);

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 22
	Params.bTraceAsyncScene = true;
#endif

	//TArray<FOverlapResult> Overlaps;

	FCollisionObjectQueryParams ObjectParams(FCollisionObjectQueryParams::InitType::AllObjects);

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World != nullptr)
	{
		World->OverlapMultiByObjectType(Overlaps, SpherePos, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(SphereRadius), Params);
	}


	//for (int32 OverlapIdx = 0; OverlapIdx < Overlaps.Num(); ++OverlapIdx)
	//{

	//	FOverlapResult const& O = Overlaps[OverlapIdx];
	//	if (O.Component.IsValid())
	//	{
	//		//if (!ComponentClassFilter || O.Component.Get()->IsA(ComponentClassFilter))
	//		//{
	//			OutComponents.Add(O.Component.Get());
	//		//}
	//	}

	//}

	return (Overlaps.Num() > 0);
}


void FSnapSystemModule::GetAllAttachedActors(AActor* Actor, TSet<const AActor*>& Attached) const
{
	TArray<AActor*> MyAttached;
	Actor->GetAttachedActors(MyAttached);
	for (AActor* Child : MyAttached)
	{
		Attached.Add(Child);
		GetAllAttachedActors(Child, Attached);
	}

	//TArray<UActorComponent*> Comps = Actor->GetComponentsByClass(UChildActorComponent::StaticClass());
	//for (UActorComponent* Comp : Comps)
	//{
	//	UChildActorComponent* ChildComp = Cast<UChildActorComponent>(Comp);
	//	if (ChildComp)
	//	{
	//		AActor* ChildActor = ChildComp->GetChildActor();
	//		if (ChildActor)
	//		{
	//			Attached.AddUnique(ChildActor);
	//			GetAllAttachedActors(ChildActor, Attached);
	//		}
	//	}
	//}
}




void FSnapSystemModule::SetPivot(FTransform Pivot, bool bRotate) const
{
	GUnrealEd->SetPivot(Pivot.GetLocation(), false, true);
	GUnrealEd->SetPivotMovedIndependently(true);

	//if (bRotate && GLevelEditorModeTools().IsModeActive(FMSSModeEdMode::EM_MSSModeEdModeId)) {
	//	if (FMSSModeEdMode* MssMode = static_cast<FMSSModeEdMode*>(GLevelEditorModeTools().FindMode(FMSSModeEdMode::EM_MSSModeEdModeId)))
	//	{
	//		MssMode->SetPivot(Pivot);
	//	}

	//}

}


void FSnapSystemModule::SnapSplinePoint(USplineComponent* SplineComp, int32 PointIndex, FTransform PrevT)
{
	if (!SkipSnap())
	{

		FSnapSocket MyS, OtherS;

		if (SearchSpline(SplineComp, PointIndex, MyS, OtherS))
		{
			FTransform T = (FTransform(FRotator(0, 180.0f, 0)) * OtherS.T).GetRelativeTransform(MyS.T);

			FSnapParameters Params;
			if (!GetDefault<UUSnapSystemSettings>()->bIgnoreSocketParams)
			{
				MakeParams(Params, MyS, OtherS);
			}

			UpdateTransform(Params, T);


			T = T * MyS.T;

			if (!GetDefault<UUSnapSystemSettings>()->bNoSnapback || !T.Equals(PrevT, GetDefault<UUSnapSystemSettings>()->NoSnapbackTolerance))
			{

				float TgtSize = (SplineComp->GetTangentAtSplinePoint(PointIndex, ESplineCoordinateSpace::World) /*- SplineComp->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World)*/).Size();


				SplineComp->SetLocationAtSplinePoint(PointIndex, T.GetLocation(), ESplineCoordinateSpace::World, false);


				bool bIsStart = (PointIndex == 0);
				FVector Tgt = T.GetRotation().GetForwardVector()*(SplineComp->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World) - SplineComp->GetLocationAtSplinePoint(PointIndex + (bIsStart ? 1 : -1), ESplineCoordinateSpace::World)).Size();

				if (bIsStart)
				{
					Tgt = -Tgt;
				}

				SplineComp->SplineCurves.Rotation.Points[PointIndex].OutVal = SplineComp->GetComponentTransform().GetRotation().Inverse() * T.GetRotation();

				SplineComp->SplineCurves.Scale.Points[PointIndex].OutVal = T.GetScale3D();
				SplineComp->SetTangentsAtSplinePoint(PointIndex, Tgt, Tgt, ESplineCoordinateSpace::World, true);


			}

		}
	}

	SplineSocketCache.Reset();

}

bool FSnapSystemModule::SearchSpline(USplineComponent* MyComp, const int32 PointIndex, FSnapSocket& MyS, FSnapSocket& OtherS)
{
	const bool bUseNoSnapTag = GetDefault<UUSnapSystemSettings>()->bUseNoSnapTag;

	if (bUseNoSnapTag && MyComp->ComponentHasTag(NAME_NoSnap))
	{
		return false;
	}

	const float SearchDist = GetDefault<UUSnapSystemSettings>()->SearchDist;
	float FoundDist = SearchDist * 2.0f;

	const float MinDot = FMath::Cos(FMath::DegreesToRadians(180.0 - GetDefault<UUSnapSystemSettings>()->MaxAngle));

	TSet<const AActor*> ActorsToIgnore;
	TArray<UPrimitiveComponent*> ComponentsToIgnore;
	if(MyComp->GetOwner())
	{
		ActorsToIgnore.Add(MyComp->GetOwner());
	}

	FSnapSocket S;

	S.Comp = MyComp;
	S.T = MyComp->GetTransformAtSplinePoint(PointIndex, ESplineCoordinateSpace::World, true);
	
	if (PointIndex == 0)
	{
		S.T = FTransform(FRotator(0, 180.0f, 0)) * S.T;
	}

	if (!MyComp->GetName().Split("_", &S.Name, &S.Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
	{
		S.Name = MyComp->GetName();
	}

	S.FullName = FName(*(S.Name  + (PointIndex == 0?TEXT("_0"):TEXT("_1"))));

	return SearchSocket(S, FoundDist, ActorsToIgnore, ComponentsToIgnore, MyS, OtherS);
}

bool FSnapSystemModule::CheckSocketOpen(const FSnapSocket& S, const TSet<const AActor*>& ActorsToConsider, const bool bInverse)
{
	//if (!GetDefault<UUSnapSystemSettings>()->bSnapOpenOnly)
	//{
	//	return true;
	//}

	if (UPrimitiveComponent* SComp = S.Comp.Get())
	{
		if (SComp->IsA(USplineComponent::StaticClass()))
		{
			if (FSnapSocket* CachedPtr = SplineSocketCache.Find(S))
			{
				return CachedPtr->bOpen;
			}
		}
		else
		{
			if (FSnapSocket* CachedPtr = SocketCache.Find(S))
			{
				return CachedPtr->bOpen;
			}
		}


		const float R = GetDefault<UUSnapSystemSettings>()->OpenCheckRadius;
		const FTransform Tstep = FTransform(FVector(R, 0.0, 0.0)) * S.T;
		//TArray<UPrimitiveComponent*> OverlapComponents;
		TArray<FOverlapResult> Overlaps;


		TSet<const AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(SComp->GetOwner());

		const TArray<UPrimitiveComponent*> ComponentsToIgnore;

		//		if (SphereOverlapComponents(SComp, Tstep.GetLocation(), R, UActorComponent::StaticClass(), ActorsToIgnore, ComponentsToIgnore, OverlapComponents))

		if (SphereOverlapComponents(SComp, Tstep.GetLocation(), R, ActorsToIgnore, ComponentsToIgnore, Overlaps/*OverlapComponents*/))
		{
//			for (UPrimitiveComponent* ActorComp : OverlapComponents)
			for (FOverlapResult& Overlap : Overlaps)
			{	
				if (UPrimitiveComponent* ActorComp = Overlap.GetComponent())
				{
					//filter components to consider, excludes trigger volumes and such
					if (GetDefault<UUSnapSystemSettings>()->TargetComponentClasses.Contains(FSoftClassPath(ActorComp->GetClass())))
					{
						if (ActorsToConsider.Contains(ActorComp->GetOwner()) == bInverse)
						{
							FSnapSocket Cached = S;
							Cached.bOpen = false;

							if (SComp->IsA(USplineComponent::StaticClass()))
							{
								SplineSocketCache.Add(Cached);
							}
							else
							{
								SocketCache.Add(Cached);
							}

							return false;
						}
					}
				}
			}
		}


		FSnapSocket Cached = S;
		Cached.bOpen = true;
		if (SComp->IsA(USplineComponent::StaticClass()))
		{
			SplineSocketCache.Add(Cached);
		}
		else
		{
			SocketCache.Add(Cached);
		}

		return true;
	}
	return false;
}

bool FSnapSystemModule::SearchSocket(const FSnapSocket& S, float& FoundDist, const TSet<const AActor*>& ActorsToIgnore, const TArray<UPrimitiveComponent*>& ComponentsToIgnore, FSnapSocket& MyS, FSnapSocket& OtherS)
{
	const bool bIgnoreName = GetDefault<UUSnapSystemSettings>()->bIgnoreSocketName;
	const float SearchDist = GetDefault<UUSnapSystemSettings>()->SearchDist;
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(180.0 - GetDefault<UUSnapSystemSettings>()->MaxAngle));
	const bool bSnapOpenOnly = GetDefault<UUSnapSystemSettings>()->bSnapOpenOnly;
	const bool bUseNoSnapTag = GetDefault<UUSnapSystemSettings>()->bUseNoSnapTag;

	bool bFound = false;

	TArray<UPrimitiveComponent*> OverlapComponents;
	TArray<FOverlapResult> Overlaps;


	if (UPrimitiveComponent* SComp = S.Comp.Get())
	{
		if ((!bSnapOpenOnly || CheckSocketOpen(S, ActorsToIgnore, true)) && SphereOverlapComponents(SComp, S.T.GetLocation(), SearchDist, /*UActorComponent::StaticClass(),*/ ActorsToIgnore, ComponentsToIgnore, Overlaps/*OverlapComponents*/))
		{
			TArray<AActor*> OverlapActors;

			//for (UPrimitiveComponent* ActorComp2 : OverlapComponents)
			//{
			for (FOverlapResult& Overlap : Overlaps)
			{
				if (UPrimitiveComponent* ActorComp2 = Overlap.GetComponent())
				{

					if (!bUseNoSnapTag || !ActorComp2->ComponentHasTag(NAME_NoSnap))
					{
						if (ActorComp2->GetOwner())
						{
							OverlapActors.AddUnique(ActorComp2->GetOwner());
						}
						if (GetDefault<UUSnapSystemSettings>()->TargetComponentClasses.Contains(FSoftClassPath(ActorComp2->GetClass())))
						{


							TArray<FComponentSocketDescription> OutSockets;
							ActorComp2->QuerySupportedSockets(OutSockets);

							for (const FComponentSocketDescription& SocketDesc : OutSockets)
							{
								if (SocketDesc.Type == EComponentSocketType::Socket)
								{
									FString Name2;
									FString Rest2;
									if (!SocketDesc.Name.ToString().Split("_", &Name2, &Rest2, ESearchCase::IgnoreCase, ESearchDir::FromStart))
									{
										Name2 = SocketDesc.Name.ToString();
									}

									//FTransform T2 = ActorComp2->GetSocketTransform(SocketDesc.Name);
									FTransform T2 = GetSocketTransform(ActorComp2, SocketDesc.Name, Overlap.ItemIndex);

									if (bIgnoreName || MatchName(S.Name, Name2))
									{


										float Dist = (S.T.GetLocation() - T2.GetLocation()).Size();

										if (Dist <= SearchDist && Dist <= FoundDist && FVector::DotProduct(S.T.Rotator().Vector(), T2.Rotator().Vector()) < MinDot)
										{


											if (!bSnapOpenOnly || CheckSocketOpen(FSnapSocket(ActorComp2, Name2, Rest2, T2, SocketDesc.Name), ActorsToIgnore, false))
											{
												MyS = S;
												OtherS = FSnapSocket(ActorComp2, Name2, Rest2, T2, SocketDesc.Name);
												bFound = true;
												FoundDist = Dist;
											}


										}
									}
								}
							}
						}
					}
				}
			}

			if (GetDefault<UUSnapSystemSettings>()->TargetComponentClasses.Contains(FSoftClassPath(USplineComponent::StaticClass())))
			{
				for (AActor* OverlapActor : OverlapActors)
				{
					if (OverlapActor)
					{
						TArray<UActorComponent*> OverlapSplines;// = OverlapActor->GetComponentsByClass(USplineComponent::StaticClass());
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
						OverlapSplines = OverlapActor->GetComponentsByClass(USplineComponent::StaticClass());
#else
						OverlapActor->GetComponents(USplineComponent::StaticClass(), OverlapSplines);
#endif


						for (UActorComponent* SplineTmp : OverlapSplines)
						{
							if (USplineComponent* SplineComp = Cast<USplineComponent>(SplineTmp))
							{
								if (!bUseNoSnapTag || !SplineComp->ComponentHasTag(NAME_NoSnap))
								{
									FString Name2;
									FString Rest2;
									if (!SplineComp->GetName().Split("_", &Name2, &Rest2, ESearchCase::IgnoreCase, ESearchDir::FromStart))
									{
										Name2 = SplineComp->GetName();
									}



									if (bIgnoreName || MatchName(S.Name, Name2))
									{
										TArray<FTransform> EndPoints2;
										EndPoints2.Add(FTransform(FRotator(0, 180.0f, 0)) * SplineComp->GetTransformAtSplinePoint(0, ESplineCoordinateSpace::World, true));
										EndPoints2.Add(SplineComp->GetTransformAtSplinePoint(SplineComp->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World, true));

										int32 Num = 0;
										for (FTransform& T2 : EndPoints2)
										{

											float Dist = (S.T.GetLocation() - T2.GetLocation()).Size();

											if (Dist <= SearchDist && Dist <= FoundDist && FVector::DotProduct(S.T.Rotator().Vector(), T2.Rotator().Vector()) < MinDot)
											{
												const FName FullName = FName(*(SplineComp->GetName() + TEXT("_") + FString::FromInt(Num)));

												if (!bSnapOpenOnly || CheckSocketOpen(FSnapSocket(SplineComp, Name2, Rest2, T2, FullName), ActorsToIgnore, false))
												{
													bFound = true;
													FoundDist = Dist;

													MyS = S;
													OtherS = FSnapSocket(SplineComp, Name2, Rest2, T2, FullName);
												}
											}

											Num++;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (bFound)
	{
		MyS.T.NormalizeRotation();
		OtherS.T.NormalizeRotation();
	}

	return bFound;
}

void FSnapSystemModule::MakeParams(FSnapParameters& Params, const FSnapSocket& MyS, const FSnapSocket& OtherS)
{
	FSnapParameters MyParams, OtherParams;

	MyParams.SnapAngles = GetDefault<UUSnapSystemSettings>()->DefaultRotSnap;
	MyParams.SnapScale = GetDefault<UUSnapSystemSettings>()->DefaultScaleSnap;
	OtherParams.SnapAngles = GetDefault<UUSnapSystemSettings>()->DefaultRotSnap;
	OtherParams.SnapScale = GetDefault<UUSnapSystemSettings>()->DefaultScaleSnap;

	SocketParams(MyS.Rest, MyParams);
	SocketParams(OtherS.Rest, OtherParams);
	Params.SetMerge(OtherParams, MyParams);
}

void FSnapSystemModule::UpdateTransform(const FSnapParameters& Params, FTransform& T, bool bIgnoreScale)
{
	FVector RotSnap = GetDefault<UUSnapSystemSettings>()->DefaultRotSnap;
	FRotator R = T.Rotator();

	if (Params.bSnap)
	{
		RotSnap = Params.SnapAngles;
	}


	if (RotSnap.X == 0.0f)
	{
		R.Roll = 0.0f;
	}
	else
	{
		R.Roll -= FMath::RoundToFloat(R.Roll / RotSnap.X) * RotSnap.X;
	}


	if (RotSnap.Y == 0.0f)
	{
		R.Pitch = 0.0f;
	}
	else
	{
		R.Pitch -= FMath::RoundToFloat(R.Pitch / RotSnap.Y) * RotSnap.Y;
	}


	if (RotSnap.Z == 0.0f)
	{
		R.Yaw = 0.0f;
	}
	else
	{
		R.Yaw -= FMath::RoundToFloat(R.Yaw / RotSnap.Z) * RotSnap.Z;
	}


	T.SetRotation(FQuat(R));


	if (!bIgnoreScale)
	{
		FVector ScaleSnap = GetDefault<UUSnapSystemSettings>()->DefaultScaleSnap;
		FVector S = T.GetScale3D();

		if (Params.bScale)
		{
			ScaleSnap = Params.SnapScale;
		}

		S.X = (ScaleSnap.X == 0.0f ? 1.0f : S.X);
		S.Y = (ScaleSnap.Y == 0.0f ? 1.0f : S.Y);
		S.Z = (ScaleSnap.Z == 0.0f ? 1.0f : S.Z);

		T.SetScale3D(S);
	}
	else
	{
		T.SetScale3D(FVector(1.f, 1.f, 1.f));
	}
}

bool FSnapSystemModule::SkipSnap()
{
	const FSnapSystemCommands& Commands = FSnapSystemCommands::Get();
	bool bIsChordPressed = false;
	for (uint32 i = 0; i < static_cast<uint8>(EMultipleKeyBindingIndex::NumChords); ++i)
	{
		EMultipleKeyBindingIndex ChordIndex = static_cast<EMultipleKeyBindingIndex> (i);
		const FInputChord& Chord = *Commands.HoldToSkipSnap->GetActiveChord(ChordIndex);

		bIsChordPressed |= (Chord.NeedsControl() == GCurrentLevelEditingViewportClient->IsCtrlPressed())
			&& (Chord.NeedsAlt() == GCurrentLevelEditingViewportClient->IsAltPressed())
			&& (Chord.NeedsShift() == GCurrentLevelEditingViewportClient->IsShiftPressed())
			&& GCurrentLevelEditingViewportClient->Viewport->KeyState(Chord.Key) == true;
	}
	return bIsChordPressed;
}


bool FSnapSystemModule::EqualsTransform(const FTransform& T1, const FTransform& T2)
{
	bool bLocEq = T1.GetLocation().Equals(T2.GetLocation(), GetDefault<UUSnapSystemSettings>()->NoSnapbackTolerance);
	bool bRotEq = T1.GetRotation().Equals(T2.GetRotation());
	return bLocEq && bRotEq;
}

FTransform FSnapSystemModule::GetSocketTransform(const FSnapSocket& Socket)
{
	if (UPrimitiveComponent* Comp = Socket.Comp.Get())
	{
		if (USplineComponent* SplineComp = Cast<USplineComponent>(Comp))
		{
			if (Socket.FullName.ToString().EndsWith(TEXT("0")))
			{
				return FTransform(FRotator(0, 180.0f, 0)) * SplineComp->GetTransformAtSplinePoint(0, ESplineCoordinateSpace::World, true);
			}
			else
			{
				return SplineComp->GetTransformAtSplinePoint(SplineComp->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World, true);
			}
		}
		else
		{
			return GetSocketTransform(Comp, Socket.FullName, Socket.Index);
		}
	}
	return FTransform();
}

FTransform FSnapSystemModule::GetSocketTransform(UPrimitiveComponent* Comp, FName SocketName, int32 InstanceIndex)
{
	if (Comp)
	{
		if (UInstancedStaticMeshComponent* ISMComp = Cast<UInstancedStaticMeshComponent>(Comp))
		{
			FTransform TI;
			ISMComp->GetInstanceTransform(InstanceIndex, TI, true);
			return Comp->GetSocketTransform(SocketName, ERelativeTransformSpace::RTS_Component) * TI;
		}
		else
		{
			return Comp->GetSocketTransform(SocketName);
		}
	}
	return FTransform();
}
void FSnapSystemModule::CopySockets()
{
	UCopyPasteSockets::CopySockets();
}
void FSnapSystemModule::PasteSockets()
{
	UCopyPasteSockets::PasteSockets();

}



#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSnapSystemModule, SnapSystem)
