// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCancelAbilityTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablAbilityContext.h"
#include "AbleCorePrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCancelAbilityTask::UAblCancelAbilityTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Ability(),
	m_TagQuery(),
	m_PassiveBehavior(RemoveOneStack),
	m_CancelResult(Interrupted),
	m_EventName(NAME_None),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server)
{

}

UAblCancelAbilityTask::~UAblCancelAbilityTask()
{

}

void UAblCancelAbilityTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	check(Context.IsValid());

	Super::OnTaskStart(Context);

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	for (TWeakObjectPtr<AActor> TargetActor : TaskTargets)
	{
		if (!TargetActor.IsValid())
		{
			continue;
		}

		if (UAblAbilityComponent* AbilityComponent = TargetActor->FindComponentByClass<UAblAbilityComponent>())
		{
			if (const UAblAbility* ActiveAbility = AbilityComponent->GetActiveAbility())
			{
				if (ShouldCancelAbility(*ActiveAbility, *Context.Get()))
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Cancelling Ability %s on Actor %s."), *ActiveAbility->GetDisplayName(),
							*TargetActor->GetName()));
					}
#endif
					AbilityComponent->CancelAbility(ActiveAbility, m_CancelResult.GetValue());
				}
			}

			TArray<UAblAbility*> CurrentPassives;
			AbilityComponent->GetCurrentPassiveAbilities(CurrentPassives);

			for (UAblAbility* Passive : CurrentPassives)
			{
				if (ShouldCancelAbility(*Passive, *Context.Get()))
				{
					switch (m_PassiveBehavior.GetValue())
					{
					case RemoveOneStack:
					case RemoveOneStackWithRefresh:
					{
						int32 StackCount = AbilityComponent->GetCurrentStackCountForPassiveAbility(Passive);
						int32 NewStackCount = FMath::Max(StackCount - 1, 0);

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Setting Passive Ability %s Stack on Actor %s from %d to %d."), *Passive->GetDisplayName(),
								*TargetActor->GetName(), StackCount, NewStackCount));
						}
#endif

						AbilityComponent->SetPassiveStackCount(Passive, NewStackCount, m_PassiveBehavior.GetValue() == RemoveOneStackWithRefresh, m_CancelResult.GetValue());
					}
					break;
					case RemoveEntireStack:
					default:
					{
#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Cancelling Passive Ability %s on Actor %s."), *Passive->GetDisplayName(),
								*TargetActor->GetName()));
						}
#endif
						AbilityComponent->CancelAbility(Passive, m_CancelResult.GetValue());
					}
					break;
					}
				}
			}
		}
	}

}

TStatId UAblCancelAbilityTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblCancelAbilityTask, STATGROUP_Able);
}

void UAblCancelAbilityTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Ability, TEXT("Ability"));
}

bool UAblCancelAbilityTask::ShouldCancelAbility(const UAblAbility& Ability, const UAblAbilityContext& Context) const
{
	// Give BP the first shot.
	if (Ability.ShouldCancelAbilityBP(&Context, &Ability, m_EventName))
	{
		return true;
	}

	if (*m_Ability && m_Ability->GetDefaultObject<UAblAbility>()->GetAbilityNameHash() == Ability.GetAbilityNameHash())
	{
		return true;
	}

	if (!m_TagQuery.IsEmpty())
	{
		return Ability.GetAbilityTagContainer().MatchesQuery(m_TagQuery);
	}

	return false;
}

#if WITH_EDITOR

FText UAblCancelAbilityTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblCancelAbilityTaskFormat", "{0}: {1}");
	FString AbilityName = TEXT("<null>");
	if (*m_Ability)
	{
		if (UAblAbility* Ability = Cast<UAblAbility>(m_Ability->GetDefaultObject()))
		{
			AbilityName = Ability->GetDisplayName();
		}
	}
	else if (!m_TagQuery.IsEmpty())
	{
		AbilityName = m_TagQuery.GetDescription();
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AbilityName));
}

FText UAblCancelAbilityTask::GetRichTextTaskSummary() const
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
	StringBuilder.AppendLineFormat(LOCTEXT("AblCancelAbilityTaskRichFmt", "\t- Ability: {0}"), FText::FromString(AbilityName));

	return StringBuilder.ToText();
}

#endif

#undef LOCTEXT_NAMESPACE