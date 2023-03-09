// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AbleStyle.h"
#include "Runtime/Slate/Public/Framework/Commands/Commands.h"
#include "Styling/ISlateStyle.h"

class FAblAbilityEditorCommands : public TCommands<FAblAbilityEditorCommands>
{
public:
	FAblAbilityEditorCommands()
		: TCommands<FAblAbilityEditorCommands>(
			TEXT("AblAbilityEditor"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "AblAbilityEditor", "Ability Editor"), // Localized context name for displaying
			NAME_None, // Parent
			FAbleStyle::Get()->GetStyleSetName() // Icon Style Set
			)
	{
	}

	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface

public:
	TSharedPtr<FUICommandInfo> m_AddTask;
	TSharedPtr<FUICommandInfo> m_DuplicateTask;
	TSharedPtr<FUICommandInfo> m_RemoveTask;
	TSharedPtr<FUICommandInfo> m_PlayAbility;
	TSharedPtr<FUICommandInfo> m_StopAbility;
	TSharedPtr<FUICommandInfo> m_StepAbility;
	TSharedPtr<FUICommandInfo> m_StepAbilityBackwards;
	TSharedPtr<FUICommandInfo> m_Resize;
	TSharedPtr<FUICommandInfo> m_Validate;
	TSharedPtr<FUICommandInfo> m_SetPreviewAsset;
	TSharedPtr<FUICommandInfo> m_ResetPreviewAsset;
	TSharedPtr<FUICommandInfo> m_ToggleCost;
	TSharedPtr<FUICommandInfo> m_ToggleShowCollisionQueries;
	TSharedPtr<FUICommandInfo> m_ToggleDrawCameraComponent;
	TSharedPtr<FUICommandInfo> m_ToggleDrawArrowComponent;
	TSharedPtr<FUICommandInfo> m_ToggleDrawCharacterCollision;
	TSharedPtr<FUICommandInfo> m_CaptureThumbnail;
	TSharedPtr<FUICommandInfo> m_CopyTask;
	TSharedPtr<FUICommandInfo> m_PasteTask;
	TSharedPtr<FUICommandInfo> m_TutorialWebLink;
	TSharedPtr<FUICommandInfo> m_DiscordWebLink;
	TSharedPtr<FUICommandInfo> m_RecopyParentTasks;
};

class FAblAbilityEditorViewportCommands : public TCommands<FAblAbilityEditorViewportCommands>
{
public:
	FAblAbilityEditorViewportCommands()
		: TCommands<FAblAbilityEditorViewportCommands>(
			TEXT("AblAbilityEditorViewport"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "AblAbilityEditorViewport", "Ability Editor Viewport"), // Localized context name for displaying
			NAME_None, // Parent
			FAbleStyle::Get()->GetStyleSetName() // Icon Style Set
			)
	{
		
	}

	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface

public:
	TSharedPtr<FUICommandInfo> m_CameraFollow;
};