// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablSpawnActorTask.h"

#include "ablAbility.h"
#include "ablAbilityContext.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblSpawnActorTaskScratchPad::UAblSpawnActorTaskScratchPad()
{

}

UAblSpawnActorTaskScratchPad::~UAblSpawnActorTaskScratchPad()
{

}

UAblSpawnActorTask::UAblSpawnActorTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_ActorClass(),
	m_AmountToSpawn(1),
	m_SpawnCollision(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn),
	m_InitialVelocity(ForceInitToZero),
	m_SetOwner(true),
	m_OwnerTargetType(EAblAbilityTargetType::ATT_Self),
	m_AttachToOwnerSocket(false),
	m_AttachmentRule(EAttachmentRule::KeepRelative),
	m_SocketName(NAME_None),
	m_InheritOwnerLinearVelocity(false),
	m_MarkAsTransient(true),
	m_DestroyAtEnd(false),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server)
{

}

UAblSpawnActorTask::~UAblSpawnActorTask()
{

}

void UAblSpawnActorTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	check(Context.IsValid());

	Super::OnTaskStart(Context);

	TArray<TWeakObjectPtr<AActor>> OutActors;
	GetActorsForTask(Context, OutActors);

	if (OutActors.Num() == 0)
	{
		// Just incase, to not break previous content.
		OutActors.Add(Context->GetSelfActor());
	}

	TSubclassOf<AActor> ActorClass = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_ActorClass);
	int NumToSpawn = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AmountToSpawn);
	FAblAbilityTargetTypeLocation SpawnTargetLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SpawnLocation);

	if (!ActorClass.GetDefaultObject())
	{
		UE_LOG(LogAble, Warning, TEXT("SpawnActorTask for Ability [%s] does not have a class specified."), *(Context->GetAbility()->GetDisplayName()));
		return;
	}

	UAblSpawnActorTaskScratchPad* ScratchPad = nullptr;
	if (m_DestroyAtEnd)
	{
		ScratchPad = Cast<UAblSpawnActorTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		ScratchPad->SpawnedActors.Empty();
	}
	
	FTransform SpawnTransform;
	SpawnTargetLocation.GetTransform(*Context.Get(), SpawnTransform);

	FVector SpawnLocation;

	for (int32 i = 0; i < OutActors.Num(); ++i)
	{
		UWorld* ActorWorld = OutActors[i]->GetWorld();
		FActorSpawnParameters SpawnParams;

		if (SpawnTargetLocation.GetSourceTargetType() == EAblAbilityTargetType::ATT_TargetActor)
		{
			SpawnTargetLocation.GetTargetTransform(*Context.Get(), i, SpawnTransform);
		}

		if (m_SetOwner)
		{
			SpawnParams.Owner = GetSingleActorFromTargetType(Context, m_OwnerTargetType);
		}

		SpawnParams.SpawnCollisionHandlingOverride = m_SpawnCollision;

		if (m_MarkAsTransient)
		{
			SpawnParams.ObjectFlags = EObjectFlags::RF_Transient; // We don't want to save anything on this object.
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Spawning Actor %s using Transform %s."), *ActorClass->GetName(), *SpawnTransform.ToString()));
		}
#endif
		// Go through our spawns.
		for (int32 SpawnIndex = 0; SpawnIndex < NumToSpawn; ++SpawnIndex)
		{
			SpawnParams.Name = MakeUniqueObjectName(ActorWorld, ActorClass);

			AActor* SpawnedActor = ActorWorld->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);
			if (!SpawnedActor)
			{
				UE_LOG(LogAble, Warning, TEXT("Failed to spawn Actor %s using Transform %s."), *ActorClass->GetName(), *SpawnTransform.ToString());
				return;
			}

			FVector InheritedVelocity;
			if (m_InheritOwnerLinearVelocity && SpawnedActor->GetOwner())
			{
				InheritedVelocity = SpawnedActor->GetOwner()->GetVelocity();
			}

			if (!m_InitialVelocity.IsNearlyZero())
			{
				// Use the Projectile Movement Component if they have one setup since this is likely used for spawning projectiles.
				if (UProjectileMovementComponent* ProjectileComponent = SpawnedActor->FindComponentByClass<UProjectileMovementComponent>())
				{
					ProjectileComponent->SetVelocityInLocalSpace(m_InitialVelocity + InheritedVelocity);
				}
				else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SpawnedActor->GetRootComponent()))
				{
					PrimitiveComponent->AddImpulse(m_InitialVelocity + InheritedVelocity);
				}
			}

			if (m_AttachToOwnerSocket)
			{
				if (USkeletalMeshComponent* SkeletalComponent = SpawnedActor->GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
				{
					if (USceneComponent* SceneComponent = SpawnedActor->FindComponentByClass<USceneComponent>())
					{
						FAttachmentTransformRules AttachRules(m_AttachmentRule, false);
						SceneComponent->AttachToComponent(SkeletalComponent, AttachRules, m_SocketName);
					}
				}
			}

			if (m_DestroyAtEnd)
			{
				ScratchPad->SpawnedActors.Add(SpawnedActor);
			}

			if (m_FireEvent)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Calling OnSpawnedActorEvent with event name %s, Spawned Actor %s and Spawn Index %d."), *m_Name.ToString(), *SpawnedActor->GetName(), SpawnIndex));
				}
#endif
				Context->GetAbility()->OnSpawnedActorEventBP(Context.Get(), m_Name, SpawnedActor, SpawnIndex);
			}
		}
	}

}

void UAblSpawnActorTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (m_DestroyAtEnd && Context.IsValid())
	{
		UAblSpawnActorTaskScratchPad* ScratchPad = Cast<UAblSpawnActorTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);

		if (ScratchPad->SpawnedActors.Num())
		{
			for (AActor* SpawnedActor : ScratchPad->SpawnedActors)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Destroying Spawned Actor %s."), *SpawnedActor->GetName()));
				}
#endif
				UWorld* SpawnedWorld = SpawnedActor->GetWorld();
				SpawnedWorld->DestroyActor(SpawnedActor);
			}

			ScratchPad->SpawnedActors.Empty();
		}
	}
}

UAblAbilityTaskScratchPad* UAblSpawnActorTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (m_DestroyAtEnd)
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblSpawnActorTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblSpawnActorTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblSpawnActorTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblSpawnActorTask, STATGROUP_Able);
}

void UAblSpawnActorTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ActorClass, TEXT("Actor Class"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AmountToSpawn, TEXT("Amount to Spawn"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SpawnLocation, TEXT("Location"));
}

#if WITH_EDITOR

FText UAblSpawnActorTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblSpawnActorTaskFormat", "{0}: {1} x{2}");
	FString ActorName = TEXT("<null>");
	if (*m_ActorClass)
	{
		if (AActor* Actor = Cast<AActor>(m_ActorClass->GetDefaultObject()))
		{
			ActorName = Actor->GetName();
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ActorName), m_AmountToSpawn);
}

FText UAblSpawnActorTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString ActorName;
	if (m_ActorClassDelegate.IsBound())
	{
		ActorName = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_ActorClassDelegate.GetFunctionName().ToString() });
	}
	else
	{
		ActorName = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_ActorClass\" Filter=\"/Script/Engine.Actor,/Script/Engine.Blueprint\">{0}</>"), { m_ActorClass ? m_ActorClass->GetName() : TEXT("NULL") });
	}

	FString AmountToSpawnString;
	if (m_AmountToSpawnDelegate.IsBound())
	{
		AmountToSpawnString = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AmountToSpawnDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AmountToSpawnString = FString::Format(TEXT("<a id=\"AblTextDecorators.IntValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_AmountToSpawn\" MinValue=\"1\">{0}</>"), { m_AmountToSpawn });
	}

	StringBuilder.AppendLineFormat(LOCTEXT("AblSpawnActorTaskRichFmt", "\t- Actor Class: {0}\n\t- Amount to Spawn: {1}"), FText::FromString(ActorName), FText::FromString(AmountToSpawnString));

	return StringBuilder.ToText();
}

EDataValidationResult UAblSpawnActorTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_ActorClass == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoClassDefined", "No Actor Class defined: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnSpawnedActorEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnSpawnedActorEventBP_NotFound", "Function 'OnSpawnedActorEventBP' not found: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }

    return result;
}

#endif

#undef LOCTEXT_NAMESPACE