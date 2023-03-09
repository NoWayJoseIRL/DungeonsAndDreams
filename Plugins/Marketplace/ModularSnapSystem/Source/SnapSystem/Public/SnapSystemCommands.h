// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SnapSystemStyle.h"

class FSnapSystemCommands : public TCommands<FSnapSystemCommands>
{
public:

	FSnapSystemCommands()
		: TCommands<FSnapSystemCommands>(TEXT("SnapSystem"), NSLOCTEXT("Contexts", "SnapSystem", "SnapSystem Plugin"), NAME_None, FSnapSystemStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > SnapSystemOptions;
	TSharedPtr< FUICommandInfo > SnapSystemEnable;
	TSharedPtr< FUICommandInfo > HoldToSkipSnap;
	TSharedPtr< FUICommandInfo > ShowSockets;

	TSharedPtr< FUICommandInfo > CopySockets;
	TSharedPtr< FUICommandInfo > PasteSockets;

	TSharedPtr< FUICommandInfo > ExportSockets;
	TSharedPtr< FUICommandInfo > ImportSockets;
	TSharedPtr< FUICommandInfo > DeleteSockets;

};
