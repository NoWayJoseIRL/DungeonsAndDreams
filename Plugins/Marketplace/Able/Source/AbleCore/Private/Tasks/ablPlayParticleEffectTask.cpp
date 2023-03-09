// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablPlayParticleEffectTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Components/SkeletalMeshComponent.h"
#include "Runtime/Engine/Public/ParticleEmitterInstances.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Niagara/Public/NiagaraFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblPlayParticleEffectTaskScratchPad::UAblPlayParticleEffectTaskScratchPad()
{

}

UAblPlayParticleEffectTaskScratchPad::~UAblPlayParticleEffectTaskScratchPad()
{

}

UAblPlayParticleEffectTask::UAblPlayParticleEffectTask(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
    m_EffectTemplate(nullptr),
    m_NiagaraEffectTemplate(nullptr),
    m_AttachToSocket(false),
    m_Scale(1.0f),
    m_DynamicScaleSize(0.0f),
    m_DestroyAtEnd(false)
{

}

UAblPlayParticleEffectTask::~UAblPlayParticleEffectTask()
{

}

void UAblPlayParticleEffectTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
    Super::OnTaskStart(Context);

    if (!m_EffectTemplate && !m_NiagaraEffectTemplate)
    {
        UE_LOG(LogAble, Warning, TEXT("No Particle System set for PlayParticleEffectTask in Ability [%s]"), *Context->GetAbility()->GetDisplayName());
        return;
    }

    FTransform OffsetTransform(FTransform::Identity);
    FTransform SpawnTransform = OffsetTransform;

	FAblAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);
    UParticleSystem* EffectTemplate = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_EffectTemplate);
    UNiagaraSystem* NiagaraEffectTemplate = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_NiagaraEffectTemplate);

	float Scale = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Scale);
	float DynamicScale = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_DynamicScaleSize);

    TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect = nullptr;
    TWeakObjectPtr<UNiagaraComponent> SpawnedNiagaraEffect = nullptr;

    UAblPlayParticleEffectTaskScratchPad* ScratchPad = nullptr;
    if (m_DestroyAtEnd)
    {
        ScratchPad = Cast<UAblPlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
		ScratchPad->SpawnedEffects.Empty();
		ScratchPad->SpawnedNiagaraEffects.Empty();
    }

    TArray<TWeakObjectPtr<AActor>> TargetArray;
    GetActorsForTask(Context, TargetArray);

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

        FVector emitterScale(Scale);

        if (DynamicScale > 0.0f)
        {
            float targetRadius = Target->GetSimpleCollisionRadius();
            float scaleFactor = targetRadius / DynamicScale;
            emitterScale = FVector(scaleFactor);
        }

        SpawnTransform.SetScale3D(emitterScale);
        
#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Spawning Emitter %s with Transform %s, Scale [%4.2f] Dynamic Scale [%4.2f] Final Scale [%4.2f]"),
                EffectTemplate ? *EffectTemplate->GetName() : *NiagaraEffectTemplate->GetName(), *SpawnTransform.ToString(), Scale, DynamicScale, emitterScale.X));
        }
#endif
        
        if (m_AttachToSocket)
        {
			USceneComponent* AttachComponent = Target->FindComponentByClass<USceneComponent>();
            if (EffectTemplate)
            {
                SpawnedEffect = UGameplayStatics::SpawnEmitterAttached(EffectTemplate, AttachComponent, Location.GetSocketName(), Location.GetOffset(), Location.GetRotation(), SpawnTransform.GetScale3D());
            }
            else
            {
                SpawnedNiagaraEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraEffectTemplate, AttachComponent, Location.GetSocketName(), Location.GetOffset(), m_Location.GetRotation(), SpawnTransform.GetScale3D(), EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None);
            }
        }
        else
        {
            if (EffectTemplate)
            {
                SpawnedEffect = UGameplayStatics::SpawnEmitterAtLocation(Target->GetWorld(), EffectTemplate, SpawnTransform);
            }
            else
            {
                SpawnedNiagaraEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Target->GetWorld(), NiagaraEffectTemplate, SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator(), SpawnTransform.GetScale3D());
            }
        }

        if (m_DestroyAtEnd && ScratchPad)
        {
            if (SpawnedEffect.IsValid())
            {
                ScratchPad->SpawnedEffects.Add(SpawnedEffect);
            }

            if (SpawnedNiagaraEffect.IsValid())
            {
                ScratchPad->SpawnedNiagaraEffects.Add(SpawnedNiagaraEffect);
            }
        }

        // Set our Parameters.
        for (UAblParticleEffectParam* Parameter : m_Parameters)
        {
			UFXSystemComponent* SpawnedEffectComponent = SpawnedNiagaraEffect.IsValid() ? Cast<UFXSystemComponent>(SpawnedNiagaraEffect.Get()) : Cast<UFXSystemComponent>(SpawnedEffect.Get());
			if (!SpawnedEffectComponent)
			{
				break;
			}

            if (Parameter->IsA<UAblParticleEffectParamContextActor>())
            {
                UAblParticleEffectParamContextActor* ContextActorParam = Cast<UAblParticleEffectParamContextActor>(Parameter);
                if (ContextActorParam->GetContextActorType() == EAblAbilityTargetType::ATT_TargetActor)
                {
					SpawnedEffectComponent->SetActorParameter(Parameter->GetParameterName(), Target.Get());
                }
                else if (AActor* FoundActor = GetSingleActorFromTargetType(Context, ContextActorParam->GetContextActorType()))
                {
					SpawnedEffectComponent->SetActorParameter(Parameter->GetParameterName(), FoundActor);
                }
            }
			else
			{
				ApplyParticleEffectParameter(Parameter, Context, SpawnedEffectComponent, SpawnedEffect);
			}
        }
    }
}

void UAblPlayParticleEffectTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

	if (Context.IsValid())
	{
		UAblPlayParticleEffectTaskScratchPad* ScratchPad = Cast<UAblPlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);

		for (TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect : ScratchPad->SpawnedEffects)
		{
			if (SpawnedEffect.IsValid())
			{
				if (UFXSystemComponent* SpawnedEffectSystem = Cast<UFXSystemComponent>(SpawnedEffect.Get()))
				{
					for (const UAblParticleEffectParam* param : m_Parameters)
					{
						if (param->GetEnableTickUpdate())
						{
							ApplyParticleEffectParameter(param, Context, SpawnedEffectSystem, SpawnedEffect);
						}
					}
				}
			}
		}

		TWeakObjectPtr<UParticleSystemComponent> InvalidComponent(nullptr);

		for (TWeakObjectPtr<UNiagaraComponent> SpawnedEffect : ScratchPad->SpawnedNiagaraEffects)
		{
			if (SpawnedEffect.IsValid())
			{
				if (UFXSystemComponent* SpawnedEffectSystem = Cast<UFXSystemComponent>(SpawnedEffect.Get()))
				{
					for (const UAblParticleEffectParam* param : m_Parameters)
					{
						if (param->GetEnableTickUpdate())
						{
							ApplyParticleEffectParameter(param, Context, SpawnedEffectSystem, InvalidComponent);
						}
					}
				}
			}
		}
	}
}

void UAblPlayParticleEffectTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const
{
    Super::OnTaskEnd(Context, Result);

    if (m_DestroyAtEnd && Context.IsValid())
    {
        UAblPlayParticleEffectTaskScratchPad* ScratchPad = Cast<UAblPlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
        check(ScratchPad);

        for (TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect : ScratchPad->SpawnedEffects)
        {
            if (SpawnedEffect.IsValid())
            {
#if !(UE_BUILD_SHIPPING)
                if (IsVerbose())
                {
                    PrintVerbose(Context, FString::Printf(TEXT("Destroying Emitter %s"), *SpawnedEffect->GetName()));
                }
#endif
                SpawnedEffect->bAutoDestroy = true;
                SpawnedEffect->DeactivateSystem();
            }
        }

        for (TWeakObjectPtr<UNiagaraComponent> SpawnedEffect : ScratchPad->SpawnedNiagaraEffects)
        {
            if (SpawnedEffect.IsValid())
            {
#if !(UE_BUILD_SHIPPING)
                if (IsVerbose())
                {
                    PrintVerbose(Context, FString::Printf(TEXT("Destroying Emitter %s"), *SpawnedEffect->GetName()));
                }
#endif
                SpawnedEffect->SetAutoDestroy(true);
                SpawnedEffect->Deactivate();
            }
        }
    }
}

bool UAblPlayParticleEffectTask::NeedsTick() const
{
	return m_Parameters.Num() != 0 && m_Parameters.FindByPredicate([](const UAblParticleEffectParam* RHS){ return RHS && RHS->GetEnableTickUpdate(); }) != nullptr;
}

UAblAbilityTaskScratchPad* UAblPlayParticleEffectTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (m_DestroyAtEnd)
	{
		if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblPlayParticleEffectTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblPlayParticleEffectTaskScratchPad>(Context.Get());
	}

    return nullptr;
}

TStatId UAblPlayParticleEffectTask::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAblPlayParticleEffectTask, STATGROUP_Able);
}

void UAblPlayParticleEffectTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Location"));
    ABL_BIND_DYNAMIC_PROPERTY(Ability, m_EffectTemplate, TEXT("Effect Template"));
    ABL_BIND_DYNAMIC_PROPERTY(Ability, m_NiagaraEffectTemplate, TEXT("Niagara Template"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Scale, TEXT("Scale"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_DynamicScaleSize, TEXT("Dynamic Scale"));

	for (UAblParticleEffectParam* Param : m_Parameters)
	{
		if (Param)
		{
			Param->BindDynamicDelegates(Ability);
		}
	}
}

void UAblPlayParticleEffectTask::ApplyParticleEffectParameter(const UAblParticleEffectParam* Parameter, const TWeakObjectPtr<const UAblAbilityContext>& Context, UFXSystemComponent* EffectSystem, TWeakObjectPtr<UParticleSystemComponent>& CascadeComponent) const
{
	if (!EffectSystem)
	{
		return;
	}

	if (Parameter->IsA<UAblParticleEffectParamContextActor>())
	{
		const UAblParticleEffectParamContextActor* ContextActorParam = Cast<UAblParticleEffectParamContextActor>(Parameter);
		if (ContextActorParam->GetContextActorType() != EAblAbilityTargetType::ATT_TargetActor)
		{
			if (AActor* FoundActor = GetSingleActorFromTargetType(Context, ContextActorParam->GetContextActorType()))
			{
				EffectSystem->SetActorParameter(Parameter->GetParameterName(), FoundActor);
			}
		}
	}
	else if (Parameter->IsA<UAblParticleEffectParamLocation>())
	{
		const UAblParticleEffectParamLocation* LocationParam = Cast<UAblParticleEffectParamLocation>(Parameter);
		FTransform outTransform;
		LocationParam->GetLocation(Context).GetTransform(*Context.Get(), outTransform);
		EffectSystem->SetVectorParameter(Parameter->GetParameterName(), outTransform.GetTranslation());
	}
	else if (Parameter->IsA<UAblParticleEffectParamFloat>())
	{
		const UAblParticleEffectParamFloat* FloatParam = Cast<UAblParticleEffectParamFloat>(Parameter);
		EffectSystem->SetFloatParameter(Parameter->GetParameterName(), FloatParam->GetFloat(Context));
	}
	else if (Parameter->IsA<UAblParticleEffectParamColor>())
	{
		const UAblParticleEffectParamColor* ColorParam = Cast<UAblParticleEffectParamColor>(Parameter);
		EffectSystem->SetColorParameter(Parameter->GetParameterName(), ColorParam->GetColor(Context));
	}
	else if (Parameter->IsA<UAblParticleEffectParamMaterial>() && CascadeComponent.IsValid())
	{
		const UAblParticleEffectParamMaterial* MaterialParam = Cast<UAblParticleEffectParamMaterial>(Parameter);
		CascadeComponent->SetMaterialParameter(Parameter->GetParameterName(), MaterialParam->GetMaterial(Context));
	}
	else if (Parameter->IsA<UAblParticleEffectParamVector>())
	{
		const UAblParticleEffectParamVector* VectorParam = Cast<UAblParticleEffectParamVector>(Parameter);
		EffectSystem->SetVectorParameter(Parameter->GetParameterName(), VectorParam->GetVector(Context));
	}
	else if (Parameter->IsA<UAblParticleEffectParamBool>())
	{
		const UAblParticleEffectParamBool* BoolParam = Cast<UAblParticleEffectParamBool>(Parameter);
		EffectSystem->SetBoolParameter(Parameter->GetParameterName(), BoolParam->GetBool(Context));
	}
}


#if WITH_EDITOR

FText UAblPlayParticleEffectTask::GetDescriptiveTaskName() const
{
    const FText FormatText = LOCTEXT("AblPlayParticleEffectTaskFormat", "{0}: {1}");
    FString ParticleName = TEXT("<null>");
    if (m_EffectTemplate)
    {
        ParticleName = m_EffectTemplate->GetName();
    }
    return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ParticleName));
}

FText UAblPlayParticleEffectTask::GetRichTextTaskSummary() const
{
    FTextBuilder StringBuilder;

    StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

    FString EffectName = TEXT("NULL");
    if (m_EffectTemplateDelegate.IsBound())
    {
        EffectName = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_EffectTemplateDelegate.GetFunctionName().ToString() });
    }
    else
    {
        EffectName = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_EffectTemplate\" Filter=\"/Script/Engine.ParticleSystem\">{0}</>"), { m_EffectTemplate ? m_EffectTemplate->GetName() : EffectName });
    }

    StringBuilder.AppendLineFormat(LOCTEXT("AblPlayParticleEffectTaskRichFmt1", "\t- Effect: {0}"), FText::FromString(EffectName));

    EffectName = TEXT("NULL");
    if (m_NiagaraEffectTemplateDelegate.IsBound())
    {
        EffectName = FString::Format(TEXT("<a id=\"AblTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_NiagaraEffectTemplateDelegate.GetFunctionName().ToString() });
    }
    else
    {
        EffectName = FString::Format(TEXT("<a id=\"AblTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_NiagaraEffectTemplate\" Filter=\"/Script/Niagara.NiagaraSystem\">{0}</>"), { m_NiagaraEffectTemplate ? m_NiagaraEffectTemplate->GetName() : EffectName });
    }

    StringBuilder.AppendLineFormat(LOCTEXT("AblPlayParticleEffectTaskRichFmt2", "\t- Niagara Effect: {0}"), FText::FromString(EffectName));

    return StringBuilder.ToText();
}

EDataValidationResult UAblPlayParticleEffectTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}

bool UAblPlayParticleEffectTask::FixUpObjectFlags()
{
	return false;
}

#endif

#undef LOCTEXT_NAMESPACE

