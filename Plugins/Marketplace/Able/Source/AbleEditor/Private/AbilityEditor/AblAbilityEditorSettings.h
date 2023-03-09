// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AblAbilityEditorSettings.generated.h"

UENUM()
enum EAblAbilityEditorTimeStep
{
	FifteenFPS UMETA(DisplayName = "15 FPS"),
	ThirtyFPS UMETA(DisplayName = "30 FPS"),
	SixtyFPS UMETA(DisplayName = "60 FPS"),
	OneTwentyFPS UMETA(DisplayName = "120 FPS"),
};

// Settings for the Able Ability editor
UCLASS(config = EditorPerProjectUserSettings)
class UAblAbilityEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UAblAbilityEditorSettings();

	/* What to set the Field of View (FOV) to for the in-game viewport in the Ability Editor.*/
	UPROPERTY(config, EditAnywhere, Category = Viewport, meta = (DisplayName = "Field of View", ClampMin = 70.0f, ClampMax = 180.0f))
	float m_FOV;

	/* Whether to mute the in-game audio of the viewport or not.*/
	UPROPERTY(config, EditAnywhere, Category = Viewport, meta = (DisplayName = "Mute Audio"))
	bool m_MuteAudio;

	/* At what rate to step an Ability when paused and stepping frame by frame.*/
	UPROPERTY(config, EditAnywhere, Category = Viewport, meta = (DisplayName = "Ability Step Rate"))
	TEnumAsByte<EAblAbilityEditorTimeStep> m_AbilityStepTimeStep;

	/* How often (in seconds) to place a major guideline on the Timeline Editor*/
	UPROPERTY(config, EditAnywhere, Category = Guidelines, meta = (DisplayName="Major Line Frequency", ClampMin=0.1f))
	float m_MajorLineFrequency;

	/* How often (in seconds) to place a minor guideline on the Timeline Editor*/
	UPROPERTY(config, EditAnywhere, Category = Guidelines, meta = (DisplayName = "Minor Line Frequency", ClampMin=0.01f))
	float m_MinorLineFrequency;

	/* Place the time above each Major guideline.*/
	UPROPERTY(config, EditAnywhere, Category = Guidelines, meta = (DisplayName = "Show Guideline Labels"))
	bool m_ShowGuidelineLabels;

	/* If true, Task nodes will use a more dynamic, descriptive title than just the Task name.*/
	UPROPERTY(config, EditAnywhere, Category = Timeline, meta = (DisplayName = "Descriptive Task Titles"))
	bool m_ShowDescriptiveTaskTitles;

	/* The preview asset used in the Viewport.*/
	UPROPERTY(config, EditAnywhere, Category = "Preview Asset", meta = (DisplayName = "Preview Asset"))
	FSoftObjectPath m_PreviewAsset;

	/* The target asset used in the Viewport.*/
	//UPROPERTY(config, EditAnywhere, Category = "Preview Asset", meta = (DisplayName = "Allowed Preview Classes"))
	//TArray<UClass*> m_PreviewAllowedClasses;

	/* The target asset used in the Viewport.*/
	UPROPERTY(config, EditAnywhere, Category = "Target Asset", meta = (DisplayName = "Target Asset"))
	FSoftObjectPath m_TargetAsset;

	/* The target asset used in the Viewport.*/
	//UPROPERTY(config, EditAnywhere, Category = "Target Asset", meta = (DisplayName = "Allowed Target Classes"))
	//TArray<UClass*> m_TargetAllowedClasses;

	/* If true, Static Meshes will be selectable when choosing a Preview Asset. */
	//UPROPERTY(config, EditAnywhere, Category ="Preview Asset", meta = (DisplayName = "Allow Static Meshes"))
	//bool m_AllowStaticMeshes;

	/* If true, Skeletal Meshes will be selectable when choosing a Preview Asset. */
	//UPROPERTY(config, EditAnywhere, Category = "Preview Asset", meta = (DisplayName = "Allow Skeletal Meshes"))
	//bool m_AllowSkeletalMeshes;

	/* If true, Animation Blueprints will be selectable when choosing a Preview Asset. */
	//UPROPERTY(config, EditAnywhere, Category = "Preview Asset", meta = (DisplayName = "Allow Animation Blueprints"))
	//bool m_AllowAnimationBlueprints;

	/* If true, Blueprints that inherit from Pawn (Pawns and Characters for example) will be selectable when choosing a Preview Asset. */
	//UPROPERTY(config, EditAnywhere, Category = "Preview Asset", meta = (DisplayName = "Allow Pawn Blueprints"))
	//bool m_AllowPawnBlueprints;

	/* If true, Able will use the value Preview Component Rotation value to rotate the mesh. */
	UPROPERTY(config, EditAnywhere, Category = "Preview Asset", meta = (DisplayName = "Mesh Rotation"))
	bool m_RotateMesh;

	/* If the preview asset is just a simple Skeletal/Static/etc Mesh component, this option will modify its component rotation so its facing the proper direction.*/
	UPROPERTY(config, EditAnywhere, Category = "Preview Asset", meta = (DisplayName = "Preview Component Rotation", EditCondition = m_RotateMesh))
	FRotator m_MeshRotation;

	/* If true, dragging Tasks around on the timeline will snap them to the user defined snap units.*/
	UPROPERTY(config, EditAnywhere, Category = "Timeline", meta = (DisplayName = "Snap Tasks"))
	bool m_SnapTasks;

	/* The unit, in seconds, to use when snapping Tasks. Smallest valid unit is 1 millisecond. */
	UPROPERTY(config, EditAnywhere, Category = "Timeline", meta = (DisplayName = "Snap Unit", EditCondition=m_SnapTasks, ClampMin = 0.001f))
	float m_SnapUnit;

	/* If true, the Ability Editor will poll every second and see if it needs to re-create any out of date Task classes. This can occur with Custom Tasks during iteration. */
	UPROPERTY(config, EditAnywhere, Category = Viewport, meta = (DisplayName = "Auto Re-create Tasks"))
	bool m_AutoRecreateTasks;

	/* Returns the delta time value based on the rate selected by the user. */
	float GetAbilityTimeStepDelta() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
