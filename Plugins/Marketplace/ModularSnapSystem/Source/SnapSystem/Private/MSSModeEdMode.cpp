// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#include "MSSModeEdMode.h"
#include "MSSModeEdModeToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"

#include "LevelEditor.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "Editor/GroupActor.h"
#include "Engine/Selection.h"
#include "EditorViewportClient.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Materials/Material.h"

#include "HitProxies.h"
#include "Components/SplineComponent.h"

#include "USnapSystemSettings.h"
#include "Components/InstancedStaticMeshComponent.h"


const FEditorModeID FMSSModeEdMode::EM_MSSModeEdModeId = TEXT("EM_MSSModeEdMode");

FMSSModeEdMode::FMSSModeEdMode()
	:CustomPivot(FMatrix::Identity), 
	bCustomPivot(false),
	bSplineIsEdited(false)

{

}

FMSSModeEdMode::~FMSSModeEdMode()
{

}

bool FMSSModeEdMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	return true;// OtherModeID != FBuiltinEditorModes::EM_None;
}

void FMSSModeEdMode::Enter()
{
	FEdMode::Enter();

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FMSSModeEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}
}

void FMSSModeEdMode::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

/**
* A hit proxy class for sockets in the main editor viewports.
*/
struct HTestSocketProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	AActor* Actor;
	USceneComponent* SceneComponent;
	FName SocketName;

	HTestSocketProxy(AActor* InActor, USceneComponent* InSceneComponent, FName InSocketName)
		: HHitProxy(HPP_UI)
		, Actor(InActor)
		, SceneComponent(InSceneComponent)
		, SocketName(InSocketName)
	{}
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(Actor);
		Collector.AddReferencedObject(SceneComponent);
	}
};
IMPLEMENT_HIT_PROXY(HTestSocketProxy, HHitProxy);

void FMSSModeEdMode::Render(const FSceneView * View, FViewport * Viewport, FPrimitiveDrawInterface * PDI)
{
	const bool bUseNoSnapTag = GetDefault<UUSnapSystemSettings>()->bUseNoSnapTag;

	//FEdMode::Render(View, Viewport, PDI);
	
	if (!Viewport->GetClient()->IsInGameView() && GetDefault<UUSnapSystemSettings>()->bShowSockets)
	{
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{

			if (AActor* Mesh = Cast<AActor>(*It))
			{

				TArray<UActorComponent*> SourceComps;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
				SourceComps = Mesh->GetComponentsByClass(UActorComponent::StaticClass());
#else
				Mesh->GetComponents(SourceComps);
#endif

				for (UActorComponent* ActorComp : SourceComps)
				{
					if (!bUseNoSnapTag || !ActorComp->ComponentHasTag(FSnapSystemModule::NAME_NoSnap))
					{
						if (GetDefault<UUSnapSystemSettings>()->SourceComponentClasses.Contains(FSoftClassPath(ActorComp->GetClass())))
						{


							if (USplineComponent* SplineComp = Cast<USplineComponent>(ActorComp))
							{
								if (SplineComp->GetNumberOfSplinePoints() > 1)
								{
									//FString Name, Rest;

									//if (!SplineComp->GetName().Split("_", &Name, &Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
									//{
									//	Name = SplineComp->GetName();
									//}

									//TArray<FTransform> EndPoints;
									//EndPoints.Add(SplineComp->GetTransformAtSplinePoint(0, ESplineCoordinateSpace::World));
									//EndPoints.Add(SplineComp->GetTransformAtSplinePoint(SplineComp->GetNumberOfSplinePoints(), ESplineCoordinateSpace::World));

									//for (FTransform& T : EndPoints)
									//{
									//	const float DiamondSize = 10.f;
									//	const FColor DiamondColor(255, 128, 128);

									//	PDI->SetHitProxy(new HTestSocketProxy(Mesh, SplineComp, FName(*Name)));

									//	DrawCoordinateSystem(PDI, T.GetLocation(), T.GetRotation().Rotator(), 5.0f, SDPG_Foreground, 1.0f);

									//	PDI->SetHitProxy(NULL);
									//}
								}
							}
							else if (UInstancedStaticMeshComponent* ISMComp = Cast<UInstancedStaticMeshComponent>(ActorComp))
							{
								TArray<FComponentSocketDescription> OutSockets;
								ISMComp->QuerySupportedSockets(OutSockets);
								int32 Count = ISMComp->GetInstanceCount();

								if (OutSockets.Num() > 0 && Count > 0)
								{
									TArray<FTransform> IT;
									for (int32 I = 0; I < Count; I++)
									{
										FTransform T;
										ISMComp->GetInstanceTransform(I, T, true);
										IT.Add(T);
									}

									for (const FComponentSocketDescription& SocketDesc : OutSockets)
									{
										if (SocketDesc.Type == EComponentSocketType::Socket)
										{

											const FTransform T = ISMComp->GetSocketTransform(SocketDesc.Name, ERelativeTransformSpace::RTS_Component);
											const float DiamondSize = 10.f;
											const FColor DiamondColor(255, 128, 128);

											for (int32 I = 0; I < Count; I++)
											{
												const FTransform ITT = T * IT[I];
												//PDI->SetHitProxy(new HTestSocketProxy(Mesh, SourceComp, SocketDesc.Name));
												const float Scale = FMath::Max(4.0f, 4.0f * View->WorldToScreen(ITT.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]));

												//DrawSphere(PDI, T.GetLocation(), FRotator::ZeroRotator, FVector(DiamondSize), 10, 5, GEngine->ArrowMaterial->GetRenderProxy(false), SDPG_Foreground);
												//DrawWireDiamond(PDI, T.ToMatrixWithScale(), DiamondSize, DiamondColor, SDPG_Foreground);
												DrawCoordinateSystem(PDI, ITT.GetLocation(), ITT.GetRotation().Rotator(), Scale, SDPG_Foreground, Scale / 4.0f);

												//PDI->SetHitProxy(NULL);
											}
										}

									}
								}
							}
							else if (USceneComponent* SourceComp = Cast<USceneComponent>(ActorComp))
							{

								TArray<FComponentSocketDescription> OutSockets;
								SourceComp->QuerySupportedSockets(OutSockets);

								for (const FComponentSocketDescription& SocketDesc : OutSockets)
								{
									if (SocketDesc.Type == EComponentSocketType::Socket)
									{

										FTransform T = SourceComp->GetSocketTransform(SocketDesc.Name);
										const float DiamondSize = 10.f;
										const FColor DiamondColor(255, 128, 128);

										//PDI->SetHitProxy(new HTestSocketProxy(Mesh, SourceComp, SocketDesc.Name));
										const float Scale = FMath::Max(4.0f, 4.0f * View->WorldToScreen(T.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]));

										//DrawSphere(PDI, T.GetLocation(), FRotator::ZeroRotator, FVector(DiamondSize), 10, 5, GEngine->ArrowMaterial->GetRenderProxy(false), SDPG_Foreground);
										//DrawWireDiamond(PDI, T.ToMatrixWithScale(), DiamondSize, DiamondColor, SDPG_Foreground);
										DrawCoordinateSystem(PDI, T.GetLocation(), T.GetRotation().Rotator(), Scale, SDPG_Foreground, Scale / 4.0f);

										//PDI->SetHitProxy(NULL);
									}

								}
							}
						}
					}
				}
			}
		}
	}
}

void FMSSModeEdMode::DrawHUD(FEditorViewportClient * ViewportClient, FViewport * Viewport, const FSceneView * View, FCanvas * Canvas)
{
	//FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);
	const bool bUseNoSnapTag = GetDefault<UUSnapSystemSettings>()->bUseNoSnapTag;


	if (!ViewportClient->IsInGameView() && GetDefault<UUSnapSystemSettings>()->bShowSockets && GetDefault<UUSnapSystemSettings>()->ShowSocketMode > EShowSocketMode::Short)
	{
		const float DPI = Canvas->GetDPIScale();
		const int32 HalfX = Viewport->GetSizeXY().X / 2; 
		const int32 HalfY = Viewport->GetSizeXY().Y / 2;
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{

			if (AActor* Mesh = Cast<AActor>(*It))
			{

				TArray<UActorComponent*> SourceComps;// = Mesh->GetComponentsByClass(UActorComponent::StaticClass());
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
				SourceComps = Mesh->GetComponentsByClass(UActorComponent::StaticClass());
#else
				Mesh->GetComponents(SourceComps);
#endif

				for (UActorComponent* ActorComp : SourceComps)
				{
					if (!bUseNoSnapTag || !ActorComp->ComponentHasTag(FSnapSystemModule::NAME_NoSnap))
					{
						if (GetDefault<UUSnapSystemSettings>()->SourceComponentClasses.Contains(FSoftClassPath(ActorComp->GetClass())))
						{

							if (USplineComponent* SplineComp = Cast<USplineComponent>(ActorComp))
							{
								if (SplineComp->GetNumberOfSplinePoints() > 1)
								{
									FString Name, Rest;

									if (GetDefault<UUSnapSystemSettings>()->ShowSocketMode == EShowSocketMode::Normal)
									{
										if (!SplineComp->GetName().Split("_", &Name, &Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
										{
											Name = SplineComp->GetName();
										}
									}
									else
									{
										Name = SplineComp->GetName();
									}

									TArray<FTransform> EndPoints;
									EndPoints.Add(SplineComp->GetTransformAtSplinePoint(0, ESplineCoordinateSpace::World));
									EndPoints.Add(SplineComp->GetTransformAtSplinePoint(SplineComp->GetNumberOfSplinePoints(), ESplineCoordinateSpace::World));

									for (FTransform& T : EndPoints)
									{
										const FPlane proj = View->Project(T.GetLocation());
										if (proj.W > 0.f)
										{
											const int32 XPos = (HalfX + (HalfX * proj.X)) / DPI;
											const int32 YPos = (HalfY + (HalfY * (proj.Y * -1)) + 10) / DPI;

											FCanvasTextItem TextItem(FVector2D(XPos, YPos), FText::FromString(Name), GEngine->GetSmallFont(), FLinearColor(FColor(255, 196, 196)));
											FCanvasTextItem TextItemS(FVector2D(XPos + 1, YPos + 1), FText::FromString(Name), GEngine->GetSmallFont(), FLinearColor(FColor(0, 0, 0)));

											//Canvas->SetHitProxy(new HTestSocketProxy(Mesh, SplineComp, FName(*Name)));
											Canvas->DrawItem(TextItemS);
											Canvas->DrawItem(TextItem);
											//Canvas->SetHitProxy(NULL);

										}
									}
								}
							}

							else if (UInstancedStaticMeshComponent* ISMComp = Cast<UInstancedStaticMeshComponent>(ActorComp))
							{
								TArray<FComponentSocketDescription> OutSockets;
								ISMComp->QuerySupportedSockets(OutSockets);
								int32 Count = ISMComp->GetInstanceCount();

								if (OutSockets.Num() > 0 && Count > 0)
								{
									TArray<FTransform> IT;
									for (int32 I = 0; I < Count; I++)
									{
										FTransform T0;
										ISMComp->GetInstanceTransform(I, T0, true);
										IT.Add(T0);
									}

									for (const FComponentSocketDescription& SocketDesc : OutSockets)
									{
										if (SocketDesc.Type == EComponentSocketType::Socket)
										{

											const FTransform ParentT = ISMComp->GetSocketTransform(SocketDesc.Name, ERelativeTransformSpace::RTS_Component);


											for (int32 I = 0; I < Count; I++)
											{
												const FTransform T = ParentT * IT[I];

												if (FVector::DotProduct(View->ViewRotation.Vector(), T.Rotator().Vector()) < 0.0f)
												{
													FString Name, Rest;
													if (GetDefault<UUSnapSystemSettings>()->ShowSocketMode == EShowSocketMode::Normal)
													{
														if (!SocketDesc.Name.ToString().Split("_", &Name, &Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
														{
															Name = SocketDesc.Name.ToString();
														}
													}
													else
													{
														Name = SocketDesc.Name.ToString();
													}

													const FPlane proj = View->Project(T.GetLocation());
													if (proj.W > 0.f)
													{
														const int32 XPos = (HalfX + (HalfX * proj.X)) / DPI;
														const int32 YPos = (HalfY + (HalfY * (proj.Y * -1))) / DPI;

														FCanvasTextItem TextItem(FVector2D(XPos, YPos), FText::FromString(Name), GEngine->GetSmallFont(), FLinearColor(FColor(255, 196, 196)));
														FCanvasTextItem TextItemS(FVector2D(XPos + 1, YPos + 1), FText::FromString(Name), GEngine->GetSmallFont(), FLinearColor(FColor(0, 0, 0)));

														//Canvas->SetHitProxy(new HTestSocketProxy(Mesh, SourceComp, SocketDesc.Name));
														Canvas->DrawItem(TextItemS);
														Canvas->DrawItem(TextItem);
														//Canvas->SetHitProxy(NULL);

													}
												}

											}
										}

									}
								}
							}

							else if (USceneComponent* SourceComp = Cast<USceneComponent>(ActorComp))
							{

								TArray<FComponentSocketDescription> OutSockets;
								SourceComp->QuerySupportedSockets(OutSockets);

								for (const FComponentSocketDescription& SocketDesc : OutSockets)
								{
									if (SocketDesc.Type == EComponentSocketType::Socket)
									{

										FTransform T = SourceComp->GetSocketTransform(SocketDesc.Name);

										if (FVector::DotProduct(View->ViewRotation.Vector(), T.Rotator().Vector()) < 0.0f)
										{
											FString Name, Rest;
											if (GetDefault<UUSnapSystemSettings>()->ShowSocketMode == EShowSocketMode::Normal)
											{
												if (!SocketDesc.Name.ToString().Split("_", &Name, &Rest, ESearchCase::IgnoreCase, ESearchDir::FromStart))
												{
													Name = SocketDesc.Name.ToString();
												}
											}
											else
											{
												Name = SocketDesc.Name.ToString();
											}

											const FPlane proj = View->Project(T.GetLocation());
											if (proj.W > 0.f)
											{
												const int32 XPos = (HalfX + (HalfX * proj.X)) / DPI;
												const int32 YPos = (HalfY + (HalfY * (proj.Y * -1))) / DPI;

												FCanvasTextItem TextItem(FVector2D(XPos, YPos), FText::FromString(Name), GEngine->GetSmallFont(), FLinearColor(FColor(255, 196, 196)));
												FCanvasTextItem TextItemS(FVector2D(XPos + 1, YPos + 1), FText::FromString(Name), GEngine->GetSmallFont(), FLinearColor(FColor(0, 0, 0)));

												//Canvas->SetHitProxy(new HTestSocketProxy(Mesh, SourceComp, SocketDesc.Name));
												Canvas->DrawItem(TextItemS);
												Canvas->DrawItem(TextItem);
												//Canvas->SetHitProxy(NULL);

											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

bool FMSSModeEdMode::HandleClick(FEditorViewportClient * InViewportClient, HHitProxy * HitProxy, const FViewportClick & Click)
{
	//UE_LOG(LogTemp, Log, TEXT("HandleClick"));
	bSplineIsEdited = false;


	if (HitProxy)
	{
		if (HitProxy->IsA(HTestSocketProxy::StaticGetType()))
		{
			HTestSocketProxy* SocketProxy = static_cast<HTestSocketProxy*>(HitProxy);

			UE_LOG(LogTemp, Log, TEXT("Socket: %s"), *SocketProxy->SocketName.ToString());
			//TODO show menu
			return true;
		}
		else if (HitProxy->IsA(HComponentVisProxy::StaticGetType()))
		{
			HComponentVisProxy* VisProxy = (HComponentVisProxy*)HitProxy;
			const UActorComponent* ClickedComponent = VisProxy->Component.Get();
			if (ClickedComponent != NULL && ClickedComponent->IsA(USplineComponent::StaticClass()))
			{
				const USplineComponent* SplineComp = Cast<USplineComponent>(ClickedComponent);


				if (SplineComp->GetOwner())
				{
					SplineCurves = SplineComp->SplineCurves;
					bSplineIsEdited = true;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24					
					SplineCompPropName = FComponentVisualizer::GetComponentPropertyName(SplineComp);
#else					
					SplineCompPropPath = FComponentPropertyPath(SplineComp);
#endif

					SplineOwningActor = SplineComp->GetOwner();

					//UE_LOG(LogTemp, Log, TEXT("Clicked on spline point "));
				}
			}
		}
	}
	return  FEdMode::HandleClick(InViewportClient, HitProxy, Click);
}

bool FMSSModeEdMode::StartTracking(FEditorViewportClient * InViewportClient, FViewport * InViewport)
{
	//UE_LOG(LogTemp, Log, TEXT("StartTracking "));
	if (bSplineIsEdited)
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
		USplineComponent* SplineComp = Cast<USplineComponent>(FComponentVisualizer::GetComponentFromPropertyName(SplineOwningActor.Get(), SplineCompPropName));
#else
		USplineComponent* SplineComp = Cast<USplineComponent>(SplineCompPropPath.GetComponent());
#endif

		if (SplineComp != nullptr)
		{
			SplineCurves = SplineComp->SplineCurves;
		}
	}

	return false;// FEdMode::StartTracking(InViewportClient, InViewport);
}

bool FMSSModeEdMode::EndTracking(FEditorViewportClient * InViewportClient, FViewport * InViewport)
{
	//UE_LOG(LogTemp, Log, TEXT("EndTracking "));
	USplineComponent* C = nullptr;
	int32 I = 0;

	if (IsSplineEndpointEdited(C, I))
	{

		FSnapSystemModule& SSM = FSnapSystemModule::Get();
		SSM.SnapSplinePoint(C, I, FTransform(SplineCurves.Rotation.Points[I].OutVal, SplineCurves.Position.Points[I].OutVal, SplineCurves.Scale.Points[I].OutVal) * C->GetComponentTransform());

	}
	return false;// FEdMode::EndTracking(InViewportClient, InViewport);;
}

bool FMSSModeEdMode::UsesToolkits() const
{
	return false;
}

void FMSSModeEdMode::ActorSelectionChangeNotify()
{
	FEdMode::ActorSelectionChangeNotify();

	ClearPivot();
	//UE_LOG(LogTemp, Log, TEXT("ActorSelectionChangeNotify "));
	bSplineIsEdited = false;
}

void FMSSModeEdMode::ActorsDuplicatedNotify(TArray<AActor*>& PreDuplicateSelection, TArray<AActor*>& PostDuplicateSelection, bool bOffsetLocations)
{
	//FEdMode::ActorsDuplicatedNotify(PreDuplicateSelection, PostDuplicateSelection, bOffsetLocations);
	//bCustomPivot = true;
}

//bool FMSSModeEdMode::GetCustomDrawingCoordinateSystem(FMatrix & InMatrix, void * InData)
//{
//	if (bCustomPivot)
//	{
//		InMatrix = CustomPivot;
//	}
//	return false;// FEdMode::GetCustomDrawingCoordinateSystem(InMatrix, InData);//bCustomPivot;
//}

void FMSSModeEdMode::SetPivot(FTransform & Pivot)
{
	this->CustomPivot = Pivot.ToMatrixNoScale().RemoveTranslation();
	bCustomPivot = true;
}

void FMSSModeEdMode::ClearPivot()
{
	bCustomPivot = false;
}

bool FMSSModeEdMode::IsSplineEndpointEdited(USplineComponent*& SplineComp, int32& PointIndex)
{
	bool bModified = false;

	if (bSplineIsEdited)
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 24
		SplineComp = Cast<USplineComponent>(FComponentVisualizer::GetComponentFromPropertyName(SplineOwningActor.Get(), SplineCompPropName));
#else
		SplineComp = Cast<USplineComponent>(SplineCompPropPath.GetComponent());
#endif

		if (SplineComp != nullptr)
		{
			bool StartEdited = false;
			bool EndEdited = false;

			FSplineCurves NewCurves = SplineComp->SplineCurves;

			StartEdited = (SplineCurves.Position.Points[0].OutVal != NewCurves.Position.Points[0].OutVal ||
				SplineCurves.Rotation.Points[0].OutVal != NewCurves.Rotation.Points[0].OutVal ||
				(NewCurves.Position.Points[0].InterpMode != EInterpCurveMode::CIM_CurveAuto &&
					NewCurves.Position.Points[0].InterpMode != EInterpCurveMode::CIM_CurveAutoClamped &&
					(SplineCurves.Position.Points[0].ArriveTangent != NewCurves.Position.Points[0].ArriveTangent ||
						SplineCurves.Position.Points[0].LeaveTangent != NewCurves.Position.Points[0].LeaveTangent)
					));


			EndEdited = (SplineCurves.Position.Points.Last(0).OutVal != NewCurves.Position.Points.Last(0).OutVal ||
				SplineCurves.Rotation.Points.Last(0).OutVal != NewCurves.Rotation.Points.Last(0).OutVal ||
				(NewCurves.Position.Points.Last(0).InterpMode != EInterpCurveMode::CIM_CurveAuto &&
					NewCurves.Position.Points.Last(0).InterpMode != EInterpCurveMode::CIM_CurveAutoClamped &&
					(SplineCurves.Position.Points.Last(0).ArriveTangent != NewCurves.Position.Points.Last(0).ArriveTangent ||
						SplineCurves.Position.Points.Last(0).LeaveTangent != NewCurves.Position.Points.Last(0).LeaveTangent)
					));

			PointIndex = StartEdited ? 0 : (EndEdited ? SplineCurves.Position.Points.Num() - 1 : -1);

			bModified = StartEdited || EndEdited;
		}
	}
	return bModified;
}
