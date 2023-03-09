// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#include "SnapSystemCommands.h"

#define LOCTEXT_NAMESPACE "FSnapSystemModule"

void FSnapSystemCommands::RegisterCommands()
{
	UI_COMMAND(SnapSystemEnable, "Snap System", "Enable or disable Snap System Plugin", EUserInterfaceActionType::ToggleButton, FInputGesture());
	UI_COMMAND(SnapSystemOptions, "Snap System Settings...", "Edit Snap System Plugin settings", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(HoldToSkipSnap, "Hold To Skip Snap", "Hold this key to skip the snapping", EUserInterfaceActionType::ToggleButton, FInputGesture(EKeys::C));
	UI_COMMAND(ShowSockets, "Show Sockets", "Show / hide sockets on selected actors", EUserInterfaceActionType::ToggleButton, FInputGesture());

	UI_COMMAND(CopySockets, "Copy Sockets...", "Copy sockets from selected actors", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(PasteSockets, "Paste Sockets...", "Paste sockets on selected actor", EUserInterfaceActionType::Button, FInputGesture());

	UI_COMMAND(ExportSockets, "Export Sockets...", "Export sockets from selected assets to a text file", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ImportSockets, "Import Sockets...", "Import sockets from a text file to selected asssets", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(DeleteSockets, "Delete Sockets...", "Delet sockets in selected assets", EUserInterfaceActionType::Button, FInputGesture());

}

#undef LOCTEXT_NAMESPACE
