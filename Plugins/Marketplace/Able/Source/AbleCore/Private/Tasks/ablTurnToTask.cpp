// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablTurnToTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"

#if (!UE_BUILD_SHIPPING)
#include "ablAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblTurnToTaskScratchPad::UAblTurnToTaskScratchPad()
{

}

UAblTurnToTaskScratchPad::~UAblTurnToTaskScratchPad()
{

}

UAblTurnToTask::UAblTurnToTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_RotationTarget(EAblAbilityTargetType::ATT_TargetActor),
	m_UseRotationVector(false),
	m_RotationVector(ForceInitToZero),
	m_TrackTarget(false),
	m_SetYaw(true),
	m_SetPitch(false),
	m_Blend(GetDuration()),
	m_UseTaskDurationAsBlendTime(true),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_ClientAndServer)
{

}

UAblTurnToTask::~UAblTurnToTask()
{

}

void UAblTurnToTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblTurnToTaskScratchPad* ScratchPad = Cast<UAblTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	ScratchPad->InProgressTurn.Empty();
	ScratchPad->TurningBlend = m_Blend;

	if (m_UseTaskDurationAsBlendTime)
	{
		ScratchPad->TurningBlend.SetBlendTime(GetDuration());
	}

	AActor* TargetActor = GetSingleActorFromTargetType(Context, m_RotationTarget.GetValue());

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	for (TWeakObjectPtr<AActor>& TurnTarget : TaskTargets)
	{
		FRotator TargetRotation = GetTargetRotation(Context, TurnTarget.Get(), TargetActor);
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Setting up turning for Actor %s with a Target turn of %s."), *TurnTarget->GetName(), *TargetRotation.ToCompactString()));
		}
#endif
		ScratchPad->InProgressTurn.Add(FTurnToTaskEntry(TurnTarget.Get(), TargetRotation));
	}

}

void UAblTurnToTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

	UAblTurnToTaskScratchPad* ScratchPad = Cast<UAblTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

	ScratchPad->TurningBlend.Update(deltaTime);
	const float BlendingValue = ScratchPad->TurningBlend.GetBlendedValue();
	AActor* TargetActor = GetSingleActorFromTargetType(Context, m_RotationTarget.GetValue());

	for (FTurnToTaskEntry& Entry : ScratchPad->InProgressTurn)
	{
		if (Entry.Actor.IsValid())
		{
			if (m_TrackTarget)
			{
				// Update our Target rotation.
				Entry.Target = GetTargetRotation(Context, Entry.Actor.Get(), TargetActor);
			}

			FRotator LerpedRotation = FMath::Lerp(Entry.Actor->GetActorRotation(), Entry.Target, BlendingValue);
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Setting Actor %s rotation to %s ."), *Entry.Actor->GetName(), *LerpedRotation.ToCompactString()));
			}
#endif
			Entry.Actor->SetActorRotation(LerpedRotation);
		}
	}
}

void UAblTurnToTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (!Context.IsValid())
	{
		return;
	}

	UAblTurnToTaskScratchPad* ScratchPad = Cast<UAblTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

	for (const FTurnToTaskEntry& Entry : ScratchPad->InProgressTurn)
	{
		if (Entry.Actor.IsValid())
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Setting Actor %s rotation to %s ."), *Entry.Actor->GetName(), *Entry.Target.ToCompactString()));
			}
#endif
			Entry.Actor->SetActorRotation(Entry.Target);
		}
	}
}

UAblAbilityTaskScratchPad* UAblTurnToTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblTurnToTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblTurnToTaskScratchPad>(Context.Get());
}

TStatId UAblTurnToTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblTurnToTask, STATGROUP_Able);
}

void UAblTurnToTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_UseRotationVector, "Use Rotation Vector");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_RotationVector, "Rotation Vector");
}

FRotator UAblTurnToTask::GetTargetRotation(const TWeakObjectPtr<const UAblAbilityContext>& Context, const AActor* Source, const AActor* Destination) const
{
	float Yaw = 0.0f;
	float Pitch = 0.0f;

	FVector ToTarget = GetTargetVector(Context, Source, Destination);
	ToTarget.Normalize();

	FVector2D YawPitch = ToTarget.UnitCartesianToSpherical();

	if (m_SetYaw)
	{
		Yaw = FMath::RadiansToDegrees(YawPitch.Y);
	}

	if (m_SetPitch)
	{
		Pitch = FMath::RadiansToDegrees(YawPitch.X);
	}

	FRotator OutRotator(Pitch, Yaw, 0.0f);

	return OutRotator + m_RotationOffset;
}

FVector UAblTurnToTask::GetTargetVector(const TWeakObjectPtr<const UAblAbilityContext>& Context, const AActor* Source, const AActor* Destination) const
{
	bool useVector = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_UseRotationVector);

	if (useVector)
	{
		return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_RotationVector);
	}

	if (Source && Destination)
	{
		return Destination->GetActorLocation() - Source->GetActorLocation();
	}

	return FVector::ZeroVector;
}

#if WITH_EDITOR

FText UAblTurnToTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblTurnToTaskFormat", "{0}: {1}");
	FString TargetName = FAbleLogHelper::GetTargetTargetEnumAsString(m_RotationTarget);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

void UAblTurnToTask::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (GetDuration() < m_Blend.GetBlendTime())
	{
		m_EndTime = GetStartTime() + m_Blend.GetBlendTime();
	}
}

#endif

#undef LOCTEXT_NAMESPACE