// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/ablAbilityEditorCommands.h"

#include "AbleEditorPrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditorCommands"

void FAblAbilityEditorCommands::RegisterCommands()
{
	UI_COMMAND(m_AddTask, "Add Task", "Inserts a new task into the ability timeline.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_DuplicateTask, "Duplicate Task", "Duplicates the currently selected task.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_RemoveTask, "Remove Task", "Removes the current task.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_PlayAbility, "Play", "Plays the current Ability.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(m_StopAbility, "Stop", "Stops the current Ability.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_StepAbility, "Forward Step", "Steps the Ability forward a frame.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_StepAbilityBackwards, "Backward Step", "Steps the Ability backwards a frame.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_Resize, "Resize", "Resizes the ability length to the last task's completion, leaving no dead space at the end of the ability.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_Validate, "Validate", "Validates an Ability by checking it against a variety of unit tests.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_SetPreviewAsset, "Set Preview Asset", "Sets the asset to use for the Preview Actor.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_ResetPreviewAsset, "Reset Preview Asset", "Resets the preview asset, this can clean up any one-time state changes you want to test.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_ToggleCost, "Show Cost", "Shows the estimated cost of a task. Green tasks are fast, while red are more complex.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(m_ToggleShowCollisionQueries, "Show Collision Queries", "Draws the Collision Query when it executes in an Ability. This includes Queries, Sweeps, and Raycasts.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(m_ToggleDrawArrowComponent, "Show Arrow Component", "Draws the Arrow Component (if it exists) on the Preview Actor.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(m_ToggleDrawCameraComponent, "Show Camera Component", "Draws the Camera Component (if it exists) on the Preview Actor.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(m_ToggleDrawCharacterCollision, "Show Character Collision", "Draws the Character Collision (if it exists) on the Preview Actor.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(m_CaptureThumbnail, "Capture Thumbnail", "Captures the current viewport image and uses it as the thumbnail for this Ability.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_CopyTask, "Copy Task", "Copies the current Task to the clipboard.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_PasteTask, "Paste Task", "Pastes the current Task from the clipboard into the Ability.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(m_TutorialWebLink, "Tutorials...", "Various tutorials about using Able", EUserInterfaceActionType::None, FInputChord());
	UI_COMMAND(m_DiscordWebLink, "Discord...", "Official Able Discord Channel", EUserInterfaceActionType::None, FInputChord());
	UI_COMMAND(m_RecopyParentTasks, "Recopy Parent Tasks...", "Recopies all Parent Tasks into the Child Ability. This will likely cause dupes if you have the original copies there.", EUserInterfaceActionType::None, FInputChord());
}


void FAblAbilityEditorViewportCommands::RegisterCommands()
{
	UI_COMMAND(m_CameraFollow, "Follow", "Sets the camera to follow the preview actor.", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE