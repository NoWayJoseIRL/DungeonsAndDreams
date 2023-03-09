// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.


#include "ExportImportSockets.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "EditorDirectories.h"
#include "Misc/FileHelper.h"

#include "Editor.h"

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
#include "Toolkits/AssetEditorManager.h"
#else
#include "Subsystems/AssetEditorSubsystem.h"
#endif

#include "Misc/MessageDialog.h"
#include "Misc/ScopedSlowTask.h"


#define LOCTEXT_NAMESPACE "FSnapSystemModule"

DEFINE_LOG_CATEGORY(LogMSSExport);

UExportImportSockets::UExportImportSockets(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UExportImportSockets::ExportSockets(TArray<FAssetData> SelectedAssets)
{
	//UE_LOG(LogMSSExport, Log, TEXT("UExportImportSockets::ExportSockets"));

	TArray<FString> Output;
	int32 SocketCount = 0;
	int32 MeshCount = 0;

	for (const FAssetData& Asset : SelectedAssets)
	{
		if (UStaticMesh* SM = Cast< UStaticMesh >(Asset.GetAsset()))
		{
			if (SM->Sockets.Num() > 0)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
				Output.Add(Asset.AssetClassPath.GetAssetName().ToString() + " " + Asset.PackageName.ToString());
#else
				Output.Add(Asset.AssetClass.ToString() + " " + Asset.PackageName.ToString());
#endif
				

				MeshCount++;
				for (const UStaticMeshSocket* S : SM->Sockets)
				{

					const FTransform T(S->RelativeRotation, S->RelativeLocation, S->RelativeScale);

					Output.Add(TEXT("S ") + S->SocketName.ToString() + " " + T.ToString());
					SocketCount++;
				}
			}

		}
	}

	if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
	{
		TArray<FString> SaveFilenames;


		bool bOpened = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			LOCTEXT("ExportSockets", "Export Sockets").ToString(),
			*(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT)),
			TEXT(""),
			TEXT("Text file|*.txt"),
			EFileDialogFlags::None,
			SaveFilenames
		);

		if (bOpened)
		{
			FString FileName = SaveFilenames[0];
			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, FPaths::GetPath(FileName)); // Save path as default for next time.

			if (FFileHelper::SaveStringArrayToFile(Output, *FileName))
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("Status_ExportSockets", "Exported {0} sockets from {1} meshes."), FText::AsNumber(SocketCount), FText::AsNumber(MeshCount)));
			}
			else
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("Status_ExportError", "Error writing to file {1}"), FText::FromString(FileName)));
			}
		}
	}

}
void UExportImportSockets::ImportSockets(TArray<FAssetData> SelectedAssets)
{

	UE_LOG(LogMSSExport, Log, TEXT("UExportImportSockets::ImportSockets"));

	const FText WarningMessage = LOCTEXT("Warning_ImportSockets", "Are you sure you want to import sockets into selected meshes? Existing sockets with the same name will be overwritten. There is no undo available (but you can close the editor without saving modified assets).");
	const FText OneCandidateMsg = LOCTEXT("Warning_OneCandidate", "Exact match not found for \"{0}\" import into \"{1}\" instead?");
	const FText TwoCandidateMsg = LOCTEXT("Warning_TwoCandidate", "Many candidates for {0}, import sockets into {1}?");

	if (FMessageDialog::Open(EAppMsgType::YesNo, WarningMessage) == EAppReturnType::Yes)
	{
		FScopedSlowTask SlowTask(1.0f, LOCTEXT("ImportingSockets", "Importing Sockets"));


		if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
		{
			TArray<FString> OpenFilenames;

			bool bOpened = DesktopPlatform->OpenFileDialog(
				FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
				LOCTEXT("ImportSockets", "Import Sockets from file").ToString(),
				*(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT)),
				TEXT(""),
				TEXT("Text file|*.txt"),
				EFileDialogFlags::None,
				OpenFilenames
			);


			if (bOpened)
			{
				FString& FileName = OpenFilenames[0];
				FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, FPaths::GetPath(FileName));

				TArray<FString> Input;
				TArray<FString> LineArray;

				int32 LineNum = 0;

				int32 ErrCount = 0;
				int32 MeshCount = 0;
				int32 SocketCount = 0;

				if (FFileHelper::LoadFileToStringArray(Input, *FileName))
				{
					UStaticMesh* CurrentStaticMesh = NULL;
					bool bMeshModified = false;
					EAppReturnType::Type OneCandAnswer = EAppReturnType::Retry;

					for (const FString& Line : Input)
					{
						LineNum++;

						int32 Len = Line.ParseIntoArray(LineArray, TEXT(" "));


						if (Len > 0)
						{
							if (LineArray[0] == TEXT("StaticMesh"))
							{
								if (Len > 1)
								{
									if (CurrentStaticMesh && bMeshModified)
									{
										//save previous mesh


#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
										{
											FAssetEditorManager& AssetEditorManager = FAssetEditorManager::Get();
											AssetEditorManager.CloseAllEditorsForAsset(CurrentStaticMesh);
										}
#else
										if (GEditor && GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
										{
											GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(CurrentStaticMesh);
										}
#endif

										CurrentStaticMesh->PostEditChange();
										CurrentStaticMesh->MarkPackageDirty();

										MeshCount++;
									}

									

									CurrentStaticMesh = NULL;
									bMeshModified = false;

									//const FName CurrentMeshName(*LineArray[1]);
									const FString CurMeshS = LineArray[1];

									int32 MaxSufLen = 0;
									FAssetData AssetCandidate;
									int32 NumCandidates = 0;
									bool Equals = false;

									//load the next mesh
									for (const FAssetData& Asset : SelectedAssets)
									{

										int32 SufLen = CommonSuffix(CurMeshS, Asset.PackageName.ToString(), Equals);
										//UE_LOG(LogMSSExport, Log, TEXT("Common suffix %d %s"), SufLen, Equals ? TEXT("Equals") : TEXT(""));

										if (Equals)
										{
											//exact path match
											CurrentStaticMesh = Cast< UStaticMesh >(Asset.GetAsset());
											
											AssetCandidate = Asset;
											UE_LOG(LogMSSExport, Log, TEXT("Importing sockets into: %s"), *CurMeshS);
											break;
										}
										//same asset name
										else if(SufLen > 0)
										{
											
											if (SufLen > MaxSufLen)
											{
												//find best candidate: same name and bigger path length
												MaxSufLen = SufLen;;
												AssetCandidate = Asset;
												NumCandidates = 1;
											}
											else if (SufLen == MaxSufLen)
											{
												//same name and same matching path length 
												NumCandidates++;
											}
										}

									}

									//exact match not found, but assets with same name exist
									if (!Equals && NumCandidates > 0)
									{
										if (NumCandidates == 1)
										{
											EAppReturnType::Type Ans = OneCandAnswer;

											if (Ans == EAppReturnType::Retry)
											{
												//asking
												Ans = FMessageDialog::Open(EAppMsgType::YesNoYesAllNoAllCancel, FText::Format(OneCandidateMsg, FText::FromString(CurMeshS), FText::FromName(AssetCandidate.PackageName)));

												if (Ans == EAppReturnType::YesAll)
												{
													OneCandAnswer = Ans = EAppReturnType::Yes;
												}
												else if (Ans == EAppReturnType::NoAll)
												{
													OneCandAnswer = Ans = EAppReturnType::No;
												}
												else if (Ans == EAppReturnType::Cancel)
												{
													OneCandAnswer = EAppReturnType::Cancel;
													break;
												}
											}

											if (Ans == EAppReturnType::Yes)
											{
												CurrentStaticMesh = Cast< UStaticMesh >(AssetCandidate.GetAsset());
												SelectedAssets.Remove(AssetCandidate);
												UE_LOG(LogMSSExport, Log, TEXT("Importing sockets into: %s (from: %s)"), *AssetCandidate.PackageName.ToString(), *CurMeshS);

											}
										}
										else
										{
											UE_LOG(LogMSSExport, Error, TEXT("Line %d: %s. Error: there are multiple selected assets with the same name. Cannot find unique best candidate for importing. Skipping."), LineNum, *CurMeshS);
											ErrCount++;
										}
									}
									else if (Equals)
									{
										SelectedAssets.Remove(AssetCandidate);
									}
								}
								else
								{
									UE_LOG(LogMSSExport, Error, TEXT("Line %d: Mesh name is missing"), LineNum);
									ErrCount++;
								}
							}

							else if (CurrentStaticMesh && LineArray[0] == TEXT("S"))
							{
								if (Len > 2)
								{
									FTransform T;
									if (T.InitFromString(LineArray[2]))
									{


										FName SocketName(*LineArray[1]);

										if (UStaticMeshSocket* OldSocket = CurrentStaticMesh->FindSocket(SocketName))
										{
											OldSocket->RelativeLocation = T.GetLocation();
											OldSocket->RelativeRotation = T.GetRotation().Rotator();
											OldSocket->RelativeScale = T.GetScale3D();

											UE_LOG(LogMSSExport, Log, TEXT("   Modifying Socket: %s %s"), *LineArray[1], *T.ToString());
										}
										else
										{
											UStaticMeshSocket* NewSocket = NewObject<UStaticMeshSocket>(CurrentStaticMesh);
											NewSocket->SocketName = FName(*LineArray[1]);
											NewSocket->RelativeLocation = T.GetLocation();
											NewSocket->RelativeRotation = T.GetRotation().Rotator();
											NewSocket->RelativeScale = T.GetScale3D();

											CurrentStaticMesh->Sockets.Add(NewSocket);
											UE_LOG(LogMSSExport, Log, TEXT("   Adding Socket: %s %s"), *LineArray[1], *T.ToString());

										}
										SocketCount++;
										
										bMeshModified = true;
											
										
									}
									else
									{
										UE_LOG(LogMSSExport, Error, TEXT("Line %d: Cannot parse transform %s"), LineNum, *LineArray[2]);
										ErrCount++;
									}
								}
								else
								{
									UE_LOG(LogMSSExport, Error, TEXT("Line %d: Incorrect socket definition"), LineNum, *LineArray[2]);
									ErrCount++;
								}
							}
						}

					}

					if (CurrentStaticMesh && bMeshModified)
					{
						//save last mesh modified mesh


#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
						{
							FAssetEditorManager& AssetEditorManager = FAssetEditorManager::Get();
							AssetEditorManager.CloseAllEditorsForAsset(CurrentStaticMesh);
						}
#else
						if (GEditor && GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
						{
							GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(CurrentStaticMesh);
						}
#endif

						CurrentStaticMesh->PostEditChange();
						CurrentStaticMesh->MarkPackageDirty();

						MeshCount++;
					}
				}
				else
				{
					UE_LOG(LogMSSExport, Error, TEXT("Error reading file %s"), *FileName);
					ErrCount++;
				}



				if (ErrCount == 0)
				{
					FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("Status_ImportSockets", "Imported {0} sockets into {1} meshes. No errors. See Output Log for details."), FText::AsNumber(SocketCount), FText::AsNumber(MeshCount)));
				}
				else
				{
					FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("Status_ImportSockets", "Imported {0} sockets into {1} meshes with {2} errors. See Output Log for details and errors."), FText::AsNumber(SocketCount), FText::AsNumber(MeshCount), FText::AsNumber(ErrCount)));
				}
			}
		}
	}
}

void UExportImportSockets::DeleteSockets(TArray<FAssetData> SelectedAssets)
{
	UE_LOG(LogMSSExport, Log, TEXT("UExportImportSockets::DeleteSockets"));

	FText WarningMessage = LOCTEXT("Warning_DeleteSockets", "Are you sure you want to delete sockets from all selected meshes?  There is no undo available (but you can close the editor without saving modified assets).");
	if (FMessageDialog::Open(EAppMsgType::YesNo, WarningMessage) == EAppReturnType::Yes)
	{
		FScopedSlowTask SlowTask(1.0f, LOCTEXT("DeletingSockets", "Deleting Sockets"));

		for (const FAssetData& Asset : SelectedAssets)
		{
			if (UStaticMesh* CurrentStaticMesh = Cast< UStaticMesh >(Asset.GetAsset()))
			{
				if (CurrentStaticMesh->Sockets.Num() > 0)
				{
					CurrentStaticMesh->Sockets.Empty();

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
					{	
						FAssetEditorManager& AssetEditorManager = FAssetEditorManager::Get();
						AssetEditorManager.CloseAllEditorsForAsset(CurrentStaticMesh);
					}
#else
					if (GEditor && GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
					{
						GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(CurrentStaticMesh);
					}
#endif
					CurrentStaticMesh->PostEditChange();
					CurrentStaticMesh->MarkPackageDirty();
				}
			}
		}
	}
}

//returns the number of matching path elements counting from the end.
int32 UExportImportSockets::CommonSuffix(const FString A, const FString B, bool& Equals)
{
	Equals = false;

	TArray<FString> AP, BP;

	A.Reverse().ParseIntoArray(AP, TEXT("/"));
	B.Reverse().ParseIntoArray(BP, TEXT("/"));

	int32 MinL = AP.Num() > BP.Num() ? BP.Num() : AP.Num();

	for (int32 i = 0; i < MinL; i++)
	{
		if (AP[i] != BP[i])
		{
			return i;
		}
	}

	Equals = (AP.Num() == BP.Num());

	return MinL;
}

#undef LOCTEXT_NAMESPACE