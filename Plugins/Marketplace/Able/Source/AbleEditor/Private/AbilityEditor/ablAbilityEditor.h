// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Editor/Kismet/Public/BlueprintEditor.h"

#include "AbilityEditor/ablAbilityEditorCommands.h"
#include "AbilityEditor/ablAbilityEditorViewportClient.h"

class FAblAbilityEditorToolbar;
class UAblAbility;
class UAblAbilityBlueprint;
class UAblAbilityComponent;
class UAblAbilityContext;
class UAblAbilityEditorSettings;
class UAblAbilityTask;
class UAblAbilityValidator;
class UAnimationAsset;

struct AssetData;
struct FSlateBrush;

/* Able Ability Editor, extends Blueprint Editor. */
class FAblAbilityEditor : public FBlueprintEditor
{
public:
	/* Initializes an instance of the Editor. */
	void InitAbilityEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UBlueprint*>& InAbilityBlueprints, bool ShouldUseDataOnlyEditor);

	/* Reinitialize the editor and do any required work.*/
	void Reinitialize();

	FAblAbilityEditor();
	virtual ~FAblAbilityEditor();

	// IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	// End of IToolkit interface

	/* Returns the documentation location for this editor. Not sure if it's relevant to 3rd party editors like this one. */
	virtual FString GetDocumentationLink() const override;

	/* Returns a pointer to the Blueprint object we are currently editing, as long as we are editing exactly one */
	virtual UBlueprint* GetBlueprintObj() const override;

	/* Returns the Ability Blueprint we're editing. */
	UAblAbilityBlueprint* GetAbilityBlueprint() const;

	/* Returns Ability Blueprint down-casted to an UObject. */
	UObject* GetAbilityBlueprintAsObject() const;

	/* Returns our Ability Blueprint as the generated UAblAbility object. */
	UAblAbility* GetAbility() const;

	/* Returns a Const version of the UAblAbility object. */
	const UAblAbility* GetConstAbility() const;

	/* Returns the Ability Task that we are currently inspecting. */
	const UAblAbilityTask* GetCurrentlySelectedAbilityTask() const;

	/* Returns a mutable version of the Ability Task that we are currently inspecting. */
	UAblAbilityTask* GetMutableCurrentlySelectedAbilityTask() const;

	/* Toggles a run-time dependency to the provide Task. */
	void ToggleDependencyOnTask(const UAblAbilityTask* Task, const UAblAbilityTask* DependsOn);

	/* Returns true, or false, if the provided Task has dependency on the DependsOn Task. */
	bool TaskHasDependency(const UAblAbilityTask* Task, const UAblAbilityTask* DependsOn) const;

	/* Toggles the Lock status on a Task. */
	void ToggleLockOnTask(UAblAbilityTask* Task);

	//~ Begin FTickableEditorObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableEditorObject Interface

	/* Returns the dirty brush for a given mode. */
	const FSlateBrush* GetDirtyImageForMode(FName Mode) const;

	/* Adds a new Task to the Ability. */
	void AddNewTask();

	/* Removes the currently selected Task from the Ability. */
	void RemoveCurrentTask();

	/* Removes the provided Task from the Ability. */
	void RemoveTask(const UAblAbilityTask& Task);

	/* Duplicates the current Task.*/
	void DuplicateCurrentTask();

	/* Copies the currently Task to the clipboard. */
	void CopyCurrentTaskToClipboard();

	/* Handles creating a Task that came from the clipboard. */
	void PasteClipboardTask();

	/* Sets the provided Task as the Currently Selected Task.*/
	void SetCurrentTask(const UAblAbilityTask* Task);

	/* Sets the Ability Length to the minimum amount of time required to play all Tasks to completion. */
	void ResizeAbility();

	/* Executes Ability Validators on the current Ability. */
	void Validate() const;
	
	/* Returns the Ability Toolbar. */
	TSharedPtr<FAblAbilityEditorToolbar> GetAbilityToolbar() { return m_AbilityEditorToolbar; }

	/* Returns the Start and End time of the Ability. */
	FVector2D GetAbilityTimeRange() const;
	
	/* Returns the Preview Scene used in our Viewport. */
	FORCEINLINE FAbilityEditorPreviewScene& GetPreviewScene() { return m_PreviewScene; }
	
	/* Returns the Ability Editor Settings. */
	FORCEINLINE UAblAbilityEditorSettings& GetEditorSettings() { return *m_EditorSettings; }

	/* Returns a Const version of the Ability Editor Settings. */
	FORCEINLINE const UAblAbilityEditorSettings& GetEditorSettings() const { return *m_EditorSettings; }

	/* Returns the Ability Validator. */
	FORCEINLINE UAblAbilityValidator& GetValidator() { return *m_Validator; }
	
	/* Whether or not to show Task Cost Estimates. */
	FORCEINLINE bool ShowTaskCostEstimate() { return m_DisplayTaskCost; }

	/* Returns the color of a Task according to the estimated cost. */
	FLinearColor GetTaskCostColor(const UAblAbilityTask& Task) const;

	/* Returns if we are drawing all Collision queries or not. */
	bool IsDrawingCollisionQueries() const;

	/* Whether or not to allow Arrow Components on the Preview Actor. */
	FORCEINLINE bool IsDrawingArrowComponent() const { return m_DrawArrowComponent; }

	/* Whether or not to allow Camera Components on the Preview Actor. */
	FORCEINLINE bool IsDrawingCameraComponent() const { return m_DrawCameraComponent; }

	/* Whether or not to allow Character Collision on the Preview Actor. */
	FORCEINLINE bool IsDrawingCharacterCollision() const { return m_DrawCharacterCollision; }

	/* Plays the current Ability on the Preview Actor. */
	void PlayAbility();

	/* Pauses the current playing Ability. */
	void PauseAbility();

	/* If pauses, this steps the Ability forward a frame. */
	void StepAbility();

	/* If pauses, this steps the Ability backwards a frame. */
	void StepAbilityBackwards();

	/* Stops the currently playing Ability. */
	void StopAbility();

	/* Whether or not an Ability is currently executing on the Preview Actor. */
	bool IsPlayingAbility() const;

	/* Used to directly set the time of the Ability in the Preview Viewport. */
	void SetAbilityPreviewTime(float Time);

	/* Returns true if the Ability execution is paused. */
	FORCEINLINE bool IsPaused() const { return m_IsPaused; }
	
	/* Returns the length, in seconds, of the current Ability. */
	float GetAbilityLength() const;

	/* Returns the current time of the Ability begin played on the Preview Actor. */
	float GetAbilityCurrentTime() const;

	/* Creates a new Preview Actor in the provided World. */
	void CreatePreviewActor(UWorld* World);

	/* Spawns the Select Preview Asset Modal window. */
	void SetPreviewAsset();

	void SetPreviewAsset(const FAssetData& PreviewAsset);

	void SetTargetAsset(const FAssetData& TargetAsset);

	/* Sets the Viewport Widget. */
	void SetViewportWidget(TWeakPtr<class SAbilityEditorViewportTabBody>& Viewport) { m_Viewport = Viewport; }

	/* Returns the Tab Icon of the Ability Editor. */
	virtual const FSlateBrush* GetDefaultTabIcon() const override;

	/* Generates a context menu for Tasks and pushes it onto the window stack. */
	TSharedRef<SWidget> GenerateTaskContextMenu();

	/* Get our Preview Actor. */
	AActor* GetAbilityPreviewActor() const { return m_PreviewActor.Get(); }

	/* Add a Preview Target Actor. */
	void AddAbilityPreviewTarget();

	/* Remove a specific Preview Target Actor. */
	void DeleteAbilityPreviewTarget(AActor* TargetToDelete);

	/* Destroy all Preview Target Actors.*/
	void ClearAllPreviewTargets();

	/* Clear any Forced Targets. */
	void ClearForceTarget();

	/* Force a Target to a specicied index. */
	void SetForceTarget(int index);

	/* Return all currently allocated Preview Targets. */
	const TArray<TWeakObjectPtr<AActor>>& GetAbilityPreviewTargets() const { return m_PreviewTargets; }

	/* Respawns the Preview Actor into its original state. */
	void ResetPreviewActor();

	/* Respawns the Target Actors into their original states. */
	void ResetTargetActors();

	/* Request Editor Refresh all elements.*/
	void RequestEditorRefresh();

	/* Returns the dynamic identifier binding for a given class. */
	FString GetDynamicBindingIdentifierForClass(const UClass* InClass) const;

	/* Opens the Tutorial web page.*/
	void OpenTutorialWebLink() const;

	/* Opens the Discord channel page.*/
	void OpenDiscordWebLink() const;

	/* Copies the Parent classes Tasks into the Ability.*/
	void CopyParentTasks();
protected:
	virtual void OnBlueprintChangedImpl(UBlueprint* InBlueprint, bool bIsJustBeingCompiled = false) override;

	/* Called before Assets are saved. */
	virtual void GetSaveableObjects(TArray<UObject*>& OutObjects) const override;

	/* Menu Extension Helper. */
	void ExtendMenu();

	/* Toolbar Extension Helper. */
	void ExtendToolbar();

	/* Bind all Commands needed by this Editor. */
	void BindCommands();

	AActor* InternalSpawnActor(UClass* actorClass, const FTransform& initTransform = FTransform::Identity);

	/* Creates the Preview Actor. */
	void SpawnPreviewActor(const FTransform& initTransform = FTransform::Identity);

	/* Creates a Target Actor. */
	void SpawnTargetActor(const FTransform& initTransform = FTransform::Identity);

	/* Toggles Cost View. */
	void ToggleCostView();

	/* Toggles Drawing Collision Queries. */
	void ToggleShowQueryVolumes();

	/* Toggles Character Collision. */
	void ToggleDrawCharacterCollision();

	/* Toggles Arrow Component. */
	void ToggleDrawArrowComponent();

	/* Toggles Camera Component. */
	void ToggleDrawCameraComponent();

	/* Captures the current Viewport image as thumbnail. */
	void CaptureThumbnail();

	/* Helper method to spawn a Preview Actor from a given Blueprint. */
	void SpawnPreviewActorFromBlueprint(UWorld* World);

	/* Helper method to create the Task Dependency options of the Task Context Menu. */
	void CreateTaskDependencyMenu(FMenuBuilder& MenuBuilder);

	/* Helper method to throttle Task updates. */
	bool ShouldCallTaskTick(float DeltaTime);

	/* Helper method to create the preview Context for Task / Ability Ticking. */
	void CreatePreviewContext();

	/** Brush to use as a dirty marker for assets */
	const FSlateBrush* m_AssetDirtyBrush;
	
	// Our Tab Icon
	const FSlateBrush* m_TabIcon;

	// Extenders
	TSharedPtr<FExtender> m_ToolbarExtender;
	TSharedPtr<FExtender> m_MenuExtender;

	// Editor Toolbar
	TSharedPtr<FAblAbilityEditorToolbar> m_AbilityEditorToolbar;

	// Our currently selected Task.
	TWeakObjectPtr<const UAblAbilityTask> m_CurrentTask;

	// Preview Scene for our Viewport
	FAbilityEditorPreviewScene m_PreviewScene;

	// Editor Settings
	UAblAbilityEditorSettings* m_EditorSettings;

	// Editor Viewport
	TWeakPtr<class SAbilityEditorViewportTabBody> m_Viewport;

	// Our Preview Actor (and their Ability Component)
	TWeakObjectPtr<AActor> m_PreviewActor;
	TWeakObjectPtr<UAblAbilityComponent> m_PreviewAbilityComponent;
	FTransform m_PreviewActorSavedTransform;

	// Our Preview Targets
	TArray<TWeakObjectPtr<AActor>> m_PreviewTargets;
	TArray<FTransform> m_PreviewTargetsSavedTransforms;

	// The Force Target of the Ability.
	TWeakObjectPtr<AActor> m_ForcedTarget;

	// The Ability Validator
	TWeakObjectPtr<UAblAbilityValidator> m_Validator;

	// Task Preview Context
	TWeakObjectPtr<UAblAbilityContext> m_PreviewContext;

	// Pause State of the Ability.
	bool m_IsPaused;

	// Various UI Options for our Ability Editor
	bool m_DisplayTaskCost;
	bool m_DrawCharacterCollision;
	bool m_DrawArrowComponent;
	bool m_DrawCameraComponent;

	// Throttle values to avoid spamming Task updates while in Editor.
	const float TASK_UPDATE_RATE = 1.0f / 30.0f; // 30Hz.
	float m_TaskTickTimer;

	// Recreate Task timer.
	float m_RecreateTaskTimer;
};