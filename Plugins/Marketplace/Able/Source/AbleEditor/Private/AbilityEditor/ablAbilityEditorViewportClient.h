// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "EditorViewportClient.h"
#include "PreviewScene.h"
#include "UnrealWidgetFwd.h"
#include "UnrealWidget.h"

class FAblAbilityEditor;
class SAbilityEditorViewport;
class UExponentialHeightFogComponent;
class UStaticMeshComponent;

class FAbilityEditorPreviewScene : public FPreviewScene
{
public:
	FAbilityEditorPreviewScene(ConstructionValues CVS);
};

class FAbilityEditorViewportClient : public FEditorViewportClient
{
public:
	FAbilityEditorViewportClient(FAbilityEditorPreviewScene& InPreviewScene, TWeakPtr<FAblAbilityEditor> InAbilityEditor, const TSharedRef<SAbilityEditorViewport>& InAbilityEditorViewport);
	virtual ~FAbilityEditorViewportClient();

	virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = false) override;
	virtual bool InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;

	virtual void ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;

	virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
	virtual void TrackingStopped() override;

	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	virtual FVector GetWidgetLocation() const override;

	FTransform GetWorldPositionForMouse(int32 MouseX, int32 MouseY);
	const TArray<TWeakObjectPtr<AActor>>& GetSelectedActors() const { return m_SelectedActors; }
	bool HasSelectedSpawnActors() const;
	
	void CaptureThumbnail();
	void TickWorld(float DeltaSeconds);
private:
	bool m_CaptureThumbnail;
	TWeakPtr<FAblAbilityEditor> m_AbilityEditor;

	FVector m_SpawnActorWidgetLocation;
	TArray<TWeakObjectPtr<AActor>> m_SelectedActors;
};