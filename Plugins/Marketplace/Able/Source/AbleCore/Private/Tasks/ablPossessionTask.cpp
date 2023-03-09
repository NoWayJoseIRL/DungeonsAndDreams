// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPossessionTask.h"

#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#if (!UE_BUILD_SHIPPING)
#include "ablAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblPossessionTaskScratchPad::UAblPossessionTaskScratchPad()
{

}

UAblPossessionTaskScratchPad::~UAblPossessionTaskScratchPad()
{

}

UAblPossessionTask::UAblPossessionTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Possessor(EAblAbilityTargetType::ATT_Self),
	m_PossessionTarget(EAblAbilityTargetType::ATT_TargetActor)
{

}

UAblPossessionTask::~UAblPossessionTask()
{

}

void UAblPossessionTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	AActor* SelfActor = GetSingleActorFromTargetType(Context, m_Possessor.GetValue());
	AActor* PossessionTarget = GetSingleActorFromTargetType(Context, m_PossessionTarget.GetValue());

	APawn* SelfPawn = Cast<APawn>(SelfActor);
	APawn* TargetPawn = Cast<APawn>(PossessionTarget);

	if (SelfPawn && TargetPawn)
	{
		if (APlayerController* PC = Cast<APlayerController>(SelfPawn->Controller))
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Actor %s is now possessing %s"), *SelfPawn->GetName(), *TargetPawn->GetName()));
			}
#endif
			PC->Possess(TargetPawn);

			if (m_UnPossessOnEnd)
			{
				UAblPossessionTaskScratchPad* ScratchPad = Cast<UAblPossessionTaskScratchPad>(Context->GetScratchPadForTask(this));
				check(ScratchPad);
				ScratchPad->PossessorController = PC;
			}
		}
	}

}

void UAblPossessionTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);

	if (m_UnPossessOnEnd && Context.IsValid())
	{
		UAblPossessionTaskScratchPad* ScratchPad = Cast<UAblPossessionTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);

		if (ScratchPad->PossessorController.IsValid())
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose() && ScratchPad->PossessorController->GetOwner())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Calling Unpossess on %s"), *ScratchPad->PossessorController->GetOwner()->GetName()));
			}
#endif
			ScratchPad->PossessorController->UnPossess();
		}
	}
}

UAblAbilityTaskScratchPad* UAblPossessionTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (m_UnPossessOnEnd)
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblPossessionTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblPossessionTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblPossessionTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblPossessionTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblPossessionTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPossessionTaskFormat", "{0}: {1}");
	FString TargetName = FAbleLogHelper::GetTargetTargetEnumAsString(m_Possessor);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

#endif

#undef LOCTEXT_NAMESPACE

