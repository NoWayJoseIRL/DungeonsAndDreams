// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"
#include "Components/SplineComponent.h"
#include "ComponentVisualizer.h"

#include "SnapSystem.h"

class FMSSModeEdMode : public FEdMode
{
public:
	const static FEditorModeID EM_MSSModeEdModeId;
public:
	FMSSModeEdMode();
	virtual ~FMSSModeEdMode();

	// FEdMode interface
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override;

	virtual void Enter() override;
	virtual void Exit() override;

	//virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override { return false; }


	bool UsesToolkits() const override;
	virtual bool UsesPropertyWidgets() const { return true; }



	virtual void ActorSelectionChangeNotify() override;
	virtual void ActorsDuplicatedNotify(TArray<AActor*>& PreDuplicateSelection, TArray<AActor*>& PostDuplicateSelection, bool bOffsetLocations) override;

	//virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData) override;
	// End of FEdMode interface

	void SetPivot(FTransform& Pivot);
	void ClearPivot();

	bool IsSplineEdited() { return bSplineIsEdited; };
	bool IsSplineEndpointEdited(USplineComponent*& SplineComp, int32& PointIndex);

private:
	FMatrix CustomPivot;
	bool bCustomPivot;

	FSplineCurves SplineCurves;
	bool bSplineIsEdited;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
	FComponentVisualizer::FPropertyNameAndIndex SplineCompPropName;
#else
	FComponentPropertyPath SplineCompPropPath;
#endif

	TWeakObjectPtr<AActor> SplineOwningActor;


};
