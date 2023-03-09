// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablMoveToTask.h"

#include "ablAbility.h"
#include "ablAbilityUtilities.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"


#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblMoveToScratchPad::UAblMoveToScratchPad()
{

}

UAblMoveToScratchPad::~UAblMoveToScratchPad()
{

}

UAblMoveToTask::UAblMoveToTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server)
{

}

UAblMoveToTask::~UAblMoveToTask()
{

}

void UAblMoveToTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblMoveToScratchPad* ScratchPad = CastChecked<UAblMoveToScratchPad>(Context->GetScratchPadForTask(this));

	ScratchPad->CurrentTargetLocation = GetTargetLocation(Context);
	ScratchPad->ActiveMoveRequests.Empty();
	ScratchPad->ActivePhysicsMoves.Empty();
	ScratchPad->AsyncQueryIdArray.Empty();
	ScratchPad->CompletedAsyncQueries.Empty();
	ScratchPad->NavPathDelegate.Unbind();

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	for (TWeakObjectPtr<AActor>& Target : TaskTargets)
	{
		if (Target.IsValid())
		{
			if (m_UseNavPathing)
			{
				StartPathFinding(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
			}
			else
			{
				SetPhysicsVelocity(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
			}

		}
	}
}

void UAblMoveToTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

	UAblMoveToScratchPad* ScratchPad = CastChecked<UAblMoveToScratchPad>(Context->GetScratchPadForTask(this));

	// Handle any completed Async pathing queries.
	TArray<TPair<uint32, FNavPathSharedPtr>>::TIterator itProcess = ScratchPad->CompletedAsyncQueries.CreateIterator();
	for(; itProcess; ++itProcess)
	{

		TPair<uint32, TWeakObjectPtr<APawn>>* foundRecord = ScratchPad->AsyncQueryIdArray.FindByPredicate([&](const TPair<uint32, TWeakObjectPtr<APawn>>& LHS)
		{
			return LHS.Key == itProcess->Key;
		});

		if (!foundRecord)
		{
			// No record found? We may have replaced it with a more recent request. Ignore it.
			itProcess.RemoveCurrent();
			continue;
		}

		// We have a path and our actor is still valid. Start the move.
		if (foundRecord->Value.IsValid() && itProcess->Value.IsValid())
		{
			if (UPathFollowingComponent* PathFindingComponent = foundRecord->Value->FindComponentByClass<UPathFollowingComponent>())
			{
				// Start the move.
				FAIRequestID MoveRequest = PathFindingComponent->RequestMove(FAIMoveRequest(ScratchPad->CurrentTargetLocation), itProcess->Value);
				ScratchPad->ActiveMoveRequests.Add(TPair<FAIRequestID, TWeakObjectPtr<APawn>>(MoveRequest, foundRecord->Value));

#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Requested Move for Target %s."), *foundRecord->Value->GetName()));
				}
#endif
			}
		}

		ScratchPad->AsyncQueryIdArray.Remove(*foundRecord);
		itProcess.RemoveCurrent();
	}

	// If we need to update our path, do so.
	if (m_UpdateTargetPerFrame)
	{
		FVector newEndPoint = GetTargetLocation(Context);

		if ((m_Use2DDistanceChecks ?
			FVector::DistSquared2D(ScratchPad->CurrentTargetLocation, newEndPoint) :
			FVector::DistSquared(ScratchPad->CurrentTargetLocation, newEndPoint)) > (m_AcceptableRadius * m_AcceptableRadius))
		{
			// New distance, redo our pathing logic.
			ScratchPad->CurrentTargetLocation = newEndPoint;

			ScratchPad->ActiveMoveRequests.Empty(ScratchPad->ActiveMoveRequests.Num());
			ScratchPad->ActivePhysicsMoves.Empty(ScratchPad->ActivePhysicsMoves.Num());
			ScratchPad->AsyncQueryIdArray.Empty();

			ScratchPad->CurrentTargetLocation = newEndPoint;

			TArray<TWeakObjectPtr<AActor>> TaskTargets;
			GetActorsForTask(Context, TaskTargets);

			for (TWeakObjectPtr<AActor>& Target : TaskTargets)
			{
				if (Target.IsValid())
				{
					if (m_UseNavPathing)
					{
						StartPathFinding(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
					}
					else
					{
						SetPhysicsVelocity(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
					}
				}
			}
		}
	}

}

void UAblMoveToTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const
{
	Super::OnTaskEnd(Context, Result);

	if (!Context.IsValid())
	{
		return;
	}

	if (Result == EAblAbilityTaskResult::Interrupted && m_CancelMoveOnInterrupt)
	{
		UAblMoveToScratchPad* ScratchPad = CastChecked<UAblMoveToScratchPad>(Context->GetScratchPadForTask(this));

		for (TPair<FAIRequestID, TWeakObjectPtr<APawn>>& RequestPawnPair : ScratchPad->ActiveMoveRequests)
		{
			if (RequestPawnPair.Value.IsValid())
			{
				if (UPathFollowingComponent* PathComponent = RequestPawnPair.Value->FindComponentByClass<UPathFollowingComponent>())
				{
					PathComponent->AbortMove(*this, FPathFollowingResultFlags::UserAbort, RequestPawnPair.Key);
				}
			}
		}

		for (TWeakObjectPtr<AActor>& PhysicsActor : ScratchPad->ActivePhysicsMoves)
		{
			if (UCharacterMovementComponent* CharacterMovementComponent = PhysicsActor->FindComponentByClass<UCharacterMovementComponent>())
			{
				CharacterMovementComponent->RequestDirectMove(FVector::ZeroVector, true);
			}
			else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(PhysicsActor->GetRootComponent()))
			{
				PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
		}
	}
}

UAblAbilityTaskScratchPad* UAblMoveToTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblMoveToScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblMoveToScratchPad>(Context.Get());
}

TStatId UAblMoveToTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblMoveToTask, STATGROUP_Able);
}

void UAblMoveToTask::BindDynamicDelegates( UAblAbility* Ability )
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TargetLocation, TEXT("Target Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Speed, TEXT("Speed"));
}

bool UAblMoveToTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (m_TimeOut > 0.0f && Context->GetCurrentTime() >= GetEndTime())
	{
		return true;
	}

	UAblMoveToScratchPad* ScratchPad = CastChecked<UAblMoveToScratchPad>(Context->GetScratchPadForTask(this));

	ScratchPad->ActiveMoveRequests.RemoveAll([](const TPair<FAIRequestID, TWeakObjectPtr<APawn>>& LHS)
	{
		if (LHS.Value.IsValid())
		{
			return true;
		}

		if (UPathFollowingComponent* PathComponent = LHS.Value->FindComponentByClass<UPathFollowingComponent>())
		{
			return PathComponent->DidMoveReachGoal();
		}

		return false;
	});

	ScratchPad->ActivePhysicsMoves.RemoveAll([&](const TWeakObjectPtr<AActor>& LHS)
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
	return ScratchPad->ActiveMoveRequests.Num() > 0 || ScratchPad->ActivePhysicsMoves.Num() > 0 || ScratchPad->AsyncQueryIdArray.Num() > 0;
}

FVector UAblMoveToTask::GetTargetLocation(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	if (m_TargetType.GetValue() == EAblMoveToTarget::MTT_Actor)
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
			return Context->GetSelfActor()->GetActorLocation();
		}

		return actor->GetActorLocation();
	}
	else
	{
		return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TargetLocation);
	}
}

void UAblMoveToTask::StartPathFinding(const TWeakObjectPtr<const UAblAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAblMoveToScratchPad* ScratchPad) const
{
	if (UPathFollowingComponent* PathFindingComponent = Target->FindComponentByClass<UPathFollowingComponent>())
	{
		if (APawn* Pawn = Cast<APawn>(Target))
		{
			if (AController* Controller = Pawn->GetController())
			{
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld());
				const ANavigationData* NavData = NavSys->GetNavDataForProps(Controller->GetNavAgentPropertiesRef());
				if (NavData)
				{
					FPathFindingQuery Query(Controller, *NavData, Controller->GetNavAgentLocation(), ScratchPad->CurrentTargetLocation);
					if (m_UseAsyncNavPathFinding)
					{
						if (!ScratchPad->NavPathDelegate.IsBound())
						{
							ScratchPad->NavPathDelegate.BindUObject(ScratchPad, &UAblMoveToScratchPad::OnNavPathQueryFinished);
						}

						// Async Query, queue it up and wait for results.
						int Id = NavSys->FindPathAsync(FNavAgentProperties(Controller->GetNavAgentPropertiesRef().AgentRadius, Controller->GetNavAgentPropertiesRef().AgentHeight), Query, ScratchPad->NavPathDelegate, m_NavPathFindingType.GetValue() == EAblPathFindingType::Regular ? EPathFindingMode::Regular : EPathFindingMode::Hierarchical);
						ScratchPad->AsyncQueryIdArray.Add(TPair<uint32, TWeakObjectPtr<APawn>>(Id, Pawn));
					}
					else
					{
						FPathFindingResult result = NavSys->FindPathSync(Query, m_NavPathFindingType.GetValue() == EAblPathFindingType::Regular ? EPathFindingMode::Regular : EPathFindingMode::Hierarchical);
						if (result.IsPartial() && m_CancelOnNoPathAvailable)
						{
#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Target %s was only able to find a partial path. Skipping."), *Pawn->GetName()));
							}
#endif
						}
						else
						{
							// Start the move.
							FAIRequestID MoveRequest = PathFindingComponent->RequestMove(FAIMoveRequest(ScratchPad->CurrentTargetLocation), result.Path);
							ScratchPad->ActiveMoveRequests.Add(TPair<FAIRequestID, TWeakObjectPtr<APawn>>(MoveRequest, Pawn));

#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Requested Move for Target %s."), *Pawn->GetName()));
							}
#endif
						}
					}
				}
			}
			else
			{
				UE_LOG(LogAble, Warning, TEXT("Actor %s wants to use Nav Pathing but has no controller? "), *Target->GetName());
			}
		}
		else
		{
			UE_LOG(LogAble, Warning, TEXT("Actor %s wants to use Nav Pathing but is not a Pawn. Please set the Actor to inherit from APawn. "), *Target->GetName());
		}
	}
	else
	{
		UE_LOG(LogAble, Warning, TEXT("Actor %s wants to use Nav Pathing but has no PathFollowingComponent! Please add one."), *Target->GetName());
	}
}

void UAblMoveToTask::SetPhysicsVelocity(const TWeakObjectPtr<const UAblAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAblMoveToScratchPad* ScratchPad) const
{
	FVector towardsTarget = EndLocation - Target->GetActorLocation();
	float towardsLengthSqr = towardsTarget.Size2D();
	towardsTarget.Normalize();
	FVector reqVelocity = towardsTarget * ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Speed) * FMath::Min(1.0f, towardsLengthSqr);

	if (UCharacterMovementComponent* CharacterMovementComponent = Target->FindComponentByClass<UCharacterMovementComponent>())
	{
		CharacterMovementComponent->RequestDirectMove(reqVelocity, false);
		ScratchPad->ActivePhysicsMoves.Add(Target);

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on CharacterMovementComponent to %s"), *Target->GetName(), *reqVelocity.ToCompactString()));
		}
#endif
	}
	else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		PrimitiveComponent->SetPhysicsLinearVelocity(reqVelocity);
		ScratchPad->ActivePhysicsMoves.Add(Target);

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on Primitive Root Component to %s"), *Target->GetName(), *reqVelocity.ToCompactString()));
		}
#endif
	}
	else
	{
		UE_LOG(LogAble, Warning, TEXT("Actor %s doesn't have a Character Movement Component or Primitive Root component. Unable to set velocity."), *Target->GetName());
	}
}

void UAblMoveToScratchPad::OnNavPathQueryFinished(uint32 Id, ENavigationQueryResult::Type typeData, FNavPathSharedPtr PathPtr)
{
	CompletedAsyncQueries.Add(TPair<uint32, FNavPathSharedPtr>(Id, PathPtr));
}

#if WITH_EDITOR

FText UAblMoveToTask::GetDescriptiveTaskName() const
{
	return FText::FormatOrdered(LOCTEXT("AblMoveToTaskDesc", "Move to {0}"), FText::FromString(FAbleLogHelper::GetTargetTargetEnumAsString(m_TargetActor.GetValue())));
}

#endif 

#undef LOCTEXT_NAMESPACE