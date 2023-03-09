// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ablTargetingFilters.h"

#include "AbleCorePrivate.h"

#include "ablAbility.h"
#include "ablAbilityContext.h"
#include "ablAbilityDebug.h"
#include "ablAbilityTypes.h"
#include "ablSettings.h"

#include "Async/Future.h"
#include "Async/Async.h"

#include "DrawDebugHelpers.h"
#include "Logging/LogMacros.h"
#include "Targeting/ablTargetingBase.h"

UAblAbilityTargetingFilter::UAblAbilityTargetingFilter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblAbilityTargetingFilter::~UAblAbilityTargetingFilter()
{

}

void UAblAbilityTargetingFilter::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	checkNoEntry();
}

#if WITH_EDITOR

bool UAblAbilityTargetingFilter::FixUpObjectFlags()
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

// Sorter helpers
struct FAblAbilityActorLocationTargetSorter
{
	FVector SourceLocation;
	bool XYOnly;
	bool Ascending; // Ascending for Nearest to Farthest, descending for the opposite.
	FAblAbilityActorLocationTargetSorter(FVector& InSourceLocation, bool InXYOnly, bool InAscending)
		: SourceLocation(InSourceLocation), XYOnly(InXYOnly), Ascending(InAscending) {}

	FORCEINLINE bool operator()(const TWeakObjectPtr<AActor>& A, const TWeakObjectPtr<AActor>& B) const
	{
		if (XYOnly)
		{
			// I could probably ditch this if branch: (return DistanceCheck || !Ascending)
			// but I'd rather keep things explicit and branch prediction should take care of things anyway since its constant.
			if (Ascending) // We want nearest
			{
				return FVector::DistSquaredXY(A->GetActorLocation(), SourceLocation) < FVector::DistSquaredXY(B->GetActorLocation(), SourceLocation);
			}
			else
			{
				return FVector::DistSquaredXY(A->GetActorLocation(), SourceLocation) > FVector::DistSquaredXY(B->GetActorLocation(), SourceLocation);
			}
		}
		else
		{
			if (Ascending) // We want nearest
			{
				return FVector::DistSquared(A->GetActorLocation(), SourceLocation) < FVector::DistSquared(B->GetActorLocation(), SourceLocation);
			}
			else
			{
				return FVector::DistSquared(A->GetActorLocation(), SourceLocation) > FVector::DistSquared(B->GetActorLocation(), SourceLocation);
			}
		}
	}
};
/////

UAblAbilityTargetingFilterSortByDistance::UAblAbilityTargetingFilterSortByDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Use2DDistance(true),
	m_SortDirection(EAblTargetingFilterSort::AblTargetFilterSort_Ascending)
{

}

UAblAbilityTargetingFilterSortByDistance::~UAblAbilityTargetingFilterSortByDistance()
{

}

void UAblAbilityTargetingFilterSortByDistance::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	FVector SourceLocation = GetSourceLocation(Context, TargetBase.GetSource());
	Context.GetMutableTargetActors().Sort(FAblAbilityActorLocationTargetSorter(SourceLocation, m_Use2DDistance, m_SortDirection.GetValue() == EAblTargetingFilterSort::AblTargetFilterSort_Ascending));
}

FVector UAblAbilityTargetingFilterSortByDistance::GetSourceLocation(const UAblAbilityContext& Context, EAblAbilityTargetType SourceType) const
{
	FVector ReturnVal(0.0f, 0.0f, 0.0f);
	if (SourceType == EAblAbilityTargetType::ATT_Self)
	{
		if (AActor* Owner = Context.GetSelfActor())
		{
			ReturnVal = Owner->GetActorLocation();
		}
	}
	else if (SourceType == EAblAbilityTargetType::ATT_Instigator)
	{
		if (AActor* Instigator = Context.GetInstigator())
		{
			ReturnVal = Instigator->GetActorLocation();
		}
	}
	else if (SourceType == EAblAbilityTargetType::ATT_Owner)
	{
		if (AActor* Owner = Context.GetOwner())
		{
			ReturnVal = Owner->GetActorLocation();
		}
	}
	else if (SourceType == EAblAbilityTargetType::ATT_TargetActor)
	{
		UE_LOG(LogAble, Error, TEXT("Targeting Filter has source set as 'TargetActor' or 'TargetComponent'. This is invalid as the target has not been found yet."));
	}
	else
	{
		checkNoEntry();
	}

	return ReturnVal;
}

UAblAbilityTargetingFilterSelf::UAblAbilityTargetingFilterSelf(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblAbilityTargetingFilterSelf::~UAblAbilityTargetingFilterSelf()
{

}

void UAblAbilityTargetingFilterSelf::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll([&](const TWeakObjectPtr<AActor>& LHS) { return LHS == Context.GetSelfActor(); });
}


UAblAbilityTargetingFilterOwner::UAblAbilityTargetingFilterOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblAbilityTargetingFilterOwner::~UAblAbilityTargetingFilterOwner()
{

}

void UAblAbilityTargetingFilterOwner::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll([&](const TWeakObjectPtr<AActor>& LHS) { return LHS == Context.GetOwner(); });
}

UAblAbilityTargetingFilterInstigator::UAblAbilityTargetingFilterInstigator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblAbilityTargetingFilterInstigator::~UAblAbilityTargetingFilterInstigator()
{

}

void UAblAbilityTargetingFilterInstigator::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll([&](const TWeakObjectPtr<AActor>& LHS) { return LHS == Context.GetInstigator(); });
}

UAblAbilityTargetingFilterClass::UAblAbilityTargetingFilterClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Class(nullptr),
	m_Negate(false)
{

}

UAblAbilityTargetingFilterClass::~UAblAbilityTargetingFilterClass()
{

}

// Filter Predicate.
struct FAblAbilityTargetingClassFilterPredicate
{
	const UClass* ClassFilter;
	bool Negate;
	FAblAbilityTargetingClassFilterPredicate(const UClass& InClass, bool InNegate)
		: ClassFilter(&InClass), Negate(InNegate) {}

	FORCEINLINE bool operator()(const TWeakObjectPtr<AActor>& A) const
	{
		if (A.IsValid())
		{
			bool IsChild = A->GetClass()->IsChildOf(ClassFilter);
			return Negate ? !IsChild : IsChild;
		}
		
		return true; // Default to removal
	}
};

void UAblAbilityTargetingFilterClass::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll(FAblAbilityTargetingClassFilterPredicate(*m_Class, m_Negate));
}

UAblAbilityTargetingFilterMaxTargets::UAblAbilityTargetingFilterMaxTargets(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_MaxTargets(1)
{

}

UAblAbilityTargetingFilterMaxTargets::~UAblAbilityTargetingFilterMaxTargets()
{

}

void UAblAbilityTargetingFilterMaxTargets::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();

	if (TargetActors.Num() > m_MaxTargets)
	{
		int32 NumberToTrim = TargetActors.Num() - m_MaxTargets;
		if (NumberToTrim > 0 && NumberToTrim < TargetActors.Num())
		{
			TargetActors.RemoveAt(TargetActors.Num() - NumberToTrim, NumberToTrim);
		}
	}
}

UAblAbilityTargetingFilterCustom::UAblAbilityTargetingFilterCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_UseAsync(false)
{

}

UAblAbilityTargetingFilterCustom::~UAblAbilityTargetingFilterCustom()
{

}

void UAblAbilityTargetingFilterCustom::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();
	const UAblAbility* Ability = Context.GetAbility();
	check(Ability);

	if (m_UseAsync && UAbleSettings::IsAsyncEnabled())
	{
		TArray<TFuture<bool>> FutureResults;
		FutureResults.Reserve(TargetActors.Num());

		FName EventName = m_EventName;
		for (TWeakObjectPtr<AActor>& TargetActor : TargetActors)
		{
			FutureResults.Add(Async(EAsyncExecution::TaskGraph, [&Ability, &Context, EventName, &TargetActor]
			{
				return Ability->CustomFilterConditionBP(&Context, EventName, TargetActor.Get());
			}));
		}

		check(FutureResults.Num() == TargetActors.Num());

		int TargetIndex = 0;
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
				TargetActors.RemoveAt(TargetIndex, 1, false);
			}
			else
			{
				++TargetIndex;
			}

		}

		TargetActors.Shrink();
	}
	else
	{
		for (int i = 0; i < TargetActors.Num(); )
		{
			if (!Ability->CustomFilterConditionBP(&Context, m_EventName, TargetActors[i].Get()))
			{
				TargetActors.RemoveAt(i, 1, false);
			}
			else
			{
				++i;
			}
		}

		TargetActors.Shrink();
	}
}

UAblAbilityTargetingFilterLineOfSight::UAblAbilityTargetingFilterLineOfSight(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UAblAbilityTargetingFilterLineOfSight::~UAblAbilityTargetingFilterLineOfSight()
{

}

void UAblAbilityTargetingFilterLineOfSight::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	UWorld* CurrentWorld = Context.GetWorld();
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

	AActor* SelfActor = Context.GetSelfActor();
	FHitResult Hit;
	FTransform SourceTransform;
	m_SourceLocation.GetTransform(Context, SourceTransform);

	FVector RaySource = SourceTransform.GetTranslation();
	FVector RayEnd;

	TArray<TWeakObjectPtr<AActor>>& MutableTargets = Context.GetMutableTargetActors();
	for (int i = 0; i < MutableTargets.Num();)
	{
		Hit.Reset(1.0f, false);

		CollisionParams.ClearIgnoredActors();
		CollisionParams.AddIgnoredActor(SelfActor);
		CollisionParams.AddIgnoredActor(MutableTargets[i].Get());

		RayEnd = MutableTargets[i]->GetActorLocation();

		if (CurrentWorld->LineTraceSingleByObjectType(Hit, RaySource, RayEnd, ObjectQuery, CollisionParams))
		{
			MutableTargets.RemoveAt(i, 1, false);
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

	MutableTargets.Shrink();
}

UAblAbilityTargetingFilterUniqueActors::UAblAbilityTargetingFilterUniqueActors(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblAbilityTargetingFilterUniqueActors::~UAblAbilityTargetingFilterUniqueActors()
{

}

void UAblAbilityTargetingFilterUniqueActors::Filter(UAblAbilityContext& Context, const UAblTargetingBase& TargetBase) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();
	for (int32 i = 0; i < TargetActors.Num(); ++i)
	{
		const TWeakObjectPtr<AActor>& currentEntry = TargetActors[i];
		for (int32 j = i + 1; j < TargetActors.Num();)
		{
			if (currentEntry == TargetActors[j])
			{
				// Swap this entry with the last entry, decrement the count - saves shifting the array around.
				TargetActors[j] = TargetActors.Last();
				TargetActors.RemoveAt(TargetActors.Num() - 1, 1, false);

				// Redo this iteration since we could have just swapped in another dupe.
			}
			else
			{
				++j;
			}
		}
	}
}
