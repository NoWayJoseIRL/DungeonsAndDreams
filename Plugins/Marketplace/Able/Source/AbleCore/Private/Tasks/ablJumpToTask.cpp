// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablJumpToTask.h"

#include "ablAbility.h"
#include "ablAbilityUtilities.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblJumpToScratchPad::UAblJumpToScratchPad()
{

}

UAblJumpToScratchPad::~UAblJumpToScratchPad()
{

}

UAblJumpToTask::UAblJumpToTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server)
{

}

UAblJumpToTask::~UAblJumpToTask()
{

}

void UAblJumpToTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblJumpToScratchPad* ScratchPad = CastChecked<UAblJumpToScratchPad>(Context->GetScratchPadForTask(this));

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	ScratchPad->CurrentTargetLocation = GetTargetLocation(Context);
	ScratchPad->JumpingPawns.Empty();

	for (TWeakObjectPtr<AActor>& Target : TaskTargets)
	{
		if (Target.IsValid())
		{
			if (APawn* Pawn = Cast<APawn>(Target))
			{
				SetPhysicsVelocity(Context, Pawn, ScratchPad->CurrentTargetLocation, ScratchPad);
			}
			else
			{
				UE_LOG(LogAble, Warning, TEXT("Actor %s is not a Pawn. Unable to set velocity."), *Target->GetName());
			}
		}
	}
}

void UAblJumpToTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	if (m_TrackActor)
	{
		UAblJumpToScratchPad* ScratchPad = CastChecked<UAblJumpToScratchPad>(Context->GetScratchPadForTask(this));
		FVector potentialNewLocation = GetTargetLocation(Context);

		if ((m_Use2DDistanceChecks ?
			FVector::DistSquared2D(ScratchPad->CurrentTargetLocation, potentialNewLocation) :
			FVector::DistSquared(ScratchPad->CurrentTargetLocation, potentialNewLocation)) > (m_AcceptableRadius * m_AcceptableRadius))
		{
			for (TWeakObjectPtr<APawn>& JumpingPawn : ScratchPad->JumpingPawns)
			{
				ScratchPad->CurrentTargetLocation = potentialNewLocation;
				SetPhysicsVelocity(Context, JumpingPawn.Get(), ScratchPad->CurrentTargetLocation, ScratchPad, false);
			}
		}
	}
}

void UAblJumpToTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const
{
	UAblJumpToScratchPad* ScratchPad = CastChecked<UAblJumpToScratchPad>(Context->GetScratchPadForTask(this));

	if (m_CancelMoveOnInterrupt && Result == EAblAbilityTaskResult::Interrupted)
	{
		for (TWeakObjectPtr<APawn>& JumpingPawn : ScratchPad->JumpingPawns)
		{
			if (UCharacterMovementComponent* CharacterMovementComponent = JumpingPawn->FindComponentByClass<UCharacterMovementComponent>())
			{
				CharacterMovementComponent->RequestDirectMove(FVector::ZeroVector, true);
			}
			else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(JumpingPawn->GetRootComponent()))
			{
				PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
		}
	}
}

bool UAblJumpToTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (m_EndTaskImmediately)
	{
		return true;
	}

	UAblJumpToScratchPad* ScratchPad = CastChecked<UAblJumpToScratchPad>(Context->GetScratchPadForTask(this));

	ScratchPad->JumpingPawns.RemoveAll([&](const TWeakObjectPtr<APawn>& LHS)
	{
		if (!LHS.IsValid())
		{
			return true;
		}

		if ((m_Use2DDistanceChecks ?
			FVector::DistSquared2D(ScratchPad->CurrentTargetLocation, LHS->GetActorLocation()) :
			FVector::DistSquared(ScratchPad->CurrentTargetLocation, LHS->GetActorLocation())) <= (m_AcceptableRadius * m_AcceptableRadius))
		{
			return true;
		}

		return false;
	});

	// We're not done as long as we have some outstanding work.
	return ScratchPad->JumpingPawns.Num() > 0;
}

UAblAbilityTaskScratchPad* UAblJumpToTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblJumpToScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblJumpToScratchPad>(Context.Get());
}

TStatId UAblJumpToTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblJumpToTask, STATGROUP_Able);
}

void UAblJumpToTask::BindDynamicDelegates( UAblAbility* Ability )
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TargetLocation, TEXT("Target Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_JumpHeight, TEXT("Jump Height"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Speed, TEXT("Speed"));
}

#if WITH_EDITOR

FText UAblJumpToTask::GetDescriptiveTaskName() const
{
	return FText::FormatOrdered(LOCTEXT("AblJumpToTaskDesc", "Jump to {0}"), FText::FromString(FAbleLogHelper::GetTargetTargetEnumAsString(m_TargetActor.GetValue())));
}

#endif

FVector UAblJumpToTask::GetTargetLocation(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (m_TargetType.GetValue() == EAblJumpToTarget::JTT_Actor)
	{

		AActor* actor = GetSingleActorFromTargetType(Context, m_TargetActor.GetValue());

		if (!actor)
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Failed to find Actor using Target Type %s, using Self."), *FAbleLogHelper::GetTargetTargetEnumAsString(m_TargetActor.GetValue())));
			}
#endif
			actor = Context->GetSelfActor();
		}

		return actor->GetActorLocation();
	}
	else
	{
		return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TargetLocation);
	}
}

void UAblJumpToTask::SetPhysicsVelocity(const TWeakObjectPtr<const UAblAbilityContext>& Context, APawn* Target, const FVector& EndLocation, UAblJumpToScratchPad* ScratchPad, bool addToScratchPad) const
{
	// Vf^2 = Vi^2 + 2 * A * D;
	FVector towardsTarget = EndLocation - Target->GetActorLocation();
	towardsTarget = towardsTarget - (towardsTarget.GetSafeNormal() * m_TargetActorOffset); // Our offset.
	float d = towardsTarget.Size2D();
	float a = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Speed);

	if (UCharacterMovementComponent* CharacterMovementComponent = Target->FindComponentByClass<UCharacterMovementComponent>())
	{
		FVector Vi = FMath::Square(CharacterMovementComponent->Velocity);
		FVector Vf = Vi + 2.0f * (a * d);
		Vf.X = FMath::Sqrt(Vf.X);
		Vf.Y = FMath::Sqrt(Vf.X);
		Vf.Z = FMath::Sqrt(Vf.Z);

		// Jump Height = Gravity * Height.
		Vf += Target->GetActorUpVector() + (Target->GetGravityDirection() * ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_JumpHeight));

		CharacterMovementComponent->RequestDirectMove(Vf, false);

		if (addToScratchPad)
		{
			ScratchPad->JumpingPawns.Add(Target);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on CharacterMovementComponent to %s"), *Target->GetName(), *Vf.ToCompactString()));
		}
#endif
	}
	else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		FVector Vi = FMath::Square(PrimitiveComponent->GetPhysicsLinearVelocity());
		FVector Vf = Vi + 2.0f * (a * d);
		Vf.X = FMath::Sqrt(Vf.X);
		Vf.Y = FMath::Sqrt(Vf.X);
		Vf.Z = FMath::Sqrt(Vf.Z);

		// Jump Height = Gravity * Height.
		Vf += Target->GetActorUpVector() + (Target->GetGravityDirection() * ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_JumpHeight));

		PrimitiveComponent->SetPhysicsLinearVelocity(Vf);

		if (addToScratchPad)
		{
			ScratchPad->JumpingPawns.Add(Target);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on Primitive Root Component to %s"), *Target->GetName(), *Vf.ToCompactString()));
		}
#endif
	}
	else
	{
		UE_LOG(LogAble, Warning, TEXT("Actor %s doesn't have a Character Movement Component or Primitive Root component. Unable to set velocity."), *Target->GetName());
	}
}

#undef LOCTEXT_NAMESPACE