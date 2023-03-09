// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablSetShaderParameterTask.h"

#include "ablAbility.h"
#include "ablSubSystem.h"
#include "AbleCorePrivate.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tasks/ablSetShaderParameterValue.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

UAblSetShaderParameterTaskScratchPad::UAblSetShaderParameterTaskScratchPad()
{

}

UAblSetShaderParameterTaskScratchPad::~UAblSetShaderParameterTaskScratchPad()
{

}

UAblSetShaderParameterTask::UAblSetShaderParameterTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_ParameterName(NAME_None),
	m_Value(nullptr),
	m_RestoreValueOnEnd(false)
{

}

UAblSetShaderParameterTask::~UAblSetShaderParameterTask()
{

}

void UAblSetShaderParameterTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);

	if (!m_Value)
	{
		UE_LOG(LogAble, Warning, TEXT("No Value set for SetShaderParameter in Ability [%s]"), *Context->GetAbility()->GetDisplayName());
		return;
	}

	// We need to convert our Actors to primitive components.
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	TArray<TWeakObjectPtr<UPrimitiveComponent>> PrimitiveComponents;

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		TInlineComponentArray<UPrimitiveComponent*> TargetComponents(Target.Get());
		PrimitiveComponents.Append(TargetComponents);
	}

	UAblSetShaderParameterTaskScratchPad* ScratchPad = Cast<UAblSetShaderParameterTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);
	ScratchPad->DynamicMaterials.Empty();
	ScratchPad->PreviousValues.Empty();

	ScratchPad->BlendIn = m_BlendIn;
	ScratchPad->BlendIn.Reset();

	ScratchPad->BlendOut = m_BlendOut;
	ScratchPad->BlendOut.Reset();

	UAblSetParameterValue* CachedValue = nullptr;
	for (TWeakObjectPtr<UPrimitiveComponent>& PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent.IsValid())
		{
			for (int32 i = 0; i < PrimitiveComponent->GetNumMaterials(); ++i)
			{
				UMaterialInterface* MaterialInterface = PrimitiveComponent->GetMaterial(i);
				UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);

				if (!MaterialInterface)
				{
					continue;
				}

				// If our material currently isn't dynamic, but we have the parameter we're looking for - instantiate a dynamic version.
				if (!DynamicMaterial && CheckMaterialHasParameter(MaterialInterface))
				{
					DynamicMaterial = PrimitiveComponent->CreateDynamicMaterialInstance(i, MaterialInterface);
				}

				if (DynamicMaterial)
				{
					CachedValue = CacheShaderValue(ScratchPad, DynamicMaterial);
					if (CachedValue)
					{
						ScratchPad->DynamicMaterials.Add(DynamicMaterial);
						ScratchPad->PreviousValues.Add(CachedValue);

						if (ScratchPad->BlendIn.IsComplete())
						{
							// If there isn't any blend. Just set it here since we won't be ticking.
							InternalSetShaderValue(Context, DynamicMaterial, m_Value, CachedValue, ScratchPad->BlendIn.GetAlpha());
						}
					}
				}
			}
		}
	}

}

void UAblSetShaderParameterTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);

	UAblSetShaderParameterTaskScratchPad* ScratchPad = Cast<UAblSetShaderParameterTaskScratchPad>(Context->GetScratchPadForTask(this));
	check(ScratchPad);

	if (!ScratchPad->BlendIn.IsComplete())
	{
		ScratchPad->BlendIn.Update(deltaTime);

		verify(ScratchPad->DynamicMaterials.Num() == ScratchPad->PreviousValues.Num());
		for (int32 i = 0; i < ScratchPad->DynamicMaterials.Num(); ++i)
		{
			InternalSetShaderValue(Context, ScratchPad->DynamicMaterials[i], m_Value, ScratchPad->PreviousValues[i], ScratchPad->BlendIn.GetBlendedValue());
		}
	}
	else if (m_RestoreValueOnEnd && !ScratchPad->BlendOut.IsComplete())
	{
		// If we're within range to start blending out, go ahead and start that process.
		if (GetEndTime() - Context->GetCurrentTime() < ScratchPad->BlendOut.GetBlendTime())
		{
			ScratchPad->BlendOut.Update(deltaTime);
			
			verify(ScratchPad->DynamicMaterials.Num() == ScratchPad->PreviousValues.Num());
			for (int32 i = 0; i < ScratchPad->DynamicMaterials.Num(); ++i)
			{
				InternalSetShaderValue(Context, ScratchPad->DynamicMaterials[i], ScratchPad->PreviousValues[i], m_Value, ScratchPad->BlendOut.GetBlendedValue());
			}
		}
	}
}

void UAblSetShaderParameterTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult Result) const
{
	Super::OnTaskEnd(Context, Result);

	if (m_RestoreValueOnEnd && Context.IsValid())
	{
		UAblSetShaderParameterTaskScratchPad* ScratchPad = Cast<UAblSetShaderParameterTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(ScratchPad);

		verify(ScratchPad->DynamicMaterials.Num() == ScratchPad->PreviousValues.Num());
		for (int32 i = 0; i < ScratchPad->DynamicMaterials.Num(); ++i)
		{
			InternalSetShaderValue(Context, ScratchPad->DynamicMaterials[i], ScratchPad->PreviousValues[i], m_Value, 1.0);
		}
	}

}

UAblAbilityTaskScratchPad* UAblSetShaderParameterTask::CreateScratchPad(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	if (UAblAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAblAbilityTaskScratchPad> ScratchPadClass = UAblSetShaderParameterTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblSetShaderParameterTaskScratchPad>(Context.Get());
}

TStatId UAblSetShaderParameterTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblSetShaderParameterTask, STATGROUP_Able);
}

UAblSetParameterValue* UAblSetShaderParameterTask::CacheShaderValue(UAblSetShaderParameterTaskScratchPad* ScratchPad, UMaterialInstanceDynamic* DynMaterial) const
{
	check(DynMaterial);
	UAblSetParameterValue* Value = nullptr;
	if (m_Value->GetType() == UAblSetParameterValue::Scalar)
	{
		float Scalar = 0.0f;
		if (DynMaterial->GetScalarParameterValue(m_ParameterName, Scalar))
		{
			UAblSetScalarParameterValue* ScalarValue = NewObject<UAblSetScalarParameterValue>(ScratchPad);
			ScalarValue->SetScalar(Scalar);
			Value = ScalarValue;
		}
	}
	else if (m_Value->GetType() == UAblSetParameterValue::Vector)
	{
		FLinearColor VectorParam;
		if (DynMaterial->GetVectorParameterValue(m_ParameterName, VectorParam))
		{
			UAblSetVectorParameterValue* VectorValue = NewObject<UAblSetVectorParameterValue>(ScratchPad);
			VectorValue->SetColor(VectorParam);
			Value = VectorValue;
		}
	}
	else if (m_Value->GetType() == UAblSetParameterValue::Texture)
	{
		UTexture* Texture = nullptr;
		if (DynMaterial->GetTextureParameterValue(m_ParameterName, Texture))
		{
			UAblSetTextureParameterValue* TextureValue = NewObject<UAblSetTextureParameterValue>(ScratchPad);
			TextureValue->SetTexture(Texture);
			Value = TextureValue;
		}
	}
	else
	{
		checkNoEntry();
	}

	return Value;
}

void UAblSetShaderParameterTask::InternalSetShaderValue(const TWeakObjectPtr<const UAblAbilityContext>& Context, UMaterialInstanceDynamic* DynMaterial, UAblSetParameterValue* Value, UAblSetParameterValue* PreviousValue, float BlendAlpha) const
{
	check(DynMaterial);
	check(Value);
	check(PreviousValue);

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Setting material parameter %s on Material %s to %s with a blend of %1.4f."), *m_ParameterName.ToString(), *DynMaterial->GetName(), *Value->ToString(), BlendAlpha));
	}
#endif

	if (UAblSetScalarParameterValue* ScalarValue = Cast<UAblSetScalarParameterValue>(Value))
	{
		UAblSetScalarParameterValue* PreviousScalarValue = CastChecked<UAblSetScalarParameterValue>(PreviousValue);
		float InterpolatedValue = FMath::Lerp(PreviousScalarValue->GetScalar(Context), ScalarValue->GetScalar(Context), BlendAlpha);
		DynMaterial->SetScalarParameterValue(m_ParameterName, InterpolatedValue);
	}
	else if (UAblSetVectorParameterValue* VectorValue = Cast<UAblSetVectorParameterValue>(Value))
	{
		UAblSetVectorParameterValue* PreviousVectorValue = CastChecked<UAblSetVectorParameterValue>(PreviousValue);
		FLinearColor InterpolatedValue = FMath::Lerp(PreviousVectorValue->GetColor(Context), VectorValue->GetColor(Context), BlendAlpha);
		DynMaterial->SetVectorParameterValue(m_ParameterName, InterpolatedValue);
	}
	else if (UAblSetTextureParameterValue* TextureValue = Cast<UAblSetTextureParameterValue>(Value))
	{
		// No Lerping allowed.
		DynMaterial->SetTextureParameterValue(m_ParameterName, TextureValue->GetTexture(Context));
	}
	else
	{
		checkNoEntry();
	}
}


bool UAblSetShaderParameterTask::CheckMaterialHasParameter(UMaterialInterface* Material) const
{
	if (UAblSetScalarParameterValue* ScalarValue = Cast<UAblSetScalarParameterValue>(m_Value))
	{
		float TempScalar;
		return Material->GetScalarParameterValue(m_ParameterName, TempScalar);
	}
	else if (UAblSetVectorParameterValue* VectorValue = Cast<UAblSetVectorParameterValue>(m_Value))
	{
		FLinearColor TempVector;
		return Material->GetVectorParameterValue(m_ParameterName, TempVector);
	}
	else if (UAblSetTextureParameterValue* TextureValue = Cast<UAblSetTextureParameterValue>(m_Value))
	{
		UTexture* TempTexture;
		return Material->GetTextureParameterValue(m_ParameterName, TempTexture);
	}

	UE_LOG(LogAble, Warning, TEXT("Could not find Parameter [%s] in Material [%s]"), *m_ParameterName.ToString(), *Material->GetName());
	return false;
}

void UAblSetShaderParameterTask::BindDynamicDelegates(UAblAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	if (m_Value)
	{
		m_Value->BindDynamicDelegates(Ability);
	}
}

#if WITH_EDITOR

FText UAblSetShaderParameterTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblPlaySetShaderParameterTaskFormat", "{0}: {1}");
	FString ParameterName = TEXT("<none>");
	if (!m_ParameterName.IsNone())
	{
		ParameterName = m_ParameterName.ToString();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ParameterName));
}

void UAblSetShaderParameterTask::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	float MinimumRequiredTimeForBlending = m_BlendIn.GetBlendTime();

	if (m_RestoreValueOnEnd)
	{
		MinimumRequiredTimeForBlending += m_BlendOut.GetBlendTime();
	}

	m_EndTime = FMath::Max(m_EndTime, MinimumRequiredTimeForBlending);
}

bool UAblSetShaderParameterTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	if (m_Value)
	{
		modified |= m_Value->FixUpObjectFlags();
	}

	return modified;
}

#endif

#undef LOCTEXT_NAMESPACE