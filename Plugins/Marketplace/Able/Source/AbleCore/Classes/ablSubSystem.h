// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Tasks/IAblAbilityTask.h"

#include "ablSubSystem.generated.h"

USTRUCT()
struct ABLECORE_API FAblTaskScratchPadBucket
{
	GENERATED_BODY()
public:
	FAblTaskScratchPadBucket() : ScratchPadClass(nullptr), Instances() {};

	UPROPERTY()
	TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass;

	UPROPERTY()
	TArray<UAblAbilityTaskScratchPad*> Instances;
};

USTRUCT()
struct ABLECORE_API FAblAbilityScratchPadBucket
{
	GENERATED_BODY()
public:
	FAblAbilityScratchPadBucket() : ScratchPadClass(nullptr), Instances() {};

	UPROPERTY()
	TSubclassOf<UAblAbilityScratchPad> ScratchPadClass;

	UPROPERTY()
	TArray<UAblAbilityScratchPad*> Instances;
};

UCLASS()
class ABLECORE_API UAblAbilityUtilitySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UAblAbilityUtilitySubsystem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual ~UAblAbilityUtilitySubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Able")
	UAblAbilityContext* FindOrConstructContext();

	UFUNCTION(BlueprintCallable, Category = "Able")
	UAblAbilityTaskScratchPad* FindOrConstructTaskScratchPad(TSubclassOf<UAblAbilityTaskScratchPad>& Class);

	UFUNCTION(BlueprintCallable, Category = "Able")
	UAblAbilityScratchPad* FindOrConstructAbilityScratchPad(TSubclassOf<UAblAbilityScratchPad>& Class);

	// Return methods
	void ReturnContext(UAblAbilityContext* Context);
	void ReturnTaskScratchPad(UAblAbilityTaskScratchPad* Scratchpad);
	void ReturnAbilityScratchPad(UAblAbilityScratchPad* Scratchpad);

private:
	// Helper methods
	FAblTaskScratchPadBucket* GetTaskBucketByClass(TSubclassOf<UAblAbilityTaskScratchPad>& Class);
	FAblAbilityScratchPadBucket* GetAbilityBucketByClass(TSubclassOf<UAblAbilityScratchPad>& Class);
	uint32 GetTotalScratchPads() const;

	UPROPERTY(Transient)
	TArray<UAblAbilityContext*> m_AllocatedContexts;

	UPROPERTY(Transient)
	TArray<UAblAbilityContext*> m_AvailableContexts;

	UPROPERTY(Transient)
	TArray<FAblTaskScratchPadBucket> m_TaskBuckets;

	UPROPERTY(Transient)
	TArray<FAblAbilityScratchPadBucket> m_AbilityBuckets;

	UPROPERTY(Transient)
	const UAbleSettings* m_Settings;
};