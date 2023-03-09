// Copyright 2018-2020 S.Chachkov & A.Putrino. All Rights Reserved.

#include "USnapSystemSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"


UUSnapSystemSettings::UUSnapSystemSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	
	//general
	bIsEnabled(true),
	bShowSockets(false),
	ShowSocketMode(EShowSocketMode::Normal),

	//search
	SearchDist(100.0f),
	MaxAngle(75.0f),
	bIgnoreSocketName(false),
	bSnapAttachedToo(false),
	bSnapOpenOnly(false),
	OpenCheckRadius(2.0f),
	bUseNoSnapTag(true),

	//snap
	DefaultRotSnap(FVector(90.0f, 360.0f, 360.0f)),
	DefaultScaleSnap(FVector(1.0f, 1.0f, 1.0f)),
	bIgnoreSocketParams(false),

	bAttachToParent(true),
	bAttachToSocket(true),
	bSnapPivot(false),
	bNoSnapback(true),
	NoSnapbackTolerance(0.1f),
	bIgnoreSplineSocketScale(true)


	

{
	SourceComponentClasses.Add(FSoftClassPath(UStaticMeshComponent::StaticClass()));
	SourceComponentClasses.Add(FSoftClassPath(USkeletalMeshComponent::StaticClass()));
	SourceComponentClasses.Add(FSoftClassPath(USplineComponent::StaticClass()));
	TargetComponentClasses.Add(FSoftClassPath(UStaticMeshComponent::StaticClass()));
	TargetComponentClasses.Add(FSoftClassPath(USkeletalMeshComponent::StaticClass()));
	TargetComponentClasses.Add(FSoftClassPath(USplineComponent::StaticClass()));

}