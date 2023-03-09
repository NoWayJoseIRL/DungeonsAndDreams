// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablSetCollisionChannelResponseTask.h"

#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Misc/EnumRange.h"
#include "Stats/Stats2.h"

#if !(UE_BUILD_SHIPPING)
#include "ablAbilityUtilities.h"
#endif

ENUM_RANGE_BY_FIRST_AND_LAST(ECollisionChannel, ECC_WorldStatic, ECC_MAX);

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblSetCollisionChannelResponseTaskScratchPad::UAblSetCollisionChannelResponseTaskScratchPad()
{

}

UAblSetCollisionChannelResponseTaskScratchPad::~UAblSetCollisionChannelResponseTaskScratchPad()
{

}

UAblSetCollisionChannelResponseTask::UAblSetCollisionChannelResponseTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Channel(ECollisionChannel::ECC_WorldDynamic),
	m_Response(ECollisionResponse::ECR_Ignore),
	m_SetAllChannelsToResponse(false),
	m_RestoreOnEnd(true)
{

}

UAblSetCollisionChannelResponseTask::~UAblSetCollisionChannelResponseTask()
{

}

void UAblSetCollisionChannelResponseTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	UAblSetCollisionChannelResponseTaskScratchPad* ScratchPad = nullptr;
	
	if (m_RestoreOnEnd)
	{
		ScratchPad = Cast<UAblSetCollisionChannelResponseTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);
		ScratchPad->PreviousCollisionValues.Empty();
	}

	// We need to convert our Actors to primitive components.
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	TArray<TWeakObjectPtr<UPrimitiveComponent>> PrimitiveComponents;

	TArray<FCollisionChannelResponsePair> AllCRPairs;
	// Append our deprecated value.
	AllCRPairs.Add(FCollisionChannelResponsePair(m_Channel.GetValue(), m_Response.GetValue()));
	AllCRPairs.Append(m_Channels);

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
		{
			PrimitiveComponents.AddUnique(PrimitiveComponent);
		}
	}

	for (TWeakObjectPtr<UPrimitiveComponent>& Component : PrimitiveComponents)
	{
		if (Component.IsValid())
		{
			if (m_RestoreOnEnd)
			{
				if (m_SetAllChannelsToResponse)
				{
					const FCollisionResponseContainer& Container = Component->GetCollisionResponseToChannels();
					for (ECollisionChannel Channel : TEnumRange<ECollisionChannel>())
					{
						ScratchPad->PreviousCollisionValues.Add(FCollisionLayerResponseEntry(Component.Get(), Channel, Container.GetResponse(Channel)));
					}
				}
				else
				{
					for (const FCollisionChannelResponsePair& Pair : AllCRPairs)
					{
						ScratchPad->PreviousCollisionValues.Add(FCollisionLayerResponseEntry(Component.Get(), Pair.CollisionChannel.GetValue(), Component->GetCollisionResponseToChannel(Pair.CollisionChannel.GetValue())));
					}
				}
			}

			if (m_SetAllChannelsToResponse)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Setting All Collision Responses on Actor %s to %s."), *Component->GetOwner()->GetName(), *FAbleLogHelper::GetCollisionResponseEnumAsString(m_Response.GetValue())));
				}
#endif
				Component->SetCollisionResponseToAllChannels(m_Response.GetValue());
			}
			else
			{
				for (const FCollisionChannelResponsePair& Pair : AllCRPairs)
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Channel %s Response on Actor %s to %s."), *FAbleLogHelper::GetCollisionChannelEnumAsString(Pair.CollisionChannel.GetValue()), *Component->GetOwner()->GetName(), *FAbleLogHelper::GetCollisionResponseEnumAsString(Pair.CollisionResponse.GetValue())));
					}
#endif
					Component->SetCollisionResponseToChannel(Pair.CollisionChannel.GetValue(), Pair.CollisionResponse.GetValue());
				}
			}
		}
	}
}

void UAblSetCollisionChannelResponseTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (m_RestoreOnEnd && Context.IsValid())
	{
		UAblSetCollisionChannelResponseTaskScratchPad* ScratchPad = Cast<UAblSetCollisionChannelResponseTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);

		for (const FCollisionLayerResponseEntry& Entry : ScratchPad->PreviousCollisionValues)
		{
			if (Entry.Primitive.IsValid())
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Channel %s Response on Actor %s to %s."), *FAbleLogHelper::GetCollisionChannelEnumAsString(Entry.Channel), *Entry.Primitive->GetOwner()->GetName(), *FAbleLogHelper::GetCollisionResponseEnumAsString(Entry.Response)));
				}
#endif
				Entry.Primitive->SetCollisionResponseToChannel(Entry.Channel, Entry.Response);
			}
		}
	}
}

UAblAbilityTaskScratchPad* UAblSetCollisionChannelResponseTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (m_RestoreOnEnd)
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblSetCollisionChannelResponseTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblSetCollisionChannelResponseTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblSetCollisionChannelResponseTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblSetCollisionChannelResponseTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblSetCollisionChannelResponseTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblSetCollisionChannelResponseTaskFormat", "{0}: {1}->{2}");

	FString CollisionChannelName = m_SetAllChannelsToResponse ? TEXT("All") : FAbleLogHelper::GetCollisionChannelEnumAsString(m_Channel);
	FString ResponseName = FAbleLogHelper::GetCollisionResponseEnumAsString(m_Response);

	if (m_Channels.Num() != 0)
	{
		if (m_Channels.Num() == 1)
		{
			CollisionChannelName = FAbleLogHelper::GetCollisionChannelEnumAsString(m_Channels[0].CollisionChannel);
			ResponseName = FAbleLogHelper::GetCollisionResponseEnumAsString(m_Channels[0].CollisionResponse);
		}
		else
		{
			CollisionChannelName = TEXT("(multiple)");
			ResponseName = TEXT("(multiple)");
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(CollisionChannelName), FText::FromString(ResponseName));
}

#endif

#undef LOCTEXT_NAMESPACE