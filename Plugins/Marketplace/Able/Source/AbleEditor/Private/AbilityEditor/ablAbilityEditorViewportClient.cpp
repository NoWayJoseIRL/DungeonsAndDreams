// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/ablAbilityEditorViewportClient.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbilityEditor/SAbilityViewport.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SphereReflectionCaptureComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "Editor/UnrealEd/Private/Editor/ActorPositioning.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
#include "SnappingUtils.h"
#include "UObject/UObjectIterator.h"

#include "Engine/StaticMesh.h"
#include "Runtime/Engine/Classes/Materials/Material.h"

#include "ImageUtils.h"
#include "ObjectTools.h"
#include "UnrealEdGlobals.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "ThumbnailRendering/ThumbnailManager.h"

FAbilityEditorPreviewScene::FAbilityEditorPreviewScene(ConstructionValues CVS)
	: FPreviewScene(CVS)
{

}

FAbilityEditorViewportClient::FAbilityEditorViewportClient(FAbilityEditorPreviewScene& InPreviewScene, TWeakPtr<FAblAbilityEditor> InAbilityEditor, const TSharedRef<SAbilityEditorViewport>& InAbilityEditorViewport)
	: FEditorViewportClient(nullptr, &InPreviewScene, StaticCastSharedRef<SEditorViewport>(InAbilityEditorViewport)),
	m_CaptureThumbnail(false)
{
	check(InAbilityEditor.IsValid());
	m_AbilityEditor = InAbilityEditor;

	ViewFOV = FMath::Clamp(m_AbilityEditor.Pin()->GetEditorSettings().m_FOV, 70.0f, 180.0f);

	// DrawHelper set up
	DrawHelper.PerspectiveGridSize = HALF_WORLD_MAX1;
	DrawHelper.AxesLineThickness = 0.0f;
	DrawHelper.bDrawGrid = true;

	EngineShowFlags.Game = 0;
	EngineShowFlags.ScreenSpaceReflections = 1;
	EngineShowFlags.AmbientOcclusion = 1;
	EngineShowFlags.SetSnap(0);

	SetRealtime(true);

	//EngineShowFlags.DisableAdvancedFeatures();
	//EngineShowFlags.SetSeparateTranslucency(true);
	//EngineShowFlags.SetCompositeEditorPrimitives(true);
	EngineShowFlags.SetParticles(true);

	if (UWorld* PreviewWorld = InPreviewScene.GetWorld())
	{
		PreviewWorld->bAllowAudioPlayback = !m_AbilityEditor.Pin()->GetEditorSettings().m_MuteAudio;
	}

	// now add floor
	UStaticMeshComponent* EditorFloorComp = NewObject<UStaticMeshComponent>(GetTransientPackage(), TEXT("EditorFloorComp"));

	UStaticMesh* FloorMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/PhAT_FloorBox.PhAT_FloorBox"), NULL, LOAD_None, NULL);
	if (ensure(FloorMesh))
	{
		EditorFloorComp->SetStaticMesh(FloorMesh);
	}

	UMaterial* Material = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/PersonaFloorMat.PersonaFloorMat"), NULL, LOAD_None, NULL);
	if (ensure(Material))
	{
		EditorFloorComp->SetMaterial(0, Material);
	}

	EditorFloorComp->SetRelativeScale3D(FVector(6.f, 6.f, 1.f));
	PreviewScene->AddComponent(EditorFloorComp, FTransform::Identity);

	// Turn off so that actors added to the world do not have a lifespan (so they will not auto-destroy themselves).
	PreviewScene->GetWorld()->bBegunPlay = false;

	PreviewScene->SetSkyCubemap(GUnrealEd->GetThumbnailManager()->AmbientCubemap);

}

FAbilityEditorViewportClient::~FAbilityEditorViewportClient()
{

}

void FAbilityEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas)
{
	FEditorViewportClient::Draw(InViewport, Canvas);
	
	if (m_CaptureThumbnail)
	{
		if (UAblAbility* Ability = m_AbilityEditor.Pin()->GetAbility())
		{
			int32 SrcWidth = InViewport->GetSizeXY().X;
			int32 SrcHeight = InViewport->GetSizeXY().Y;

			// Read the contents of the viewport into an array.
			TArray<FColor> OrigBitmap;
			if (InViewport->ReadPixels(OrigBitmap))
			{
				check(OrigBitmap.Num() == SrcWidth * SrcHeight);

				//pin to smallest value
				int32 CropSize = FMath::Min<uint32>(SrcWidth, SrcHeight);
				//pin to max size
				int32 ScaledSize = FMath::Min<uint32>(ThumbnailTools::DefaultThumbnailSize, CropSize);

				//calculations for cropping
				TArray<FColor> CroppedBitmap;
				CroppedBitmap.AddUninitialized(CropSize*CropSize);
				//Crop the image
				int32 CroppedSrcTop = (SrcHeight - CropSize) / 2;
				int32 CroppedSrcLeft = (SrcWidth - CropSize) / 2;
				for (int32 Row = 0; Row < CropSize; ++Row)
				{
					//Row*Side of a row*byte per color
					int32 SrcPixelIndex = (CroppedSrcTop + Row)*SrcWidth + CroppedSrcLeft;
					const void* SrcPtr = &(OrigBitmap[SrcPixelIndex]);
					void* DstPtr = &(CroppedBitmap[Row*CropSize]);
					FMemory::Memcpy(DstPtr, SrcPtr, CropSize * 4);
				}
				
				//Scale image down if needed
				TArray<FColor> ScaledBitmap;
				if (ScaledSize < CropSize)
				{
					FImageUtils::ImageResize(CropSize, CropSize, CroppedBitmap, ScaledSize, ScaledSize, ScaledBitmap, true);
				}
				else
				{
					//just copy the data over. sizes are the same
					ScaledBitmap = CroppedBitmap;
				}

				// Compress.
				FCreateTexture2DParameters Params;
				Params.bDeferCompression = true;
				Ability->ThumbnailImage = FImageUtils::CreateTexture2D(ScaledSize, ScaledSize, ScaledBitmap, Ability, TEXT("ThumbnailTexture"), RF_NoFlags, Params);
				Ability->MarkPackageDirty();
			}
		}

		m_CaptureThumbnail = false;
	}
}

void FAbilityEditorViewportClient::Tick(float DeltaSeconds)
{
	TSharedPtr<FAblAbilityEditor> AbilityEditor = m_AbilityEditor.IsValid() ? m_AbilityEditor.Pin() : nullptr;
	
	if (AbilityEditor.IsValid())
	{
		float FOV = FMath::Clamp(AbilityEditor->GetEditorSettings().m_FOV, 70.0f, 180.0f);

		// See if we need to update our FOV. 
		if (!FMath::IsNearlyEqual(FOV, ViewFOV))
		{
			ViewFOV = FOV;
			Invalidate();
		}

		PreviewScene->GetWorld()->bAllowAudioPlayback = !AbilityEditor->GetEditorSettings().m_MuteAudio;

		FEditorViewportClient::Tick(DeltaSeconds);

		if (!AbilityEditor->IsPaused())
		{
			TickWorld(DeltaSeconds);
		}
	}
	else
	{
		FEditorViewportClient::Tick(DeltaSeconds);
	}
}

bool FAbilityEditorViewportClient::InputKey(FViewport* InViewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed /*= 1.f*/, bool bGamepad /*= false*/)
{
	bool handled = false;

	if (Key == EKeys::Escape)
	{
		GEditor->SelectNone(false, true, false);
		handled = true;
	}

	if (Key == EKeys::Delete && HasSelectedSpawnActors())
	{
		TArray<AActor*> ActorsToDelete;
		const FScopedTransaction Delta(NSLOCTEXT("UnrealEd", "Actor Movement", "Actor Movement"));
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			AActor* SelectedActor = Cast<AActor>(*It);
			if (m_AbilityEditor.Pin()->GetAbilityPreviewTargets().Find(SelectedActor) != INDEX_NONE)
			{
				ActorsToDelete.Add(SelectedActor);
			}
		}

		if (ActorsToDelete.Num())
		{
			for (AActor* ToDelete : ActorsToDelete)
			{
				GEditor->SelectActor(ToDelete, false, false);
				m_AbilityEditor.Pin()->DeleteAbilityPreviewTarget(ToDelete);
			}
			handled = true;
		}

	}

	FInputKeyEventArgs eventArgs(InViewport, ControllerId, Key, Event, AmountDepressed, bGamepad);
	return FEditorViewportClient::InputKey(eventArgs) || handled;
}

bool FAbilityEditorViewportClient::InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
	bool handled = false;
	if (HasSelectedSpawnActors() && CurrentAxis != EAxisList::None)
	{
		const FScopedTransaction Delta(NSLOCTEXT("UnrealEd", "Actor Movement", "Actor Movement"));
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			if (AActor* SelectedActor = Cast<AActor>(*It))
			{
				GEditor->ApplyDeltaToActor(SelectedActor, true, &Drag, &Rot, &Scale);
				handled = true;
			}
		}

		return handled;
	}

	return FEditorViewportClient::InputWidgetDelta(InViewport, CurrentAxis, Drag, Rot, Scale) || handled;
}

void FAbilityEditorViewportClient::ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	if (HitProxy && HitProxy->IsA(HActor::StaticGetType()))
	{
		HActor* ActorProxy = (HActor*)HitProxy;
		if (m_AbilityEditor.IsValid())
		{
			AActor* PreviewActor = m_AbilityEditor.Pin()->GetAbilityPreviewActor();
			
			if (PreviewActor == ActorProxy->Actor || m_AbilityEditor.Pin()->GetAbilityPreviewTargets().Find(ActorProxy->Actor) != INDEX_NONE)
			{
				GEditor->SelectNone(false, true, false);
				GEditor->SelectActor(ActorProxy->Actor, true, true, true);
				SetWidgetMode(UE::Widget::WM_Translate);
				return;
			}
		}
	}

	FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
}

void FAbilityEditorViewportClient::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge)
{
	if (HasSelectedSpawnActors())
	{
		GEditor->DisableDeltaModification(true);
	}

	FEditorViewportClient::TrackingStarted(InInputState, bIsDragging, bNudge);
}

void FAbilityEditorViewportClient::TrackingStopped()
{
	if (HasSelectedSpawnActors())
	{
		GEditor->DisableDeltaModification(false);
	}

	FEditorViewportClient::TrackingStopped();
}

UE::Widget::EWidgetMode FAbilityEditorViewportClient::GetWidgetMode() const
{
	if (!HasSelectedSpawnActors())
	{
		return UE::Widget::WM_None;
	}

	return FEditorViewportClient::GetWidgetMode();
}

FVector FAbilityEditorViewportClient::GetWidgetLocation() const
{
	if (HasSelectedSpawnActors())
	{
		FVector WidgetLocation = FVector::ZeroVector;
		int32 TotalSelectedActors = 0;
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			if (AActor* SelectedActor = Cast<AActor>(*It))
			{
				WidgetLocation += SelectedActor->GetActorLocation();
				++TotalSelectedActors;
			}
		}

		if (TotalSelectedActors != 0)
		{
			WidgetLocation *= (1.0f / (float)TotalSelectedActors);
		}

		return WidgetLocation;
	}

	return FEditorViewportClient::GetWidgetLocation();
}

FTransform FAbilityEditorViewportClient::GetWorldPositionForMouse(int32 MouseX, int32 MouseY)
{
	Viewport->InvalidateHitProxy();

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		Viewport,
		GetScene(),
		EngineShowFlags)
		.SetRealtimeUpdate(IsRealtime()));
	FSceneView* View = CalcSceneView(&ViewFamily);
	FViewportCursorLocation Cursor(View, this, MouseX, MouseY);

	HHitProxy* HitProxy = Viewport->GetHitProxy(Cursor.GetCursorPos().X, Cursor.GetCursorPos().Y);

	const FActorPositionTraceResult TraceResult = FActorPositioning::TraceWorldForPositionWithDefault(Cursor, *View);

	GEditor->UnsnappedClickLocation = TraceResult.Location;
	GEditor->ClickLocation = TraceResult.Location;
	GEditor->ClickPlane = FPlane(TraceResult.Location, TraceResult.SurfaceNormal);

	// Snap the new location if snapping is enabled
	FSnappingUtils::SnapPointToGrid(GEditor->ClickLocation, FVector::ZeroVector);

	return FTransform(GEditor->ClickLocation);
}

bool FAbilityEditorViewportClient::HasSelectedSpawnActors() const
{
	if (m_AbilityEditor.IsValid())
	{
		AActor* PreviewActor = m_AbilityEditor.Pin()->GetAbilityPreviewActor();
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			if (PreviewActor == (Cast<AActor>(*It)) || m_AbilityEditor.Pin()->GetAbilityPreviewTargets().Find(Cast<AActor>(*It)) != INDEX_NONE)
			{
				return true;
			}
		}
	}

	return false;
}

void FAbilityEditorViewportClient::CaptureThumbnail()
{
	if (m_AbilityEditor.IsValid())
	{
		m_CaptureThumbnail = true;
	}
}

void FAbilityEditorViewportClient::TickWorld(float DeltaSeconds)
{
	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread)
	{
		// Allow full tick only if preview simulation is enabled and we're not currently in an active SIE or PIE session
		if (GEditor->PlayWorld == NULL && !GEditor->bIsSimulatingInEditor)
		{
			PreviewScene->GetWorld()->Tick(IsRealtime() ? LEVELTICK_All : LEVELTICK_TimeOnly, DeltaSeconds);
		}
		else
		{
			PreviewScene->GetWorld()->Tick(IsRealtime() ? LEVELTICK_ViewportsOnly : LEVELTICK_TimeOnly, DeltaSeconds);
		}
	}

	//PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
}
