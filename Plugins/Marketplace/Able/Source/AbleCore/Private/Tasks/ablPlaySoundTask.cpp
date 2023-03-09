// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPlaySoundTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblPlaySoundTaskScratchPad::UAblPlaySoundTaskScratchPad()
{

}

UAblPlaySoundTaskScratchPad::~UAblPlaySoundTaskScratchPad()
{

}

UAblPlaySoundTask::UAblPlaySoundTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Sound(nullptr),
	m_2DSound(false),
	m_SoundStartTime(0.0f),
	m_VolumeModifier(1.0f),
	m_PitchModifier(1.0f),
	m_Attenuation(nullptr),
	m_Concurrency(nullptr),
	m_AttachToSocket(false),
	m_DestroyOnEnd(false),
	m_DestroyFadeOutDuration(0.25f)
{

}

UAblPlaySoundTask::~UAblPlaySoundTask()
{

}

void UAblPlaySoundTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	if (!m_Sound)
	{
		UE_LOG(LogAble, Warning, TEXT("No Sound set for PlaySoundTask in Ability [%s]"), *Context->GetAbility()->GetDisplayName());
		return;
	}


	FTransform SpawnTransform;

	USoundBase* Sound = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Sound);
	float startTime = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SoundStartTime);
	float VolumeMod = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_VolumeModifier);
	float PitchMod = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_PitchModifier);
	USoundAttenuation* SoundAtten = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Attenuation);
	USoundConcurrency* SoundConCur = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Concurrency);
	FAblAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);

	if (m_2DSound)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Spawning 2D Sound [%s], Volume Modifier [%1.3f], Pitch Modifier [%1.3f], Sound Start Time [%4.2f], Attenuation [%s], Concurrency [%s]"),
				*Sound->GetName(),
				VolumeMod,
				PitchMod,
				startTime,
				SoundAtten ? *SoundAtten->GetName() : TEXT("NULL"),
				SoundConCur ? *SoundConCur->GetName() : TEXT("NULL")));
		}
#endif

		UGameplayStatics::PlaySound2D(GetWorld(), Sound, m_VolumeModifier, m_PitchModifier, startTime, SoundConCur);
		return; // We can early out, no reason to play a 2D sound multiple times.
	}

	TWeakObjectPtr<UAudioComponent> AttachedSound = nullptr;
	
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);
	
	UAblPlaySoundTaskScratchPad* ScratchPad = nullptr;
	if (m_DestroyOnEnd)
	{
		ScratchPad = Cast<UAblPlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this));
		ScratchPad->AttachedSounds.Empty();
	}

	for (int i = 0; i < TargetArray.Num(); ++i)
	{
		TWeakObjectPtr<AActor> Target = TargetArray[i];
		SpawnTransform.SetIdentity();

		if (Location.GetSourceTargetType() == EAblAbilityTargetType::ATT_TargetActor)
		{
			Location.GetTargetTransform(*Context, i, SpawnTransform);
		}
		else
		{
			Location.GetTransform(*Context, SpawnTransform);
		}

		if (m_AttachToSocket)
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Spawning Attached Sound [%s], Transform [%s], DestroyOnActorDestroy [%s], Volume Modifier [%1.3f], Pitch Modifier [%1.3f], Sound Start Time [%4.2f], Attenuation [%s], Concurrency [%s], Auto Destroy [%s]"),
															*Sound->GetName(),
															*SpawnTransform.ToString(),
															 m_DestroyOnActorDestroy ? TEXT("TRUE") : TEXT("FALSE"),
															 VolumeMod,
															 PitchMod,
															 startTime,
															 SoundAtten ? *SoundAtten->GetName() : TEXT("NULL"),
															 SoundConCur ? *SoundConCur->GetName() : TEXT("NULL"),
															 m_AllowAutoDestroy ? TEXT("TRUE") : TEXT("FALSE")));
			}
#endif

			AttachedSound = UGameplayStatics::SpawnSoundAttached(Sound, 
																Target->FindComponentByClass<USceneComponent>(), 
																Location.GetSocketName(), 
																Location.GetOffset(), 
																EAttachLocation::KeepRelativeOffset,
																m_DestroyOnActorDestroy,
																VolumeMod,
																PitchMod,
																startTime,
																SoundAtten,
																SoundConCur, 
																m_AllowAutoDestroy);
		}
		else
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Spawning Sound [%s], Transform [%s], Volume Modifier [%1.3f], Pitch Modifier [%1.3f], Sound Start Time [%4.2f], Attenuation [%s], Concurrency [%s], Auto Destroy [%s]"),
					*Sound->GetName(),
					*SpawnTransform.ToString(),
					VolumeMod,
					PitchMod,
					startTime,
					SoundAtten ? *SoundAtten->GetName() : TEXT("NULL"),
					SoundConCur ? *SoundConCur->GetName() : TEXT("NULL"),
					m_AllowAutoDestroy ? TEXT("TRUE") : TEXT("FALSE")));
			}
#endif
			AttachedSound = UGameplayStatics::SpawnSoundAtLocation(Target->GetWorld(),
																   Sound,
																   SpawnTransform.GetTranslation(),
																   SpawnTransform.GetRotation().Rotator(),
																   VolumeMod,
																   PitchMod,
																   startTime,
																   SoundAtten,
																   SoundConCur, 
																   m_AllowAutoDestroy);
		}

		if (AttachedSound.IsValid() && ScratchPad)
		{
			ScratchPad->AttachedSounds.Add(AttachedSound);
		}

		AttachedSound = nullptr;
	}
}

void UAblPlaySoundTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const
{
	Super::OnTaskEnd(Context, Result);

	if (m_DestroyOnEnd && Context.IsValid())
	{
		if (UAblPlaySoundTaskScratchPad* ScratchPad = Cast<UAblPlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this)))
		{
			for (TWeakObjectPtr<UAudioComponent>& AudioComponent : ScratchPad->AttachedSounds)
			{
				if (AudioComponent.IsValid())
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, TEXT("Destroying Sound."));
					}
#endif
					AudioComponent->bAutoDestroy = true;
					AudioComponent->FadeOut(m_DestroyFadeOutDuration, 0.0f);
				}
			}
		}
	}
}

UAblAbilityTaskScratchPad* UAblPlaySoundTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (m_DestroyOnEnd)
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblPlaySoundTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblPlaySoundTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblPlaySoundTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblPlaySoundTask, STATGROUP_Able);
}

void UAblPlaySoundTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Sound, TEXT("Sound"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SoundStartTime, TEXT("Sound Start Time"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_VolumeModifier, TEXT("Volume Modifier"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_PitchModifier, TEXT("Pitch Modifier"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Attenuation, TEXT("Attenuation"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Concurrency, TEXT("Concurrency"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Location"));
}

#if WITH_EDITOR

FText UAblPlaySoundTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlaySoundTaskFormat", "{0}: {1}");
	FString SoundName = TEXT("<null>");
	if (m_Sound)
	{
		SoundName = m_Sound->GetName();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(SoundName));
}

FText UAblPlaySoundTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString SoundName;
	if (m_SoundDelegate.IsBound())
	{
		SoundName = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_SoundDelegate.GetFunctionName().ToString() });
	}
	else
	{
		SoundName = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Sound\" Filter=\"/Script/Engine.SoundBase\">{0}</>"), { m_Sound ? m_Sound->GetName() : TEXT("NULL") });
	}

	FString SoundStartString;
	if (m_SoundStartTimeDelegate.IsBound())
	{
		SoundStartString = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_SoundStartTimeDelegate.GetFunctionName().ToString() });
	}
	else
	{
		SoundStartString = FString::Format(TEXT("<a id=\"AblTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_SoundStartTime\" MinValue=\"0.0\">{0}</>"), { m_SoundStartTime });
	}

	FString VolumeString;
	if (m_VolumeModifierDelegate.IsBound())
	{
		VolumeString = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_VolumeModifierDelegate.GetFunctionName().ToString() });
	}
	else
	{
		VolumeString = FString::Format(TEXT("<a id=\"AblTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_VolumeModifier\" MinValue=\"0.0\">{0}</>"), { m_VolumeModifier });
	}

	FString PitchString;
	if (m_PitchModifierDelegate.IsBound())
	{
		PitchString = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_PitchModifierDelegate.GetFunctionName().ToString() });
	}
	else
	{
		PitchString = FString::Format(TEXT("<a id=\"AblTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_PitchModifier\" MinValue=\"0.0\">{0}</>"), { m_PitchModifier });
	}

	FString AttenuationString;
	if (m_AttenuationDelegate.IsBound())
	{
		AttenuationString = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AttenuationDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AttenuationString = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Attenuation\" Filter=\"/Script/Engine.SoundAttenuation\">{0}</>"), { m_Attenuation ? m_Attenuation->GetName() : TEXT("NULL") });
	}

	FString ConcurrencyString;
	if (m_ConcurrencyDelegate.IsBound())
	{
		ConcurrencyString = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_ConcurrencyDelegate.GetFunctionName().ToString() });
	}
	else
	{
		ConcurrencyString = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Attenuation\" Filter=\"/Script/Engine.SoundConcurrency\">{0}</>"), { m_Concurrency ? m_Concurrency->GetName() : TEXT("NULL") });
	}

	StringBuilder.AppendLineFormat(LOCTEXT("AblPlaySoundTaskRichFmt", "\t- Sound: {0}\n\t- Sound Start Time: {1}\n\t- Volume Mod: {2}\n\t- Pitch Mod: {3}\n\t- Atten: {4}\n\t- Concur: {5}"), 
								   FText::FromString(SoundName),
								   FText::FromString(SoundStartString),
								   FText::FromString(VolumeString),
								   FText::FromString(PitchString),
								   FText::FromString(AttenuationString),
								   FText::FromString(ConcurrencyString));

	return StringBuilder.ToText();
}

#endif

#undef LOCTEXT_NAMESPACE

