// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablSetCollisionChannelTask.h"

#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Components/PrimitiveComponent.h"

#if !(UE_BUILD_SHIPPING)
#include "ablAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblSetCollisionChannelTaskScratchPad::UAblSetCollisionChannelTaskScratchPad()
{

}

UAblSetCollisionChannelTaskScratchPad::~UAblSetCollisionChannelTaskScratchPad()
{

}

UAblSetCollisionChannelTask::UAblSetCollisionChannelTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Channel(ECollisionChannel::ECC_Pawn),
	m_RestoreOnEnd(true)
{

}

UAblSetCollisionChannelTask::~UAblSetCollisionChannelTask()
{

}

void UAblSetCollisionChannelTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblSetCollisionChannelTaskScratchPad* ScratchPad = nullptr;
	if (m_RestoreOnEnd)
	{
		ScratchPad = Cast<UAblSetCollisionChannelTaskScratchPad>(Context->GetScratchPadForTask(this));
		ScratchPad->PrimitiveToChannelMap.Empty();
	}

	// We need to convert our Actors to primitive components.
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	TArray<TWeakObjectPtr<UPrimitiveComponent>> PrimitiveComponents;

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
		{
			PrimitiveComponents.AddUnique(PrimitiveComponent);
		}
	}

	for (TWeakObjectPtr<UPrimitiveComponent>& Component : PrimitiveComponents)
	{
		if (m_RestoreOnEnd && ScratchPad)
		{
			ECollisionChannel CurrentChannel = Component->GetCollisionObjectType();
			ScratchPad->PrimitiveToChannelMap.Add(Component, CurrentChannel);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Object Type on Actor %s to %s."), *Component->GetOwner()->GetName(), *FAbleLogHelper::GetCollisionChannelEnumAsString(m_Channel.GetValue())));
		}
#endif

		Component->SetCollisionObjectType(m_Channel.GetValue());
	}


}

void UAblSetCollisionChannelTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (!Context.IsValid())
	{
		return;
	}

	if (UAblSetCollisionChannelTaskScratchPad* ScratchPad = Cast<UAblSetCollisionChannelTaskScratchPad>(Context->GetScratchPadForTask(this)))
	{
		for (TMap<TWeakObjectPtr<UPrimitiveComponent>, TEnumAsByte<ECollisionChannel>>::TConstIterator Iter = ScratchPad->PrimitiveToChannelMap.CreateConstIterator(); Iter; ++Iter)
		{
			if (UPrimitiveComponent* PrimitiveComponent = Iter->Key.Get())
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Object Type on Actor %s to %s."), *PrimitiveComponent->GetOwner()->GetName(), *FAbleLogHelper::GetCollisionChannelEnumAsString(Iter->Value)));
				}
#endif
				PrimitiveComponent->SetCollisionObjectType(Iter->Value);
			}
		}
	}
}

UAblAbilityTaskScratchPad* UAblSetCollisionChannelTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (m_RestoreOnEnd)
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblSetCollisionChannelTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblSetCollisionChannelTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblSetCollisionChannelTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblSetCollisionChannelTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblSetCollisionChannelTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblSetCollisionChannelTaskFormat", "{0}: {1}");
	FString CollisionChannelName = FAbleLogHelper::GetCollisionChannelEnumAsString(m_Channel);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(CollisionChannelName));
}

#endif

#undef LOCTEXT_NAMESPACE
