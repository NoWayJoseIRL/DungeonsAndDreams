// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#include "CopyPasteSockets.h"
#include "SnapSystem.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/Selection.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "UObject/PropertyPortFlags.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FSnapSystemModule"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
#define MSS_STYLE FAppStyle
#else
#define MSS_STYLE FEditorStyle
#endif

UCopyPasteSockets::UCopyPasteSockets(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

bool UCopyPasteSockets::ShowCopyDialog(bool& bPivotAtZero, bool& bOpenOnly)
{
	bool bResult = false;

	TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("CopySocketsTitle", "Copy Sockets"))
		.SizingRule(ESizingRule::Autosized)
		.ClientSize(FVector2D(0.f, 300.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SVerticalBox> xxx =

		SNew(SVerticalBox)
		
		+ SVerticalBox::Slot()
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		.FillHeight(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CopySocketsText", "Copy Static Mesh Sockets from selected actors."))
			.Font(MSS_STYLE::GetFontStyle("StandardDialog.LargeFont"))
		]
		
		+ SVerticalBox::Slot()
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		.AutoHeight()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CopySocketsPivot", "Relative to World Origin"))
				.Font(MSS_STYLE::GetFontStyle("StandardDialog.SmallFont"))
			]
			.OnCheckStateChanged_Lambda([&bPivotAtZero](ECheckBoxState State) { bPivotAtZero = (State == ECheckBoxState::Checked); })
			.ToolTipText(LOCTEXT("CopySocketsPivotTip", "If checked will copy socket transforms relative to the world origin, otherwise relative to the first selected actor"))
		]


		+ SVerticalBox::Slot()
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))

		.AutoHeight()
		.VAlign(VAlign_Top)
		[
			SNew(SCheckBox)
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CopySocketsOpen", "Copy Open Sockets Only"))
				.Font(MSS_STYLE::GetFontStyle("StandardDialog.SmallFont"))
			]
			.OnCheckStateChanged_Lambda([&bOpenOnly](ECheckBoxState State) { bOpenOnly = (State == ECheckBoxState::Checked); })
			.ToolTipText(LOCTEXT("CopySocketsOpenTip", "Copy only open sockets (not hidden by other selected actors)"))

		]



		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		[
				SNew(SUniformGridPanel)
				.SlotPadding(MSS_STYLE::GetMargin("StandardDialog.SlotPadding"))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("CopySocketsButtonOk", "Copy"))
					.HAlign(HAlign_Center)
					//.Visibility(this, &SClassPickerDialog::GetSelectButtonVisibility)
					.ContentPadding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
					.OnClicked_Lambda([&bResult, DialogWindow]()
					{
						bResult = true;
						DialogWindow->RequestDestroyWindow();
						return(FReply::Handled());
					})
					.ButtonStyle(MSS_STYLE::Get(), "FlatButton.Success")
					.TextStyle(MSS_STYLE::Get(), "FlatButton.DefaultTextStyle")
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("CopySocketsButtonCancel", "Cancel"))
					.HAlign(HAlign_Center)
					.ContentPadding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
					.OnClicked_Lambda([&bResult, DialogWindow]()
					{
						bResult = false;
						DialogWindow->RequestDestroyWindow();
						return(FReply::Handled());
					})
					.ButtonStyle(MSS_STYLE::Get(), "FlatButton.Default")
					.TextStyle(MSS_STYLE::Get(), "FlatButton.DefaultTextStyle")
				]
			]
		;



	DialogWindow->SetContent(xxx);

	GEditor->EditorAddModalWindow(DialogWindow);

	return bResult;
}

bool UCopyPasteSockets::ShowPasteDialog(const FString MeshName, bool & bDeleteExisting)
{
	bool bResult = false;

	TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("PasteSocketsTitle", "Paste Sockets"))
		.SizingRule(ESizingRule::Autosized)
		.ClientSize(FVector2D(0.f, 300.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SVerticalBox> xxx =

		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		.FillHeight(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("PasteSocketsConfirm", "Paste sockets to {0}?"), FText::FromString(MeshName)))
			.Font(MSS_STYLE::GetFontStyle("StandardDialog.LargeFont"))
		]
		+ SVerticalBox::Slot()
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		.AutoHeight()
		.VAlign(VAlign_Top)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PasteSocketsConfirmTip", "Tip: You can choose a particular static mesh component too."))
			.Font(MSS_STYLE::GetFontStyle("StandardDialog.SmallFont"))
		]

		+ SVerticalBox::Slot()
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		.AutoHeight()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PasteSocketEmpty", "Delete existing sockets"))
				.Font(MSS_STYLE::GetFontStyle("StandardDialog.SmallFont"))
			]
			.OnCheckStateChanged_Lambda([&bDeleteExisting](ECheckBoxState State) { bDeleteExisting = (State == ECheckBoxState::Checked); })
			.ToolTipText(LOCTEXT("PasteSocketEmptyTip", "Will delete all existing sockets from the mesh, othrwise only sockets with the same name will be overwritten"))
		]




	+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		[
			SNew(SUniformGridPanel)
			.SlotPadding(MSS_STYLE::GetMargin("StandardDialog.SlotPadding"))
		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("CopySocketsButtonOk", "Paste"))
		.HAlign(HAlign_Center)
		//.Visibility(this, &SClassPickerDialog::GetSelectButtonVisibility)
		.ContentPadding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		.OnClicked_Lambda([&bResult, DialogWindow]()
	{
		bResult = true;
		DialogWindow->RequestDestroyWindow();
		return(FReply::Handled());
	})
		.ButtonStyle(MSS_STYLE::Get(), "FlatButton.Success")
		.TextStyle(MSS_STYLE::Get(), "FlatButton.DefaultTextStyle")
		]
	+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("CopySocketsButtonCancel", "Cancel"))
		.HAlign(HAlign_Center)
		.ContentPadding(MSS_STYLE::GetMargin("StandardDialog.ContentPadding"))
		.OnClicked_Lambda([&bResult, DialogWindow]()
	{
		bResult = false;
		DialogWindow->RequestDestroyWindow();
		return(FReply::Handled());
	})
		.ButtonStyle(MSS_STYLE::Get(), "FlatButton.Default")
		.TextStyle(MSS_STYLE::Get(), "FlatButton.DefaultTextStyle")
		]
		]
	;



	DialogWindow->SetContent(xxx);

	GEditor->EditorAddModalWindow(DialogWindow);

	return bResult;
}

void UCopyPasteSockets::CopySockets()
{
	bool bPivotAtZero = false;
	bool bOpenOnly = false;
	FSnapSystemModule& MSS = FSnapSystemModule::Get();

	if (GEditor->GetSelectedActorCount() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CopySocketsNoSel", "Nothing to copy from. Please select some actors."));
		return;
	}

	if (ShowCopyDialog(bPivotAtZero, bOpenOnly))
	{
		TSet<FName> UniqueNames;
		UCopyPasteSockets* Dummy = NewObject<UCopyPasteSockets>();
		Dummy->CopyData.bPivotAtZero = bPivotAtZero;

		FTransform TZeroInv = FTransform::Identity;

		TSet<const AActor*> Selection;
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			if(AActor* A=Cast<AActor>(*It))
			{
				Selection.Add(A);

				if (!bPivotAtZero && It.GetIndex() == 0)
				{
					TZeroInv = A->GetActorTransform().Inverse();
				}
			}
			
		}



		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			if (AActor* Actor = Cast<AActor>(*It))
			{
				TArray<UActorComponent*> Comps;// = Actor->GetComponentsByClass(UActorComponent::StaticClass());

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
				Comps = Actor->GetComponentsByClass(UActorComponent::StaticClass());
#else
				Actor->GetComponents(Comps);
#endif

				for (UActorComponent* ActorComp : Comps)
				{
					if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(ActorComp))
					{
						TArray<FComponentSocketDescription> OutSockets;
						PrimitiveComp->QuerySupportedSockets(OutSockets);

						for (const FComponentSocketDescription& SocketDesc : OutSockets)
						{
							if (SocketDesc.Type == EComponentSocketType::Socket)
							{
								bool bTake = true;

								if(bOpenOnly)
								{
									FString Name2;
									FString Rest2;
									if (!SocketDesc.Name.ToString().Split("_", &Name2, &Rest2, ESearchCase::IgnoreCase, ESearchDir::FromStart))
									{
										Name2 = SocketDesc.Name.ToString();
									}

									FTransform T2 = PrimitiveComp->GetSocketTransform(SocketDesc.Name);

									bTake = MSS.CheckSocketOpen(FSnapSocket(PrimitiveComp, Name2, Rest2, T2, SocketDesc.Name), Selection, true);
								}

								if (bTake)
								{
									int32 Num = 0;
									FName NewName = SocketDesc.Name;
									while (UniqueNames.Contains(NewName))
									{
										NewName = FName(*(SocketDesc.Name.ToString() + TEXT("_") + FString::FromInt(Num)));
										Num++;
									}
									

									Dummy->CopyData.Sockets.Add(FExportSocket(NewName, PrimitiveComp->GetSocketTransform(SocketDesc.Name) * TZeroInv));
									UniqueNames.Add(NewName);
								}
							}
						}
					}
				}
			}
		}
	


		if (Dummy->CopyData.Sockets.Num() > 0)
		{ 
			FString Output;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 25 
			if (UProperty* Prop = UCopyPasteSockets::StaticClass()->FindPropertyByName(TEXT("CopyData")))
#else
			if (FProperty* Prop = UCopyPasteSockets::StaticClass()->FindPropertyByName(TEXT("CopyData")))
#endif
			{
				Prop->ExportText_Direct(Output, &Dummy->CopyData, nullptr, Dummy, (PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr);
			}

			FPlatformApplicationMisc::ClipboardCopy(*Output);
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CopySocketsNotFound", "Warning! No sockets found in selected actors." ));
		}
		

	}
}

void UCopyPasteSockets::PasteSockets()
{
	FString Input;
	UCopyPasteSockets* Dummy = NewObject<UCopyPasteSockets>();
	bool bDeleteExisting = false;

	FPlatformApplicationMisc::ClipboardPaste(Input);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 25 
	UProperty* Prop = UCopyPasteSockets::StaticClass()->FindPropertyByName(TEXT("CopyData"));
#else
	FProperty* Prop = UCopyPasteSockets::StaticClass()->FindPropertyByName(TEXT("CopyData"));
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
	Prop->ImportText_Direct(*Input, &Dummy->CopyData, nullptr, PPF_None); 
#else
	Prop->ImportText(*Input, &Dummy->CopyData, PPF_None, nullptr);
#endif
	if (Dummy->CopyData.Sockets.Num() != 0)
	{
		AActor* Actor = nullptr;
		UStaticMeshComponent* Comp = nullptr;
		UStaticMesh* CurrentStaticMesh = nullptr;

		if (GEditor->GetSelectedComponentCount() == 1)
		{
			Comp = Cast<UStaticMeshComponent>(GEditor->GetSelectedComponents()->GetTop<UActorComponent>());
		}

		if (GEditor->GetSelectedActorCount() == 1)
		{
			Actor = GEditor->GetSelectedActors()->GetTop<AActor>();
			if (!Comp)
			{
				Comp = Cast<UStaticMeshComponent>(Actor->GetRootComponent());
			}
		}

		if(Comp)
		{
			CurrentStaticMesh = Comp->GetStaticMesh();
		}

		if (Actor && Comp && CurrentStaticMesh)
		{
			if(ShowPasteDialog(CurrentStaticMesh->GetName(), bDeleteExisting))
//			if (FMessageDialog::Open(EAppMsgType::OkCancel, FText::Format(LOCTEXT("PasteSocketsConfirm", "Paste sockets to {0}?"), FText::FromString(CurrentStaticMesh->GetName()))) == EAppReturnType::Ok)
			{
				FTransform CTI = FTransform::Identity;

				if (Dummy->CopyData.bPivotAtZero)
				{
					CTI = Actor->GetActorTransform().Inverse();
				}

				const FScopedTransaction Transaction(LOCTEXT("PasteSockets", "Paste Sockets"));

				CurrentStaticMesh->PreEditChange(NULL);

				
				if(bDeleteExisting)
				{
					CurrentStaticMesh->Sockets.Empty();
				}
				else
				{
					for (FExportSocket& S : Dummy->CopyData.Sockets)
					{
						for (auto It = CurrentStaticMesh->Sockets.CreateIterator(); It; ++It)
						{
							if ((*It)->SocketName == S.Name)
							{
								It.RemoveCurrent();
							}
						}
					}
				}

				for (FExportSocket& S : Dummy->CopyData.Sockets)
				{
					UStaticMeshSocket* NewSocket = NewObject<UStaticMeshSocket>(CurrentStaticMesh);
					check(NewSocket);

					NewSocket->SocketName = S.Name;
					NewSocket->SetFlags(RF_Transactional);

					FTransform ST = S.T * CTI;

					NewSocket->RelativeLocation = ST.GetLocation();
					NewSocket->RelativeRotation = ST.GetRotation().Rotator();
					NewSocket->RelativeScale = ST.GetScale3D();

					

					CurrentStaticMesh->Sockets.Add(NewSocket);

				}
				CurrentStaticMesh->PostEditChange();
				CurrentStaticMesh->MarkPackageDirty();
			}			
			
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PasteSocketsNoSel", "No static mesh found. Please select a static mesh actor in the level or a static mesh component in an actor to paste sockets to."));

		}
		
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PasteSocketsNotFound", "No socket copy data found in the clipboard."));
	}

}

#undef LOCTEXT_NAMESPACE