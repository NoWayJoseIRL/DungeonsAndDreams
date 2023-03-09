// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Components/SplineComponent.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Engine/EngineTypes.h"



class FToolBarBuilder;
class FMenuBuilder;

struct FSnapParameters
{
	bool bSnap;
	float SnapAngle;
	FVector SnapAngles;
	bool bScale;
	FVector SnapScale;
	bool bAttach;



	FSnapParameters()
	{
		bSnap = false;
		SnapAngle = 0.0f;
		SnapAngles = FVector(0.0f, 0.0f, 0.0f);
		bScale = false;
		SnapScale = FVector(0.0f, 0.0f, 0.0f);
		bAttach = false;

	}

	void SetMerge(const FSnapParameters& OtherParams, const FSnapParameters& MyParams)
	{
		if (OtherParams.bSnap)
		{
			bSnap = OtherParams.bSnap;
			SnapAngle = OtherParams.SnapAngle;
			SnapAngles = OtherParams.SnapAngles;
		}
		else if (MyParams.bSnap)
		{
			bSnap = MyParams.bSnap;
			SnapAngle = MyParams.SnapAngle;
			SnapAngles = MyParams.SnapAngles;
		}

		if (OtherParams.bScale)
		{
			bScale = OtherParams.bScale;
			SnapScale = OtherParams.SnapScale;
		}
		else if (MyParams.bScale)
		{
			bSnap = MyParams.bSnap;
			SnapScale = MyParams.SnapScale;
		}

		if (OtherParams.bAttach)
		{
			bAttach = true;
		}
	}
};

struct FSnapSocket
{
	FSnapSocket() :
		Comp(nullptr) {};
		
	FSnapSocket(UPrimitiveComponent* Comp, FString Name, FString Rest, FTransform T, FName FullName, int32 Index=0) :
		Name(Name),
		Rest(Rest),
		T(T),
		Comp(Comp),
		FullName(FullName),
		bOpen(true),
		Index(Index)
	{};

	FString Name;
	FString Rest;
	FTransform T;
	TWeakObjectPtr<UPrimitiveComponent> Comp;
	FName FullName;

	bool bOpen;

	//Instance index for instanced static meshes
	int32 Index;

	bool operator==(const FSnapSocket& Other) const
	{
		return FullName == Other.FullName && Comp == Other.Comp && Index == Other.Index;
	}


	friend inline uint32 GetTypeHash(const FSnapSocket& Key)
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(Key.FullName));
		Hash = HashCombine(Hash, GetTypeHash(Key.Comp));
		Hash = HashCombine(Hash, GetTypeHash(Key.Index));
		return Hash;
	}
};




class FSnapSystemModule : public IModuleInterface
{
public:
	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline FSnapSystemModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FSnapSystemModule >("SnapSystem");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command. */
	void PluginButtonClicked();
	FText GetSearchDistLabel() const;
	TSharedRef<SWidget> FillSnapMenu();
	void OptionsButtonClicked();
	void PluginShowSockets();

	TSharedRef<FExtender> OnExtendLevelEditorViewMenu(const TSharedRef<FUICommandList> CommandList);


	void PluginMoveStarted(UObject& InObject);
	void PluginObjectMoved(UObject& InObject);
	void PluginActorAdded(AActor* InObject);
	void PluginActorMoved(AActor* InObject);
	void PluginSelectionChanged(UObject* InObject);

	bool IsSnapEnabled() const;
	bool IsSocketsShown() const;

	void SnapSplinePoint(USplineComponent* SplineComp, int32 PointIndex, FTransform PrevT);
	bool CheckSocketOpen(const FSnapSocket & S, const TSet<const AActor*>& ActorsToConsider, const bool bInverse);


	static const FName NAME_NoSnap;

private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	ECheckBoxState IsSnapChecked() const;
	void HandleToggleSnap(ECheckBoxState InState);
	void AddMenuExtension(FMenuBuilder& Builder);


	void SnapSel(/*UObject & InObject*/);
	bool SearchSel(FSnapSocket& MyS, FSnapSocket& OtherS, TSet<const AActor*>& Attached);
	bool GetAllSockets(TArray<FSnapSocket>& Sockets,  UActorComponent * ActorComp);
	void SnapSelComp();
	bool SearchSelComp(FSnapSocket& MyS, FSnapSocket& OtherS, TArray<const AActor*>& Attached);
	bool MatchName(const FString& Name1, const FString& Name2)const;
	//bool SphereOverlapComponents(UObject* WorldContextObject, const FVector SpherePos, float SphereRadius,  UClass* ComponentClassFilter, const TSet<AActor*>& ActorsToIgnore, const TArray<UPrimitiveComponent*>& ComponentsToIgnore, TArray<UPrimitiveComponent*>& OutComponents);
	bool SphereOverlapComponents(UObject* WorldContextObject, const FVector SpherePos, float SphereRadius, /*UClass* ComponentClassFilter, */const TSet<const AActor*>& ActorsToIgnore, const TArray<UPrimitiveComponent*>& ComponentsToIgnore, TArray<FOverlapResult>& Overlaps/*TArray<UPrimitiveComponent*>& OutComponents*/);

	bool SocketParams(const FString& Rest, FSnapParameters& Params);

	void GetAllAttachedActors(AActor* Actor, TSet<const AActor*>& Attached) const;

	bool SearchSpline(USplineComponent* MyComp, const int32 PointIndex, FSnapSocket& MyS, FSnapSocket& OtherS);
	bool SearchSocket(const FSnapSocket& S, float& FoundDist, const TSet<const AActor*>& ActorsToIgnore, const TArray<UPrimitiveComponent*>& ComponentsToIgnore, FSnapSocket& MyS, FSnapSocket& OtherS);

	void MakeParams(FSnapParameters& Params, const FSnapSocket& MyS, const FSnapSocket& OtherS);

	void UpdateTransform(const FSnapParameters & Params, FTransform & T, bool bIgnoreScale=false);

	bool SkipSnap();

	bool EqualsTransform(const FTransform & T1, const FTransform & T2);

	FTransform GetSocketTransform(const FSnapSocket& S);
	FTransform GetSocketTransform(UPrimitiveComponent* Comp, FName SocketName, int32 InstanceIndex);

	void CopySockets();
	void PasteSockets();


protected:
	void SetPivot(FTransform Pivot, bool bRotate) const;


private:
	TSharedPtr<class FUICommandList> PluginCommands;

	bool bMoveStarted;
	bool bActorAdded;

	FTransform SavedT;
	FTransform RelativeT;

	TSet<FSnapSocket> SocketCache;
	TSet<FSnapSocket> SplineSocketCache;

	FDelegateHandle ContentBrowserExtenderDelegateHandle;

};
