// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Framework/Commands/InputChord.h"
#include "USnapSystemSettings.generated.h"


UENUM()
namespace EShowSocketMode
{
	enum Type
	{
		//Shows only the transform
		Short,
		//Shows the transform and the snap point name
		Normal,
		//Shows the transform and the full socket name
		Full,

		MAX UMETA(Hidden)
	};
}

/**
 * Snap system plugin settings 
 */
UCLASS(config = EditorPerProjectUserSettings)
class SNAPSYSTEM_API UUSnapSystemSettings : public UObject
{
	GENERATED_UCLASS_BODY()


public:

	//Enables or disables SnapSystem Plugin. Same as the viewport button.
	//
	//You can configure the keyboard shortcut for this and other commands
	//under "Editor Preferences -> Keyboard Shortcuts -> SnapSystem Plugin"
	UPROPERTY(EditAnywhere, Config, Category = Settings)
	bool bIsEnabled;

	//Show sockets on selected actors. Only the "snap point name" 
	//is shown (the part of the socket name before the first "_")
	//
	//Default: false
	//
	//You can configure the keyboard shortcut for this and other commands
	//under "Editor Preferences -> Keyboard Shortcuts -> SnapSystem Plugin"
	UPROPERTY(EditAnywhere, Config, Category = Settings)
	bool bShowSockets;

	//Modes for showing sockets
	//Default: Normal
	UPROPERTY(EditAnywhere, Config, Category = Settings)
	TEnumAsByte<EShowSocketMode::Type> ShowSocketMode;

	//How far in centimeters to look for a matching socket. 
	//
	//Default: 100cm.
	UPROPERTY(EditAnywhere, Config, meta = (UIMin = 0, ClampMin = 0), Category = Search)
	float SearchDist;
	
	//Max angle in degrees between sockets that are allowed to snap.
	//
	//0 means X-axis should be stricty opposing each other, 
	//while 180 means sockets at any angle can snap.
	//
	//Allowed values [0-180]. Default value is 75 degrees.
	UPROPERTY(EditAnywhere, Config, meta = (UIMin = 0, ClampMin = 0, UIMax = 180, ClampMax = 180), Category = Search)
	float MaxAngle;

	//Don't check socket names when searching for socket to snap to.
	//
	//If false: only sockets with matching names (same prefix before the first "_" character) will snap.
	//If true: all sockets will snap, regardless of their names.
	//
	//Deafult: false
	UPROPERTY(EditAnywhere, Config, Category = Search)
	bool bIgnoreSocketName;

	//When moving an actor with other actors attached to it 
	//consider sockets of attached actors too, even if they are not selected.
	//Useful with some "prefab" style plugins.
	//
	//If false: consider only sockets of selected actors
	//If true: consider selected actors and actors attached to them (everything that moves)
	//
	//Default: false
	UPROPERTY(EditAnywhere, Config, Category = Search, AdvancedDisplay)
	bool bSnapAttachedToo;


	//Check if a socket is "open" before considering it for snapping
	//Runs an additional sphere trace just in front of the socket to see if there is no obstruction
	//Only components in TargetComponentClasses are considered as blockers!
	//
	//If false: don't run "open" checks
	//If true: check if the socket is not hidden by something else
	//
	//Default: false
	UPROPERTY(EditAnywhere, Config, Category = Search)
	bool bSnapOpenOnly;

	//If bSnapOpenOnly is enabled, how far to look for obstruction
	//Wii trace with a sphere centered at this distance in front of the socket
	//
	//Default: 2cm
	UPROPERTY(EditAnywhere, Config, Category = Search, AdvancedDisplay)
	float OpenCheckRadius;

	//Enables the check of "nosnap" component tag. 
	//If the component has such tag it will not be considered for snap operation.
	//
	//Default: true
	UPROPERTY(EditAnywhere, Config, Category = Search, AdvancedDisplay)
	bool bUseNoSnapTag;

	//Rotation snap in degrees. Can be overriden in socket name using _RX _RY _RZ parameters.
	//
	//0 means: keep world rotation, no rotation snap, 
	//360: snap to the target socket world rotation,
	//angles in between: snap to the nearest increment from the target socket world rotation. 
	//
	//Allowed values [0-360]. Default (90, 360, 360)
	UPROPERTY(EditAnywhere, Config, Category = Snap)
	FVector DefaultRotSnap;



	//Scale snap flag. Can be overriden in socket name using _SX _SY _SZ parameters.
	//
	//0 means: keep world scale, no scale snap, 
	//1: snap to the target socket world scale
	//Allowed values: 0 or 1. Default (1, 1, 1). 
	//
	//WARNING! UE4 Actor's scale is always in actor's LOCAL SPACE (not socket space).
	//It means, that for non uniform scaling weird results are expected.
	//
	//USE WITH CAUTION!
	UPROPERTY(EditAnywhere, Config, Category = Snap)
	FVector DefaultScaleSnap;


	//Don't use socket parameters when snapping.
	//
	//If true: snap point parameters like rotsnap (the part of the socket name after the first "_") 
	//will not be used, only the default values defined here in settings will be used.
	//
	//Deafult: false
	UPROPERTY(EditAnywhere, Config, Category = Snap)
	bool bIgnoreSocketParams;

	//Attach selected actors to the parent ot the target actor (the actor who has the matched socket).
	UPROPERTY(EditAnywhere, Config, Category = Snap)
	bool bAttachToParent;


	//When snapping to the socket marked with "_A" parameter, should we attach to the target socket or to the target actor's root?
	//
	//If this is true (default), the snapped actor will be attached to the matched target actor's socket.
	//Otherwise, it will be attached to the target actor's origin.
	//Attaching to socket may be important when attaching to skeletal meshes (attach to the hand for example),
	//but also can give problems when the attached actor is duplicated using alt-drag 
	//(Unreal fails to calculate the correct transform for the duplicated actor, so it will be upside-down)
	//
	//Attaching to Static Mesh Components ignores this parameter, always attach to the origin.
	UPROPERTY(EditAnywhere, Config, Category = Snap, AdvancedDisplay)
	bool bAttachToSocket;

	//Move the selection pivot to the snap point. Useful for rotating the snapped actors
	//around the last snap point. 
	//
	//Default: false.
	UPROPERTY(EditAnywhere, Config, Category = Snap)
	bool bSnapPivot;


	//Prevents "snapping back" to the previous snap point.
	//
	//If you snap an actor to another, then move it away a little it will snap back.
	//That can be annoying. Setting bNoSnapback to true prevents this behaviour.
	//Snap operation will be aborted if the transform after the snap 
	//is equal to the transform before the move started.
	//
	//Default: true
	//
	//See also: "Hold to Skip Snap" keyboard shortcut (default: C) under 
	//"Editor Preferences -> Keyboard Shortcuts -> SnapSystem Plugin"
	UPROPERTY(EditAnywhere, Config, Category = Snap, AdvancedDisplay)
	bool bNoSnapback;

	//How close is considered equals for "No Snapback" 
	//
	//Because of numerical errors for very large objects 
	//the default value of 0.1 can be too much. Try to increase.
	//For very small objects 1mm may be too much. Try to decrease.
	//
	//Default: 0.1 (1mm)
	UPROPERTY(EditAnywhere, Config, Category = Snap, AdvancedDisplay)
	float NoSnapbackTolerance;

	
	//Ignore scale when using spline end point as socket.
	//
	//Spline point scale is often non-uniform and in general 
	//quite different thing from normal actor's scale.
	//Therefore scaling actors to it can lead to weird results.
	//
	//This flag is taken into account when snapping actors, 
	//but ignored when snapping spline endpoint to another spline endpoint. 
	//Spline endpoints will use socket scale when it's 
	//allowed by global and socket parameters.
	//
	//Default: true
	UPROPERTY(EditAnywhere, Config, Category = Snap, AdvancedDisplay)
	bool bIgnoreSplineSocketScale;



	//Components to snap. Source of search operation.
	//
	//Components of moved actors around which we will start to search for snapping points
	//Can be any scene component containig sockets and SplineComponent 
	//(endpoints are considered as sockets for splines)
	//
	//Default: (StaticMeshComponent, SplineComponent)
	UPROPERTY(config, noclear, EditAnywhere, Category = Search, meta = (MetaClass = "SceneComponent"), AdvancedDisplay)
	TArray<FSoftClassPath> SourceComponentClasses;

	//Components to snap to. Target of search operation.
	//
	//Components of actors that was not moved where matching snap points can be located.
	//Can be any component containig sockets and SplineComponent 
	//(endpoints are considered as sockets for splines)
	//
	//Default: (StaticMeshComponent, SplineComponent)
	UPROPERTY(config, noclear, EditAnywhere, Category = Search, meta = (MetaClass = "SceneComponent"), AdvancedDisplay)
	TArray<FSoftClassPath> TargetComponentClasses;
};
