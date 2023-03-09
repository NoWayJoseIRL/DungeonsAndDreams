// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "CopyPasteSockets.generated.h"

USTRUCT()
struct FExportSocket
{
	GENERATED_BODY()

		FExportSocket() {};

	FExportSocket(FName Name, FTransform T) :
		Name(Name),
		T(T) {};

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FTransform T;
};

USTRUCT()
struct FCopySockets
{
	GENERATED_BODY()

	UPROPERTY()
	bool bPivotAtZero;

	UPROPERTY()
	TArray<FExportSocket> Sockets;
};

/**
 * 
 */
UCLASS()
class SNAPSYSTEM_API UCopyPasteSockets : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	static void CopySockets();
	static void PasteSockets();
	static bool ShowCopyDialog(bool& bPivotAtZero, bool& bOpneOnly);
	static bool ShowPasteDialog(const FString MeshName, bool& bDeleteExisting);


private:


	UPROPERTY()
	FCopySockets CopyData;
};
