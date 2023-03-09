// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablCustomTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblCustomTaskScratchPad::UAblCustomTaskScratchPad()
{

}

UAblCustomTaskScratchPad::~UAblCustomTaskScratchPad()
{

}

UAblCustomTask::UAblCustomTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAblCustomTask::~UAblCustomTask()
{

}

void UAblCustomTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	OnTaskStartBP(Context.Get());
}

void UAblCustomTask::OnTaskStartBP_Implementation(const UAblAbilityContext* Context) const
{

}

void UAblCustomTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAblCustomTask::OnTaskTickBP_Implementation(const UAblAbilityContext* Context, float DeltaTime) const
{

}

void UAblCustomTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
	OnTaskEndBP(Context.Get(), result);
}

void UAblCustomTask::OnTaskEndBP_Implementation(const UAblAbilityContext* Context, const EAblAbilityTaskResult result) const
{

}

bool UAblCustomTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAblCustomTask::IsDoneBP_Implementation(const UAblAbilityContext* Context) const
{
	return UAblAbilityTask::IsDone(TWeakObjectPtr<const UAblAbilityContext>(Context));
}

TSubclassOf<UAblAbilityTaskScratchPad> UAblCustomTask::GetTaskScratchPadClass(const UAblAbilityContext* Context) const
{
	return GetTaskScratchPadClassBP(Context);
}

TSubclassOf<UAblAbilityTaskScratchPad> UAblCustomTask::GetTaskScratchPadClassBP_Implementation(const UAblAbilityContext* Context) const
{
	return nullptr;
}

UAblAbilityTaskScratchPad* UAblCustomTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = GetTaskScratchPadClass(Context.Get());
	if (UClass* ScratchPadCL = ScratchPadClass.Get())
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			UAblAbilityTaskScratchPad* ScratchPad = Cast<UAblCustomTaskScratchPad>(Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass));
			ResetScratchPad(ScratchPad);
			return ScratchPad;
		}

		// No Subsystem available?
		return Cast<UAblAbilityTaskScratchPad>(NewObject<UAblCustomTaskScratchPad>(Context.Get(), ScratchPadCL));
	}

	UE_LOG(LogAble, Warning, TEXT("Custom Task [%s] is using the old Scratchpad code that will be removed. Please override \"GetTaskScratchPadClass\" in your blueprint to avoid future breakage."), *GetTaskNameBP().ToString())
	if (UAblAbilityTaskScratchPad* ScratchPad = Cast<UAblAbilityTaskScratchPad>(CreateScratchPadBP(Context.Get())))
	{
		ResetScratchPad(ScratchPad);
		return ScratchPad;
	}

	UE_LOG(LogAble, Warning, TEXT("CreateScratchPadBP returned an null scratch pad. Please check your Create Scratch Pad method in your Blueprint.\n"));

	return nullptr;
}

UAblCustomTaskScratchPad* UAblCustomTask::GetScratchPad(UAblAbilityContext* Context) const
{
	return Cast<UAblCustomTaskScratchPad>(Context->GetScratchPadForTask(this));
}

UAblCustomTaskScratchPad* UAblCustomTask::CreateScratchPadBP_Implementation(UAblAbilityContext* Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblCustomTaskScratchPad::StaticClass();
		return Cast<UAblCustomTaskScratchPad>(Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass));
	}

	return NewObject<UAblCustomTaskScratchPad>(Context);
}

void UAblCustomTask::ResetScratchPad(UAblAbilityTaskScratchPad* ScratchPad) const
{
	ResetScratchPadBP(ScratchPad);
}

void UAblCustomTask::ResetScratchPadBP_Implementation(UAblAbilityTaskScratchPad* ScratchPad) const
{
	// Do nothing since we have no idea about the contents of the scratchpad.
}

bool UAblCustomTask::IsSingleFrame() const
{
	return IsSingleFrameBP();
}

bool UAblCustomTask::IsSingleFrameBP_Implementation() const
{
	return true;
}

EAblAbilityTaskRealm UAblCustomTask::GetTaskRealm() const
{
	return GetTaskRealmBP();
}

EAblAbilityTaskRealm UAblCustomTask::GetTaskRealmBP_Implementation() const
{
	return EAblAbilityTaskRealm::ATR_Client;
}

void UAblCustomTask::GetActorsForTaskBP(const UAblAbilityContext* Context, TArray<AActor*>& OutActorArray) const
{
	TArray<TWeakObjectPtr<AActor>> TempArray;
	GetActorsForTask(TWeakObjectPtr<const UAblAbilityContext>(Context), TempArray);

	// BP's don't support Containers of WeakObjectPtrs so we have to do a fun copy here... :/
	OutActorArray.Empty();
	for (TWeakObjectPtr<AActor>& FoundActor : TempArray)
	{
		if (FoundActor.IsValid())
		{
			OutActorArray.Add(FoundActor.Get());
		}
	}
}

TStatId UAblCustomTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblCustomTask, STATGROUP_Able);
}

FText UAblCustomTask::GetTaskCategoryBP_Implementation() const
{
	return LOCTEXT("AblCustomTaskCategory", "Blueprint|Custom");
}

FText UAblCustomTask::GetTaskNameBP_Implementation() const
{
#if WITH_EDITOR 
	return GetClass()->GetDisplayNameText();
#else
	return FText::FromString(GetClass()->GetName());
#endif
}

FText UAblCustomTask::GetDescriptiveTaskNameBP_Implementation() const
{
	return LOCTEXT("AblCustomTaskDisplayName", "Custom");
}

FText UAblCustomTask::GetTaskDescriptionBP_Implementation() const
{
#if WITH_EDITOR 
	return GetClass()->GetDisplayNameText();
#else
	return FText::FromString(GetClass()->GetName());
#endif
}

FLinearColor UAblCustomTask::GetTaskColorBP_Implementation() const
{
	return FLinearColor::White;
}
#if WITH_EDITOR

FText UAblCustomTask::GetTaskCategory() const
{
	return GetTaskCategoryBP();
}

FText UAblCustomTask::GetTaskName() const
{
	return GetTaskNameBP();
}

FText UAblCustomTask::GetDescriptiveTaskName() const
{
	return GetDescriptiveTaskNameBP();
}

FText UAblCustomTask::GetTaskDescription() const
{
	return GetTaskDescriptionBP();
}

FLinearColor UAblCustomTask::GetTaskColor() const
{
	return GetTaskColorBP();
}

EDataValidationResult UAblCustomTask::IsDataValid(TArray<FText>& ValidationErrors)
{
    const FText AssetName = GetTaskName();
    return IsTaskDataValid(nullptr, AssetName, ValidationErrors);
}

#endif

#undef LOCTEXT_NAMESPACE