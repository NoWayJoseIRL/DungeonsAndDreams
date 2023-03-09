// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCollisionFilters.h"

#include "AbleCorePrivate.h"
#include "ablAbilityContext.h"
#include "ablAbilityDebug.h"
#include "ablAbilityTypes.h"
#include "ablAbilityUtilities.h"
#include "ablSettings.h"

#include "Async/Future.h"
#include "Async/Async.h"

#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCollisionFilter::UAblCollisionFilter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblCollisionFilter::~UAblCollisionFilter()
{

}

void UAblCollisionFilter::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	verifyf(false, TEXT("This method should never be called. Did you forget to override it in your child class?"));
}

FName UAblCollisionFilter::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_CollisionFilter_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

UAblCollisionFilterSelf::UAblCollisionFilterSelf(const FObjectInitializer& ObjectInitializer)
	: Super (ObjectInitializer)
{

}

UAblCollisionFilterSelf::~UAblCollisionFilterSelf()
{

}

void UAblCollisionFilterSelf::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	InOutArray.RemoveAll([&](const FAblQueryResult& LHS) { return LHS.Actor == Context->GetSelfActor(); });
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterSelf::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}

bool UAblCollisionFilter::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));
	
	if (oldFlags != GetFlags())
	{
		Modify();
		return true;
	}

	return false;
}
#endif

UAblCollisionFilterOwner::UAblCollisionFilterOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblCollisionFilterOwner::~UAblCollisionFilterOwner()
{

}

void UAblCollisionFilterOwner::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	InOutArray.RemoveAll([&](const FAblQueryResult& LHS) { return LHS.Actor == Context->GetOwner(); });
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterOwner::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAblCollisionFilterInstigator::UAblCollisionFilterInstigator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblCollisionFilterInstigator::~UAblCollisionFilterInstigator()
{

}

void UAblCollisionFilterInstigator::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	InOutArray.RemoveAll([&](const FAblQueryResult& LHS) { return LHS.Actor == Context->GetInstigator(); });
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterInstigator::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAblCollisionFilterByClass::UAblCollisionFilterByClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblCollisionFilterByClass::~UAblCollisionFilterByClass()
{

}

void UAblCollisionFilterByClass::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	bool Remove = false;
	for (int i = 0; i < InOutArray.Num(); )
	{
		if (AActor* Actor = InOutArray[i].Actor.Get())
		{
			Remove = Actor->GetClass()->IsChildOf(m_Class);
			Remove = m_Negate ? !Remove : Remove;
		}
		else
		{
			Remove = true;
		}

		if (Remove)
		{
			InOutArray.RemoveAt(i, 1, false);
		}
		else
		{
			++i;
		}
	}

	InOutArray.Shrink();
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterByClass::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_Class == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoClassDefined", "No Class Defined for Filtering: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAblCollisionFilterSortByDistance::UAblCollisionFilterSortByDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblCollisionFilterSortByDistance::~UAblCollisionFilterSortByDistance()
{

}

void UAblCollisionFilterSortByDistance::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	FTransform SourceTransform;
	FAblAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);
	Location.GetTransform(*Context.Get(), SourceTransform);
	InOutArray.Sort(FAblAbilityResultSortByDistance(SourceTransform.GetLocation(), m_Use2DDistance, m_SortDirection == EAblCollisionFilterSort::AblFitlerSort_Ascending));
}

void UAblCollisionFilterSortByDistance::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Source Location"));
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterSortByDistance::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAblCollisionFilterMaxResults::UAblCollisionFilterMaxResults(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblCollisionFilterMaxResults::~UAblCollisionFilterMaxResults()
{

}

void UAblCollisionFilterMaxResults::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	if (InOutArray.Num() > m_MaxEntities)
	{
		int32 NumberToTrim = InOutArray.Num() - m_MaxEntities;
		if (NumberToTrim > 0 && NumberToTrim < InOutArray.Num())
		{
			InOutArray.RemoveAt(InOutArray.Num() - NumberToTrim, NumberToTrim);
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterMaxResults::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_MaxEntities == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("InvalidFilterCount", "Filter is 0, will exclude all entities: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAblCollisionFilterCustom::UAblCollisionFilterCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_EventName(NAME_None),
	m_UseAsync(false)
{

}

UAblCollisionFilterCustom::~UAblCollisionFilterCustom()
{

}

void UAblCollisionFilterCustom::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	check(Context.IsValid());
	const UAblAbility* Ability = Context->GetAbility();
	check(Ability);

	if (m_UseAsync && UAbleSettings::IsAsyncEnabled())
	{
		TArray<TFuture<bool>> FutureResults;
		FutureResults.Reserve(InOutArray.Num());

		FName EventName = m_EventName;
		for (FAblQueryResult& Result : InOutArray)
		{
			FutureResults.Add(Async(EAsyncExecution::TaskGraph, [&Ability, &Context, EventName, &Result]
			{
				return Ability->CustomFilterConditionBP(Context.Get(), EventName, Result.Actor.Get());
			}));
		}

		check(FutureResults.Num() == InOutArray.Num());
		int32 InOutIndex = 0;
		for (TFuture<bool>& Future : FutureResults)
		{
			if (!Future.IsReady())
			{
				static const FTimespan OneMillisecond = FTimespan::FromMilliseconds(1.0);
				Future.WaitFor(OneMillisecond);
			}

			if (!Future.Get())
			{
				// Target passed filtering, moving on.
				InOutArray.RemoveAt(InOutIndex, 1, false);
			}
			else
			{
				++InOutIndex;
			}
		}

		InOutArray.Shrink();
	}
	else
	{
		for (int i = 0; i < InOutArray.Num(); )
		{
			if (!Ability->CustomFilterConditionBP(Context.Get(), m_EventName, InOutArray[i].Actor.Get()))
			{
				InOutArray.RemoveAt(i, 1, false);
			}
			else
			{
				++i;
			}
		}

		InOutArray.Shrink();
	}
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterCustom::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CustomFilterConditionBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CustomFilterConditionBP_NotFound", "Function 'CustomFilterConditionBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}
#endif

UAblCollisionFilterTeamAttitude::UAblCollisionFilterTeamAttitude(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, m_IgnoreAttitude(0)
{
}

UAblCollisionFilterTeamAttitude::~UAblCollisionFilterTeamAttitude()
{
}

void UAblCollisionFilterTeamAttitude::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	AActor* SelfActor = Context->GetSelfActor();

	InOutArray.RemoveAll([&](const FAblQueryResult& LHS)
	{
		ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(SelfActor, LHS.Actor.Get());

		if (((1 << (int32)Attitude) & m_IgnoreAttitude) != 0)
		{
			return true;
		}
		return false;
	});
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterTeamAttitude::IsAbilityDataValid(const UAblAbility* AbilityContext, TArray<FText>& ValidationErrors)
{
	EDataValidationResult result = EDataValidationResult::Valid;

	if (m_IgnoreAttitude == 0)
	{
		ValidationErrors.Add(FText::Format(LOCTEXT("FilterAttitudeInvalid", "Not filtering any attitudes: {0}"), FText::FromString(AbilityContext->GetDisplayName())));
		result = EDataValidationResult::Invalid;
	}

	return result;
}
#endif

UAblCollisionFilterLineOfSight::UAblCollisionFilterLineOfSight(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UAblCollisionFilterLineOfSight::~UAblCollisionFilterLineOfSight()
{

}

void UAblCollisionFilterLineOfSight::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	UWorld* CurrentWorld = Context->GetWorld();
	if (!CurrentWorld)
	{
		UE_LOG(LogAble, Warning, TEXT("Invalid World Object. Cannot run Line of Sight filter."));
		return;
	}

	FCollisionObjectQueryParams ObjectQuery;
	for (TEnumAsByte<ECollisionChannel> Channel : m_CollisionChannels)
	{
		ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionQueryParams CollisionParams;

	AActor* SelfActor = Context->GetSelfActor();
	FHitResult Hit;
	FTransform SourceTransform;
	
	FAblAbilityTargetTypeLocation SourceLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);
	
	SourceLocation.GetTransform(*Context.Get(), SourceTransform);

	FVector RaySource = SourceTransform.GetTranslation();
	FVector RayEnd;

	for (int i = 0; i < InOutArray.Num();)
	{
		Hit.Reset(1.0f, false);

		CollisionParams.ClearIgnoredActors();
		CollisionParams.AddIgnoredActor(SelfActor);
		CollisionParams.AddIgnoredActor(InOutArray[i].Actor.Get());

		RayEnd = InOutArray[i].Actor->GetActorLocation();

		if (CurrentWorld->LineTraceSingleByObjectType(Hit, RaySource, RayEnd, ObjectQuery, CollisionParams))
		{
			InOutArray.RemoveAt(i, 1, false);
		}
		else
		{
			++i;
		}

#if !UE_BUILD_SHIPPING
		if (FAblAbilityDebug::ShouldDrawQueries())
		{
			DrawDebugLine(CurrentWorld, RaySource, RayEnd, Hit.bBlockingHit ? FColor::Red : FColor::Green, FAblAbilityDebug::ShouldDrawInEditor(), FAblAbilityDebug::GetDebugQueryLifetime());
		}
#endif
	}

	InOutArray.Shrink();
}

void UAblCollisionFilterLineOfSight::BindDynamicDelegates(class UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Source Location"));
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterLineOfSight::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
	EDataValidationResult result = EDataValidationResult::Valid;

	if (m_CollisionChannels.Num() == 0)
	{
		ValidationErrors.Add(FText::Format(LOCTEXT("CollisionFilterLOSNoChannels", "No Collision Channels set for Line of Sight filter. {0}"), AssetName));
		result = EDataValidationResult::Invalid;
	}

	return result;
}
#endif

UAblCollisionFilterUniqueActors::UAblCollisionFilterUniqueActors(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UAblCollisionFilterUniqueActors::~UAblCollisionFilterUniqueActors()
{

}

void UAblCollisionFilterUniqueActors::Filter(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<FAblQueryResult>& InOutArray) const
{
	for (int32 i = 0; i < InOutArray.Num(); ++i)
	{
		const FAblQueryResult& currentEntry = InOutArray[i];
		for (int32 j = i + 1; j < InOutArray.Num();)
		{
			if (currentEntry.Actor != nullptr && currentEntry.Actor == InOutArray[j].Actor)
			{
				// Swap this entry with the last entry, decrement the count - saves shifting the array around.
				InOutArray[j] = InOutArray.Last();
				InOutArray.RemoveAt(InOutArray.Num() - 1, 1, false);

				// Redo this iteration since we could have just swapped in another dupe.
			}
			else
			{
				++j;
			}
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UAblCollisionFilterUniqueActors::IsAbilityDataValid(const UAblAbility* AbilityContext, TArray<FText>& ValidationErrors)
{
	return EDataValidationResult::Valid;
}
#endif