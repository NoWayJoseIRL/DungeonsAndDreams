// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "IAbleCore.h"

#include "CoreMinimal.h"

class ABLECORE_API FAblAbilityDebug
{
public:
#if !UE_BUILD_SHIPPING
	/* Set whether to Draw all queries, or not. */
	static void EnableDrawQueries(bool Enable);

	/* Set whether or not to preview draw queries in the Editor.*/
	static void EnableDrawInEditor(bool Enable);

	/* Returns true if we should be drawing queries. */
	static bool ShouldDrawQueries();

	/* Returns true if we should be drawing preview queries. */
	static bool ShouldDrawInEditor();

	/* Returns the lifetime, in seconds, that a query should be visible. */
	static float GetDebugQueryLifetime();

	/* Sets the lifetime, in seconds, that a query should be visible. */
	static void SetDebugQueryLifetime(float lifetime);

	/* Draws a Sphere Query.*/
	static void DrawSphereQuery(const UWorld* World, const FTransform& QueryTransform, float Radius);
	
	/* Draws a Box Query. */
	static void DrawBoxQuery(const UWorld* World, const FTransform& QueryTransform, const FVector& HalfExtents);
	
	/* Draws a 2D Cone Query. */
	static void Draw2DConeQuery(const UWorld* World, const FTransform& QueryTransform, float FOV, float Length);
	
	/* Draws a 3D Cone Query. */
	static void DrawConeQuery(const UWorld* World, const FTransform& QueryTransform, float FOV, float Length, float Height);

	/* Draws a 3D Cone Slice Query. */
	static void DrawConeSliceQuery(const UWorld* World, const FTransform& QueryTransform, float FOV, float Length, float Height);
	
	/* Draws a Raycast Query. */
	static void DrawRaycastQuery(const UWorld* World, const FVector& QueryStart, const FVector& QueryEnd);
	
	/* Draws a Capsule Query. */
	static void DrawCapsuleQuery(const UWorld* World, const FTransform& QueryTransform, float Radius, float Height);

	/* Draws a Bow Sweep Query. */
	static void DrawBoxSweep(const UWorld* World, const FTransform& Start, const FTransform& End, const FVector& HalfExtents);
	
	/* Draws a Sphere Sweep Query. */
	static void DrawSphereSweep(const UWorld* World, const FTransform& Start, const FTransform& End, float Radius);
	
	/* Draws a Capsule Sweep Query. */
	static void DrawCapsuleSweep(const UWorld* World, const FTransform& Start, const FTransform& End, float Radius, float Height);
private:
	static bool ForceDrawQueries;
	static bool DrawInEditor;

#endif // UE_BUILD_SHIPPING
};