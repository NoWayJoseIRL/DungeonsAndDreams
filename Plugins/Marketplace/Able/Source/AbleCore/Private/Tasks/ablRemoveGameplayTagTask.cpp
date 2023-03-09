// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablRemoveGameplayTagTask.h"

#include "ablAbilityComponent.h"
#include "AbleCorePrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblRemoveGameplayTagTask::UAblRemoveGameplayTagTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TaskRealm(EAblAbilityTaskRealm::ATR_Server)
{

}

UAblRemoveGameplayTagTask::~UAblRemoveGameplayTagTask()
{

}

void UAblRemoveGameplayTagTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

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
						PrintVerbose(Context, FString::Printf(TEXT("Removing Tag %s from Actor %s."), *Tag.ToString(), *Actor->GetName()));
					}
#endif
					AbilityComponent->RemoveTag(Tag);
				}
			}
		}
	}
}

TStatId UAblRemoveGameplayTagTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblRemoveGameplayTagTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblRemoveGameplayTagTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlayRemoveGameplayTagTaskFormat", "{0}: {1}");
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
