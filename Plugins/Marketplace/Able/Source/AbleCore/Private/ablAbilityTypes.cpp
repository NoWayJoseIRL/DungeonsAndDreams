// Copyright (c) Extra Life Studios, LLC. All rights reserved.
#include "ablAbilityTypes.h"

#include "AbleCorePrivate.h"
#include "ablAbility.h"
#include "ablAbilityContext.h"
#include "Camera/CameraActor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"

FAblAbilityTargetTypeLocation::FAblAbilityTargetTypeLocation()
	: m_Source(EAblAbilityTargetType::ATT_Self),
	m_Offset(ForceInitToZero),
	m_Rotation(ForceInitToZero),
	m_Socket(NAME_None),
	m_UseSocketRotation(false),
	m_UseActorRotation(false)
{

}

void FAblAbilityTargetTypeLocation::GetTargetTransform(const UAblAbilityContext& Context, int32 TargetIndex, FTransform& OutTransform) const
{
	if (TargetIndex >= Context.GetTargetActors().Num())
	{
		UE_LOG(LogAble, Warning, TEXT("Ability [%s] tried to get a target transform at time [%2.3f], but none were available. Using Self Actor's transform instead.\n"), *Context.GetAbility()->GetDisplayName(), Context.GetCurrentTime());
		OutTransform = Context.GetSelfActor()->GetActorTransform();
		return;
	}

	if (m_Source == EAblAbilityTargetType::ATT_World)
	{
		OutTransform = FTransform(FQuat(m_Rotation), m_Offset);
		return;
	}

	TWeakObjectPtr<AActor> TargetActor = Context.GetTargetActorsWeakPtr()[TargetIndex];

	if (TargetActor.IsValid())
	{
		if (!m_Socket.IsNone())
		{
			USkeletalMeshComponent* SkeletalMesh = Context.GetAbility()->GetSkeletalMeshComponentForActorBP(&Context, TargetActor.Get(), m_Socket);
			if (!SkeletalMesh)
			{
				SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
			}

			if (SkeletalMesh)
			{
				if (m_UseSocketRotation)
				{
					OutTransform = SkeletalMesh->GetSocketTransform(m_Socket);
				}
				else
				{
					OutTransform.SetTranslation(SkeletalMesh->GetSocketLocation(m_Socket));
				}
			}
		}
		else
		{
			if (m_Source == EAblAbilityTargetType::ATT_Camera && !TargetActor->IsA<ACameraActor>())
			{
				FVector ActorEyes;
				FRotator ActorEyesRot;
				TargetActor->GetActorEyesViewPoint(ActorEyes, ActorEyesRot);
				OutTransform = FTransform(FQuat(ActorEyesRot), ActorEyes);
			}
			else if (m_Source == EAblAbilityTargetType::ATT_Location)
			{
				OutTransform.SetTranslation(Context.GetTargetLocation());
			}
			else
			{
				OutTransform = TargetActor->GetActorTransform();
			}
		}

		if (m_UseActorRotation)
		{
			OutTransform.SetRotation(TargetActor->GetActorQuat());
		}

		OutTransform.ConcatenateRotation(m_Rotation.Quaternion());

		FQuat Rotator = OutTransform.GetRotation();

		FVector LocalSpaceOffset = Rotator.GetForwardVector() * m_Offset.X;
		LocalSpaceOffset += Rotator.GetRightVector() * m_Offset.Y;
		LocalSpaceOffset += Rotator.GetUpVector() * m_Offset.Z;

		OutTransform.AddToTranslation(LocalSpaceOffset);
	}
	else
	{
		UE_LOG(LogAble, Warning, TEXT("Unable to find an Actor for our Targeting source, Query will be from the origin. Is this intended?"));

		OutTransform = FTransform(m_Rotation, m_Offset);
	}
}

void FAblAbilityTargetTypeLocation::GetTransform(const UAblAbilityContext& Context, FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (m_Source == EAblAbilityTargetType::ATT_World)
	{
		OutTransform = FTransform(FQuat(m_Rotation), m_Offset);
		return;
	}

	AActor* BaseActor = GetSourceActor(Context);

	if (!BaseActor)
	{
		UE_LOG(LogAble, Warning, TEXT("Unable to find an Actor for our Targeting source, Query will be from the origin. Is this intended?"));

		OutTransform = FTransform(m_Rotation, m_Offset);
	}
	else
	{
		if (!m_Socket.IsNone())
		{
			USkeletalMeshComponent* SkeletalMesh = Context.GetAbility()->GetSkeletalMeshComponentForActorBP(&Context, BaseActor, m_Socket);
			if (!SkeletalMesh)
			{
				SkeletalMesh = BaseActor->FindComponentByClass<USkeletalMeshComponent>();
			}

			if (SkeletalMesh)
			{
				if (m_UseSocketRotation)
				{
					OutTransform = SkeletalMesh->GetSocketTransform(m_Socket);
				}
				else
				{
					OutTransform.SetTranslation(SkeletalMesh->GetSocketLocation(m_Socket));
				}
			}
		}
		else
		{
			// If we're not a camera actor, we get our "Eyes point" which is just the camera by another name.
			// Otherwise, it's safe just to grab our actor location.
			if (m_Source == EAblAbilityTargetType::ATT_Camera && !BaseActor->IsA<ACameraActor>())
			{
				FVector ActorEyes;
				FRotator ActorEyesRot;
				BaseActor->GetActorEyesViewPoint(ActorEyes, ActorEyesRot);
				OutTransform = FTransform(FQuat(ActorEyesRot), ActorEyes);
			}
			else if (m_Source == EAblAbilityTargetType::ATT_Location)
			{
				OutTransform.SetTranslation(Context.GetTargetLocation());
			}
			else
			{
				OutTransform = BaseActor->GetActorTransform();
			}
		}

		if (m_UseActorRotation)
		{
			OutTransform.SetRotation(BaseActor->GetActorQuat());
		}

		OutTransform.ConcatenateRotation(m_Rotation.Quaternion());

		FQuat Rotator = OutTransform.GetRotation();

		FVector LocalSpaceOffset = Rotator.GetForwardVector() * m_Offset.X;
		LocalSpaceOffset += Rotator.GetRightVector() * m_Offset.Y;
		LocalSpaceOffset += Rotator.GetUpVector() * m_Offset.Z;

		OutTransform.AddToTranslation(LocalSpaceOffset);
	}
}

AActor* FAblAbilityTargetTypeLocation::GetSourceActor(const UAblAbilityContext& Context) const
{
	switch (m_Source)
	{
		case EAblAbilityTargetType::ATT_World:
		case EAblAbilityTargetType::ATT_Location:
		case EAblAbilityTargetType::ATT_Self:
		case EAblAbilityTargetType::ATT_Camera: // Camera we just return self.
		{
			return Context.GetSelfActor();
		}
		break;
		case EAblAbilityTargetType::ATT_Instigator:
		{
			return Context.GetInstigator();
		}
		break;
		case EAblAbilityTargetType::ATT_Owner:
		{
			return Context.GetOwner();
		}
		break;
		case EAblAbilityTargetType::ATT_TargetActor:
		{
			if (Context.GetTargetActorsWeakPtr().Num() && Context.GetTargetActorsWeakPtr()[0].IsValid())
			{
				return Context.GetTargetActorsWeakPtr()[0].Get();
			}
		}
		break;
		default:
		{
		}
		break;
	}

	return nullptr;
}

UAblDynamicPropertySupport::UAblDynamicPropertySupport(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UAblDynamicPropertySupport::~UAblDynamicPropertySupport()
{

}

FString UAblDynamicPropertySupport::GetDynamicPropertyParentIdentifier() const
{
	checkNoEntry();
	return FString();
}

FName UAblDynamicPropertySupport::GetDynamicDelegateName(const FString& PropertyName) const
{
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	FString DelegateName = FString::Format(TEXT("OnGetDynamicProperty_%s_%s"), { GetDynamicPropertyParentIdentifier(), PropertyName });

	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName = FString::Format(TEXT("%s_%s"), { DelegateName, DynamicIdentifier });
	}

	return FName(*DelegateName);
}
