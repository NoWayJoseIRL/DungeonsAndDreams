// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablModifyContextTask.h"

#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "AbleCorePrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblModifyContextTask::UAblModifyContextTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TargetLocation(ForceInitToZero),
	m_AdditionalTargets(true),
	m_ClearCurrentTargets(false)
{

}

UAblModifyContextTask::~UAblModifyContextTask()
{

}


void UAblModifyContextTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	FVector WantsTargetLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TargetLocation);

	bool WantsAdditionalTargets = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AdditionalTargets);
	bool WantsClearTargets = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_ClearCurrentTargets);

	TArray<AActor*> AdditionalTargets;

	if (WantsAdditionalTargets)
	{
		Context->GetAbility()->CustomTargetingFindTargets(Context.Get(), AdditionalTargets);
	}

	TArray<TWeakObjectPtr<AActor>> AdditionalWeakTargets(AdditionalTargets);

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Modifying Context with Location %s, and %d New Targets. Clear Targets = %s"), 
		WantsTargetLocation.SizeSquared() > 0.0f ? *WantsTargetLocation.ToString() : TEXT("<not changed>"),
		AdditionalWeakTargets.Num(),
		WantsClearTargets ? TEXT("True") : TEXT("False")));
	}
#endif

	Context->GetSelfAbilityComponent()->ModifyContext(Context, nullptr, nullptr, WantsTargetLocation, AdditionalWeakTargets, WantsClearTargets);
}

TStatId UAblModifyContextTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblModifyContextTask, STATGROUP_Able);
}

void UAblModifyContextTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TargetLocation, TEXT("Target Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AdditionalTargets, TEXT("Additional Targets"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ClearCurrentTargets, TEXT("Clear Targets"));
}

#undef LOCTEXT_NAMESPACE