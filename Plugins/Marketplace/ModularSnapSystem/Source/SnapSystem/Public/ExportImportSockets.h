// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AssetRegistry/AssetData.h"
#include "ExportImportSockets.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogMSSExport, Log, All);

/**
 * 
 */
UCLASS()
class SNAPSYSTEM_API UExportImportSockets : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	static void ExportSockets(TArray<FAssetData> SelectedAssets);
	static void ImportSockets(TArray<FAssetData> SelectedAssets);
	static void DeleteSockets(TArray<FAssetData> SelectedAssets);

private:
	static int32 CommonSuffix(const FString A, const FString B, bool& Equals);
};
