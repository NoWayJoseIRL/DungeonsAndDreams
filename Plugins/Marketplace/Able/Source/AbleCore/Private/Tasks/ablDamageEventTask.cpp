// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablDamageEventTask.h"

#include "ablAbility.h"
#include "ablAbilityUtilities.h"
#include "AbleCorePrivate.h"
#include "ablSettings.h"
#include "Async/Async.h"
#include "GameFramework/DamageType.h"
#include "Tasks/ablValidation.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblDamageEventTask::UAblDamageEventTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Damage(1.0f),
	m_DamageSource(EAblAbilityTargetType::ATT_Self),
	m_EventName(NAME_None),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_ClientAndServer)
{

}

UAblDamageEventTask::~UAblDamageEventTask()
{

}

void UAblDamageEventTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	check(Context.IsValid());

	Super::OnTaskStart(Context);

	AActor* DamageSource = GetSingleActorFromTargetType(Context, m_DamageSource);
		
	TArray<TWeakObjectPtr<AActor>> DamageTargets;
	GetDamageTargets(Context, DamageTargets);

	FDamageEvent DamageEvent;
	DamageEvent.DamageTypeClass = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_DamageClass);

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Executing Damage calculations for %d targets."), DamageTargets.Num()));
	}
#endif

	if (DamageTargets.Num())
	{
		TArray<float> DamageValues;
		DamageValues.Reserve(DamageTargets.Num());

		if (m_UseAsyncCalculate && UAbleSettings::IsAsyncEnabled())
		{
			TArray<TFuture<float>> CalculateTasks;
			CalculateTasks.Reserve(DamageTargets.Num());

			for (TWeakObjectPtr<AActor>& DamageTarget : DamageTargets)
			{
				// Copy over to local variables for the Lambda.
				float BaseDamage = m_Damage;
				FName EventName = m_EventName;

				CalculateTasks.Add(Async(EAsyncExecution::TaskGraph, [&Context, BaseDamage, EventName, &DamageTarget, &DamageEvent]
				{
					if (Context.IsValid())
					{
						return Context->GetAbility()->CalculateDamageForActorBP(Context.Get(), EventName, BaseDamage, DamageTarget.Get(), DamageEvent);
					}

					return BaseDamage;
				}));
			}

			for (TFuture<float>& Future : CalculateTasks)
			{
				if (!Future.IsReady())
				{
					static const FTimespan OneMillisecond = FTimespan::FromMilliseconds(1.0);
					Future.WaitFor(OneMillisecond);
				}

				// Copy the Damage over.
				DamageValues.Add(Future.Get());
			}
		}
		else
		{
			for (TWeakObjectPtr<AActor>& DamageTarget : DamageTargets)
			{
				DamageValues.Add(Context->GetAbility()->CalculateDamageForActorBP(Context.Get(), m_EventName, m_Damage, DamageTarget.Get(), DamageEvent));

#if !(UE_BUILD_SHIPPING)
				if (IsVerbose() && DamageTarget.IsValid())
				{
					PrintVerbose(Context, FString::Printf(TEXT("CalculateDamageForActor with Actor %s and Base Damage %4.2f returned %4.2f."), *DamageTarget->GetName(), m_Damage, DamageValues[DamageValues.Num() - 1]));
				}
#endif
			}
		}

		check(DamageTargets.Num() == DamageValues.Num());



		AController* InstigatorController = nullptr;
		if (AActor* InstigatorActor = Context->GetInstigator())
		{
			if (InstigatorActor->IsA<AController>())
			{
				InstigatorController = Cast<AController>(InstigatorActor);
			}
			else if (InstigatorActor->IsA<APawn>())
			{
				InstigatorController = Cast<APawn>(InstigatorActor)->GetController();
			}
		}

		if (DamageEvent.DamageTypeClass)
		{
			if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
			{
				FPointDamageEvent PointEvent;
				FVector ShotDirection;
				for (int32 i = 0; i < DamageTargets.Num(); ++i)
				{
					if (DamageTargets[i].IsValid())
					{
						if (DamageSource != nullptr)
						{
							ShotDirection = DamageTargets[i]->GetActorLocation() - DamageSource->GetActorLocation();
							ShotDirection.Normalize();
							PointEvent.ShotDirection = ShotDirection;
						}

						PointEvent.Damage = DamageValues[i];

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Applying %4.2f damage to %s as Point Damage, Damage Source[%s], Shot Direction [%s]."), 
							DamageValues[i], *DamageTargets[i]->GetName(), DamageSource ? *DamageSource->GetName() : TEXT("NULL"), *PointEvent.ShotDirection.ToString()));
						}
#endif

						DamageTargets[i]->TakeDamage(DamageValues[i], *(FDamageEvent*)&PointEvent, InstigatorController, DamageSource);
					}
				}

				return;
			}
			else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
			{
				FRadialDamageEvent RadialEvent;
				if (DamageSource != nullptr)
				{
					RadialEvent.Origin = DamageSource->GetActorLocation();
				}
				RadialEvent.Params = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_RadialParams);
				for (int32 i = 0; i < DamageTargets.Num(); ++i)
				{
					if (DamageTargets[i].IsValid())
					{

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Applying %4.2f damage to %s as Radial Damage, Damage Source[%s], Damage Origin [%s]."),
								DamageValues[i], *DamageTargets[i]->GetName(), DamageSource ? *DamageSource->GetName() : TEXT("NULL"), *RadialEvent.Origin.ToString()));
						}
#endif

						DamageTargets[i]->TakeDamage(DamageValues[i], *(FDamageEvent*)&RadialEvent, InstigatorController, DamageSource);
					}
				}

				return;
			}
			// Some custom event we don't know. Fall through.
		}

		// Generic event / catch-all
		for (int32 i = 0; i < DamageTargets.Num(); ++i)
		{
			if (DamageTargets[i].IsValid())
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Applying %4.2f damage to %s."), DamageValues[i], *DamageTargets[i]->GetName()));
				}
#endif
				DamageTargets[i]->TakeDamage(DamageValues[i], DamageEvent, InstigatorController, DamageSource);
			}
		}

	}
}

void UAblDamageEventTask::BindDynamicDelegates(UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_DamageClass, TEXT("Damage Class"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_RadialParams, TEXT("Radial Damage Info"));
}

TStatId UAblDamageEventTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblDamageEventTask, STATGROUP_Able);
}

void UAblDamageEventTask::GetDamageTargets(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<TWeakObjectPtr<AActor>>& OutArray) const
{
	for (const EAblAbilityTargetType Target : m_DamageTargets)
	{
		switch (Target)
		{
			case EAblAbilityTargetType::ATT_Self:
			case EAblAbilityTargetType::ATT_Owner:
			case EAblAbilityTargetType::ATT_Instigator:
			{
				if (AActor* Actor = GetSingleActorFromTargetType(Context, Target))
				{
					OutArray.Add(Actor);
				}
			}
			break;
			case EAblAbilityTargetType::ATT_TargetActor:
			{
				const TArray<TWeakObjectPtr<AActor>>& UnfilteredTargets = Context->GetTargetActorsWeakPtr();
				for (const TWeakObjectPtr<AActor>& TempTarget : UnfilteredTargets)
				{
					if (IsTaskValidForActor(TempTarget.Get()))
					{
						OutArray.Add(TempTarget);
					}
				}
			}
			break;
			default:
			break;
		}
	}
}

#if WITH_EDITOR

FText UAblDamageEventTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblDamageEventTaskFormat", "{0}: {1}");
	FString TargetNames = TEXT("<None>");
	if (m_DamageTargets.Num())
	{
		TargetNames = FAbleLogHelper::GetTargetTargetEnumAsString(m_DamageTargets[0]);
		if (m_DamageTargets.Num() > 1)
		{
			TargetNames.Append(FString::Printf(TEXT(", ... (%d Total Targets)"), m_DamageTargets.Num()));
		}

	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetNames));
}

EDataValidationResult UAblDamageEventTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

	// Disabling this for now. It seems to cause more confusion than it fixes, but we'll see if people start complaining about not finding targets.
    //if (m_DamageSource == EAblAbilityTargetType::ATT_TargetActor || m_DamageTargets.Contains(EAblAbilityTargetType::ATT_TargetActor))
    //{
    //    if (AbilityContext->RequiresTarget())
    //    {
    //        // trust that the ability cannot run unless it has a target, so don't do any dependency validation
    //        if (AbilityContext->GetTargeting() == nullptr)
    //        {
    //            ValidationErrors.Add(FText::Format(LOCTEXT("NoTargeting", "No Targeting method Defined: {0} with RequiresTarget"), AssetName));
    //            result = EDataValidationResult::Invalid;
    //        }
    //    }
    //    else if (AbilityContext->GetTargeting() == nullptr)
    //    {
    //        // if we have a target actor, we should have a dependency task that copies results
    //        bool hasValidDependency = UAbleValidation::CheckDependencies(GetTaskDependencies());            
    //        if (!hasValidDependency)
    //        {
    //            ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryDependency", "Trying to use Target Actor but there's no Ability Targeting or Query( with GetCopyResultsToContext )and a Dependency Defined on this Task: {0}. You need one of those conditions to properly use this target."), FText::FromString(AbilityContext->GetDisplayName())));
    //            result = EDataValidationResult::Invalid;
    //        }
    //    }
    //}

    return result;
}

#endif

#undef LOCTEXT_NAMESPACE
