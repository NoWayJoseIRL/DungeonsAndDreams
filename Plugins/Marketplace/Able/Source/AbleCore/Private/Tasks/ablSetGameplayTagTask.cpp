// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablSetGameplayTagTask.h"

#include "ablAbilityComponent.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblSetGameplayTagTaskScratchPad::UAblSetGameplayTagTaskScratchPad()
{

}

UAblSetGameplayTagTaskScratchPad::~UAblSetGameplayTagTaskScratchPad()
{

}

UAblSetGameplayTagTask::UAblSetGameplayTagTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TagList(),
	m_RemoveOnEnd(false),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server)
{

}

UAblSetGameplayTagTask::~UAblSetGameplayTagTask()
{

}

void UAblSetGameplayTagTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	UAblSetGameplayTagTaskScratchPad* Scratchpad = nullptr;
	if (m_RemoveOnEnd)
	{
		Scratchpad = Cast<UAblSetGameplayTagTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(Scratchpad);
		Scratchpad->TaggedActors.Empty();
	}

	for (const TWeakObjectPtr<AActor> & Actor : TaskTargets)
	{
		if (Actor.IsValid())
		{
			if (UAblAbilityComponent* AbilityComponent = Actor->FindComponentByClass<UAblAbilityComponent>())
			{
				for (const FGameplayTag& Tag : m_TagList)
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Adding Tag %s to Actor %s."), *Actor->GetName(), *Tag.ToString()));
					}
#endif
					AbilityComponent->AddTag(Tag);
				}

				if (Scratchpad)
				{
					Scratchpad->TaggedActors.Add(Actor);
				}
			}
		}
	}
}

void UAblSetGameplayTagTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (m_RemoveOnEnd && Context.IsValid())
	{
		UAblSetGameplayTagTaskScratchPad* Scratchpad = Cast<UAblSetGameplayTagTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(Scratchpad);

		for (const TWeakObjectPtr<AActor> & Actor : Scratchpad->TaggedActors)
		{
			if (Actor.IsValid())
			{
				if (UAblAbilityComponent* AbilityComponent = Actor->FindComponentByClass<UAblAbilityComponent>())
				{
					for (const FGameplayTag& Tag : m_TagList)
					{
#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Removing Tag %s from Actor %s."), *Actor->GetName(), *Tag.ToString()));
						}
#endif
						AbilityComponent->RemoveTag(Tag);
					}
				}
			}
		}
	}
}

UAblAbilityTaskScratchPad* UAblSetGameplayTagTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (m_RemoveOnEnd)
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblSetGameplayTagTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblSetGameplayTagTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblSetGameplayTagTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblSetGameplayTagTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblSetGameplayTagTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlaySetGameplayTagTaskFormat", "{0}: {1}");
	FString GameplayTagNames = TEXT("<none>");
	if (m_TagList.Num())
	{
		if (m_TagList.Num() == 1)
		{
			GameplayTagNames = m_TagList[0].ToString();
		}
		else
		{
			GameplayTagNames = FString::Printf(TEXT("%s, ... (%d Total Tags)"), *m_TagList[0].ToString(), m_TagList.Num());
		}

	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(GameplayTagNames));
}

#endif

#undef LOCTEXT_NAMESPACE
