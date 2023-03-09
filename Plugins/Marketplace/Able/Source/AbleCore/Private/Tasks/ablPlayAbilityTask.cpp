// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPlayAbilityTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablAbilityContext.h"
#include "ablAbilityTypes.h"
#include "AbleCorePrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblPlayAbilityTask::UAblPlayAbilityTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Ability(),
	m_Owner(EAblAbilityTargetType::ATT_Owner),
	m_Instigator(EAblAbilityTargetType::ATT_Self),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server),
	m_CopyTargets(false)
{

}

UAblPlayAbilityTask::~UAblPlayAbilityTask()
{

}

void UAblPlayAbilityTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	check(Context.IsValid());

	Super::OnTaskStart(Context);

	TSubclassOf<UAblAbility> AbilityClass = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Ability);

	if (const UAblAbility* Ability = Cast<const UAblAbility>(AbilityClass->GetDefaultObject()))
	{
		EAblAbilityTargetType Instigator = ABL_GET_DYNAMIC_PROPERTY_VALUE_ENUM(Context, m_Instigator);
		EAblAbilityTargetType Owner = ABL_GET_DYNAMIC_PROPERTY_VALUE_ENUM(Context, m_Owner);

		TArray<TWeakObjectPtr<AActor>> TaskTargets;
		GetActorsForTask(Context, TaskTargets);

		AActor* InstigatorActor = GetSingleActorFromTargetType(Context, Instigator);
		AActor* OwnerActor = GetSingleActorFromTargetType(Context, Owner);
		UAblAbilityComponent* AbilityComponent = nullptr;
		
		for (const TWeakObjectPtr<AActor>& TaskTarget : TaskTargets)
		{
			if (TaskTarget.IsValid())
			{
				// Target Actor with GetSingleActorFromTargetType always returns the first entry, so we need to
				// update the value as we work if we're set to use that Target Type.
				if (Instigator == EAblAbilityTargetType::ATT_TargetActor)
				{
					InstigatorActor = TaskTarget.Get();
				}

				if (Owner == EAblAbilityTargetType::ATT_TargetActor)
				{
					OwnerActor = TaskTarget.Get();
				}

				AbilityComponent = TaskTarget->FindComponentByClass<UAblAbilityComponent>();
				if (AbilityComponent)
				{
					UAblAbilityContext* NewContext = UAblAbilityContext::MakeContext(Ability, AbilityComponent, OwnerActor, InstigatorActor);
					bool CopyTargets = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_CopyTargets);
					if (CopyTargets)
					{
						NewContext->GetMutableTargetActors().Append(Context->GetTargetActors());
					}

#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						TStringBuilder<2048> TargetStringBuilder;
						if (NewContext->GetTargetActorsWeakPtr().Num() == 0)
						{
							TargetStringBuilder.Append(TEXT("None"));
						}
						else
						{
							for (const AActor* CurrentTarget : NewContext->GetTargetActors())
							{
								TargetStringBuilder.Appendf(TEXT(", %s"), *CurrentTarget->GetName());
							}
						}

						PrintVerbose(Context, FString::Printf(TEXT("Calling ActivateAbility with Actor %s with Ability %s, Owner %s, Instigator %s, Targets ( %s )."), *TaskTarget->GetName(),
							*Ability->GetDisplayName(), InstigatorActor ? *InstigatorActor->GetName() : *FString("None"), OwnerActor ? *OwnerActor->GetName() : *FString("None"), TargetStringBuilder.ToString()));
					}
#endif
					AbilityComponent->ActivateAbility(NewContext);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogAble, Warning, TEXT("No Ability set for PlayAbilityTask in Ability [%s]"), *Context->GetAbility()->GetDisplayName());
		return;
	}
}

TStatId UAblPlayAbilityTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblPlayAbilityTask, STATGROUP_Able);
}

void UAblPlayAbilityTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Ability, TEXT("Ability"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_CopyTargets, TEXT("Copy Targets"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Owner, TEXT("Owner"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Instigator, TEXT("Instigator"));
}

#if WITH_EDITOR

FText UAblPlayAbilityTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlayAbilityTaskFormat", "{0}: {1}");
	FString AbilityName = TEXT("<null>");
	if (*m_Ability)
	{
		if (UAblAbility* Ability = Cast<UAblAbility>(m_Ability->GetDefaultObject()))
		{
			AbilityName = Ability->GetDisplayName();
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AbilityName));
}

FText UAblPlayAbilityTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString AbilityName = TEXT("NULL");
	if (m_AbilityDelegate.IsBound())
	{
		AbilityName = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AbilityDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AbilityName = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Ability\" Filter=\"/Script/AbleCore.AblAbility\">{0}</>"), { m_Ability ? m_Ability->GetDefaultObjectName().ToString() : AbilityName });
	}
	StringBuilder.AppendLineFormat(LOCTEXT("AblPlayAbilityTaskRichFmt", "\t- Ability: {0}"), FText::FromString(AbilityName));

	return StringBuilder.ToText();
}

#endif

#undef LOCTEXT_NAMESPACE