// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/IAblAbilityTask.h"
#include "AbleCorePrivate.h"
#include "ablAbility.h"
#include "ablAbilityComponent.h"
#include "ablAbilityContext.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Serialization/ArchiveCountMem.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h"
#include "Targeting/ablTargetingBase.h"

#if !(UE_BUILD_SHIPPING)
#include "ablSettings.h"
#include "ablAbilityUtilities.h"
#endif

#if WITH_EDITOR
#include "Editor.h"
#endif
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Engine/LocalPlayer.h"
#include "Engine/NetDriver.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "AblAbilityTask"

// TODO: Move this to AblUtils or some such.
namespace InstancedPropertyUtils
{
	struct FCPFUOArchive
	{
	public:
		FCPFUOArchive(bool bIncludeUntaggedDataIn)
			: bIncludeUntaggedData(bIncludeUntaggedDataIn)
			, TaggedDataScope(0)
		{}

	protected:
		FORCEINLINE void OpenTaggedDataScope() { ++TaggedDataScope; }
		FORCEINLINE void CloseTaggedDataScope() { --TaggedDataScope; }

		FORCEINLINE bool IsSerializationEnabled()
		{
			return bIncludeUntaggedData || (TaggedDataScope > 0);
		}

		bool bIncludeUntaggedData;
	private:
		int32 TaggedDataScope;
	};

	/* Serializes and stores property data from a specified 'source' object. Only stores data compatible with a target destination object. */
	struct FCPFUOWriter : public FObjectWriter, public FCPFUOArchive
	{
	public:
		/* Contains the source object's serialized data */
		TArray<uint8> SavedPropertyData;

	public:
		FCPFUOWriter(UObject* SrcObject, const UEngine::FCopyPropertiesForUnrelatedObjectsParams& Params)
			: FObjectWriter(SavedPropertyData)
			// if the two objects don't share a common native base class, then they may have different
			// serialization methods, which is dangerous (the data is not guaranteed to be homogeneous)
			// in that case, we have to stick with tagged properties only
			, FCPFUOArchive(true)
			, bSkipCompilerGeneratedDefaults(Params.bSkipCompilerGeneratedDefaults)
		{
			ArIgnoreArchetypeRef = true;
			ArNoDelta = !Params.bDoDelta;
			ArIgnoreClassRef = true;
			ArPortFlags |= Params.bCopyDeprecatedProperties ? PPF_UseDeprecatedProperties : PPF_None;

#if USE_STABLE_LOCALIZATION_KEYS
			if (GIsEditor && !(ArPortFlags & (PPF_DuplicateVerbatim | PPF_DuplicateForPIE)))
			{
				SetLocalizationNamespace(TextNamespaceUtil::EnsurePackageNamespace(SrcObject));
			}
#endif // USE_STABLE_LOCALIZATION_KEYS

			SrcObject->Serialize(*this);
		}

		//~ Begin FArchive Interface
		virtual void Serialize(void* Data, int64 Num) override
		{
			if (IsSerializationEnabled())
			{
				FObjectWriter::Serialize(Data, Num);
			}
		}

		virtual void MarkScriptSerializationStart(const UObject* Object) override { OpenTaggedDataScope(); }
		virtual void MarkScriptSerializationEnd(const UObject* Object) override { CloseTaggedDataScope(); }

#if WITH_EDITOR
		virtual bool ShouldSkipProperty(const class FProperty* InProperty) const override
		{
			static FName BlueprintCompilerGeneratedDefaultsName(TEXT("BlueprintCompilerGeneratedDefaults"));
			return bSkipCompilerGeneratedDefaults && InProperty->HasMetaData(BlueprintCompilerGeneratedDefaultsName);
		}
#endif 
		//~ End FArchive Interface

	private:
		static UClass* FindNativeSuperClass(UObject* Object)
		{
			UClass* Class = Object->GetClass();
			for (; Class; Class = Class->GetSuperClass())
			{
				if ((Class->ClassFlags & CLASS_Native) != 0)
				{
					break;
				}
			}
			return Class;
		}

		bool bSkipCompilerGeneratedDefaults;
	};

	/* Responsible for applying the saved property data from a FCPFUOWriter to a specified object */
	struct FCPFUOReader : public FObjectReader, public FCPFUOArchive
	{
	public:
		FCPFUOReader(TArray<uint8>& DataSrc, UObject* DstObject)
			: FObjectReader(DataSrc)
			, FCPFUOArchive(true)
		{
			ArIgnoreArchetypeRef = true;
			ArIgnoreClassRef = true;

#if USE_STABLE_LOCALIZATION_KEYS
			if (GIsEditor && !(ArPortFlags & (PPF_DuplicateVerbatim | PPF_DuplicateForPIE)))
			{
				SetLocalizationNamespace(TextNamespaceUtil::EnsurePackageNamespace(DstObject));
			}
#endif // USE_STABLE_LOCALIZATION_KEYS

			DstObject->Serialize(*this);
		}

		//~ Begin FArchive Interface
		virtual void Serialize(void* Data, int64 Num) override
		{
			if (IsSerializationEnabled())
			{
				FObjectReader::Serialize(Data, Num);
			}
		}

		virtual void MarkScriptSerializationStart(const UObject* Object) override { OpenTaggedDataScope(); }
		virtual void MarkScriptSerializationEnd(const UObject* Object) override { CloseTaggedDataScope(); }
		// ~End FArchive Interface
	};
}


UAblAbilityTask::UAblAbilityTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_StartTime(0.0f),
	m_EndTime(1.0f),
    m_Inheritable(false),
	m_Verbose(false),
	m_Disabled(false),
	m_TaskColor(FLinearColor::Black),
	m_DynamicPropertyIdentifer(),
	m_ResetForIteration(true)
#if WITH_EDITORONLY_DATA
	, m_Locked(false)
#endif
{
	
}

UAblAbilityTask::~UAblAbilityTask()
{

}

void UAblAbilityTask::PostLoad()
{
    Super::PostLoad();

    // remove any broken references
    const int32 numDependencies = m_Dependencies.Num();
    m_Dependencies.RemoveAll([](const UAblAbilityTask* Source) { return Source == nullptr; });
    if (numDependencies < m_Dependencies.Num())
    {
        UE_LOG(LogAble, Warning, TEXT("UAblAbilityTask.PostLoad() %s had %d NULL Dependencies."), *GetClass()->GetName(), m_Dependencies.Num() - numDependencies);
    }
}

void UAblAbilityTask::PostInitProperties()
{
	Super::PostInitProperties();

	if (!m_TaskTargets.Num())
	{
		m_TaskTargets.Add(EAblAbilityTargetType::ATT_Self);
	}
}

UWorld* UAblAbilityTask::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}
	return GetOuter()->GetWorld();
}

int32 UAblAbilityTask::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking())
	{
		// This handles absorbing authority/cosmetic
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UAblAbilityTask::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	check(GetOuter() != nullptr);

	AActor* Owner = CastChecked<AActor>(GetOuter());

	bool bProcessed = false;

	FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
	if (Context != nullptr)
	{
		for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
		{
			if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(Owner, Function))
			{
				Driver.NetDriver->ProcessRemoteFunction(Owner, Function, Parameters, OutParms, Stack, this);
				bProcessed = true;
			}
		}
	}

	return bProcessed;
}

bool UAblAbilityTask::CanStart(const TWeakObjectPtr<const UAblAbilityContext>& Context, float CurrentTime, float DeltaTime) const
{
	return CurrentTime + DeltaTime >= GetStartTime();
}

void UAblAbilityTask::OnTaskStart(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
#if !(UE_BUILD_SHIPPING)
	if (IsVerbose() && Context.IsValid())
	{
		if (AActor* Owner = Context->GetOwner())
		{
			if (const UWorld* World = Context->GetOwner()->GetWorld())
			{
				PrintVerbose(Context, FString::Printf(TEXT("OnTaskStart called for Task %s at time %f."),
					*GetName(), Context->GetCurrentTime()));
			}
		}
	}
#endif
}

void UAblAbilityTask::OnTaskTick(const TWeakObjectPtr<const UAblAbilityContext>& Context, float deltaTime) const
{
#if !(UE_BUILD_SHIPPING)
	if (IsVerbose() && Context.IsValid())
	{
		PrintVerbose(Context, FString::Printf(TEXT("OnTaskTick called for Task %s at time %2.2f. Delta Time = %1.5f"), *GetName(), Context->GetCurrentTime(), deltaTime));
	}
#endif
}

bool UAblAbilityTask::IsDone(const TWeakObjectPtr<const UAblAbilityContext>& Context) const
{
	return Context.IsValid() ? Context->GetCurrentTime() >= GetEndTime() : true;
}

void UAblAbilityTask::OnTaskEnd(const TWeakObjectPtr<const UAblAbilityContext>& Context, const EAblAbilityTaskResult result) const
{
#if !(UE_BUILD_SHIPPING)
	if (IsVerbose() && Context.IsValid())
	{
		if (AActor* Owner = Context->GetOwner())
		{
			if (const UWorld* World = Context->GetOwner()->GetWorld())
			{
				PrintVerbose(Context, FString::Printf(TEXT("OnTaskEnd called for Task %s at time %f. Task Result = %s."),
					*GetName(), Context->GetCurrentTime(), *FAbleLogHelper::GetTaskResultEnumAsString(result)));
			}
		}
	}
#endif
}

float UAblAbilityTask::GetCurrentDurationAsRatio(const UAblAbilityContext* Context) const
{
	const float currentDuration = FMath::Max(Context->GetCurrentTime() - GetStartTime(),0.0f);
	return FMath::Clamp(currentDuration / GetDuration(), 0.0f, 1.0f);
}

bool UAblAbilityTask::IsDisabled(const TWeakObjectPtr<UAblAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Disabled);
}

bool UAblAbilityTask::IsValidForNetMode(ENetMode NetMode) const
{
	if (NetMode == NM_Standalone)
	{
		return true;
	}

	switch (GetTaskRealm())
	{
	case EAblAbilityTaskRealm::ATR_Client:
	{
		return NetMode == NM_Client || NetMode == NM_ListenServer;
	}
	break;
	case EAblAbilityTaskRealm::ATR_Server:
	{
		return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
	}
	break;
	case EAblAbilityTaskRealm::ATR_ClientAndServer:
	{
		return true;
	}
	break;
	default:
		break;
	}

	return false;
}

FName UAblAbilityTask::GetDynamicDelegateName( const FString& DisplayName ) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_") + DisplayName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

void UAblAbilityTask::BindDynamicDelegates(UAblAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Disabled, TEXT("Disabled"));
}

#if WITH_EDITOR

FText UAblAbilityTask::GetRichTextTaskSummary() const
{
	FTextBuilder SummaryString;

	FNumberFormattingOptions timeFormat;
	timeFormat.MaximumFractionalDigits = 2;
	timeFormat.MinimumFractionalDigits = 2;

	SummaryString.AppendLine(GetTaskName());

    if (!m_DynamicPropertyIdentifer.IsEmpty())
    {
        SummaryString.AppendLine(FString::Format(TEXT("Dynamic Identifier: {0}"), { *m_DynamicPropertyIdentifer }));
    }

	SummaryString.AppendLineFormat(LOCTEXT("AblAbilityTaskSummaryFormat", "\t - Start Time <a id=\"AblTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_StartTime\" MinValue=\"0.0\">{0}</>"),
		FText::AsNumber(GetStartTime(), &timeFormat));

	if (IsSingleFrame())
	{
		SummaryString.AppendLine(LOCTEXT("AblAbilityTaskSummarySingleFrame", "\t - End Time: Single Frame"));
	}
	else
	{
		SummaryString.AppendLineFormat(LOCTEXT("AblAbilityTaskSummaryEndTimeFmt", "\t - End Time <a id=\"AblTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_EndTime\" MinValue=\"{0}\">{1}</>"),
			FText::AsNumber(GetStartTime(), &timeFormat),
			FText::AsNumber(GetEndTime(), &timeFormat));
	}

	if (GetTaskDependencies().Num())
	{
		SummaryString.AppendLine(LOCTEXT("AblAbilityTaskDependencies",  "\t - Dependencies: "));
		FText DependencyFmt = LOCTEXT("AblAbilityTaskDependencyFmt", "\t\t - <a id=\"AblTextDecorators.TaskDependency\" style=\"RichText.Hyperlink\" Index=\"{0}\">{1}</>");
		const TArray<const UAblAbilityTask*>& Dependencies = GetTaskDependencies();
		for (int i = 0; i < Dependencies.Num(); ++i)
		{
			if (!Dependencies[i])
			{
				continue;
			}

			SummaryString.AppendLineFormat(DependencyFmt, i, GetTaskDependencies()[i]->GetTaskName());
		}
	}

	if (m_Disabled)
	{
		SummaryString.AppendLine(LOCTEXT("AblAbilityTaskDisabled", "(DISABLED)"));
	}
	else if (m_DisabledDelegate.IsBound())
	{
		SummaryString.AppendLine(LOCTEXT("AblAbilityTaskDisabled", "(Dynamic Disable)"));
	}

	return SummaryString.ToText();
}

float UAblAbilityTask::GetEstimatedTaskCost() const
{
	float EstimatedCost = 0.0f;
	if (!IsSingleFrame())
	{
		EstimatedCost += IsAsyncFriendly() ? ABLTASK_EST_ASYNC : ABLTASK_EST_SYNC;
	}
	EstimatedCost += NeedsTick() ? ABLTASK_EST_TICK : 0.0f;

	return EstimatedCost;
}

void UAblAbilityTask::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	m_StartTime = FMath::Max(0.0f, m_StartTime);
	m_EndTime = FMath::Max(m_EndTime, m_StartTime + 0.001f);

#if WITH_EDITORONLY_DATA
	m_OnTaskPropertyModified.Broadcast(*this, PropertyChangedEvent);
#endif
}

void UAblAbilityTask::AddDependency(const UAblAbilityTask* Task)
{
	m_Dependencies.AddUnique(Task);

#if WITH_EDITORONLY_DATA
	FPropertyChangedEvent ChangeEvent(GetClass()->FindPropertyByName(FName(TEXT("m_Dependencies"))));
	m_OnTaskPropertyModified.Broadcast(*this, ChangeEvent);
#endif
}

void UAblAbilityTask::RemoveDependency(const UAblAbilityTask* Task)
{
	m_Dependencies.Remove(Task);

#if WITH_EDITORONLY_DATA
	FPropertyChangedEvent ChangeEvent(GetClass()->FindPropertyByName(FName(TEXT("m_Dependencies"))));
	m_OnTaskPropertyModified.Broadcast(*this, ChangeEvent);
#endif
}

bool UAblAbilityTask::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();
	// Make sure our flags match our packages.
	SetFlags(GetOuter()->GetMaskedFlags(RF_PropagateToSubObjects));

	if (oldFlags != GetFlags())
	{
		Modify();

		return true;
	}

	return false;
}

void UAblAbilityTask::CopyProperties(UAblAbilityTask& source)
{
	UEngine::FCopyPropertiesForUnrelatedObjectsParams copyParams;
	copyParams.bDoDelta = true;
	copyParams.bCopyDeprecatedProperties = true;
	copyParams.bSkipCompilerGeneratedDefaults = true;
	copyParams.bClearReferences = false;
	copyParams.bNotifyObjectReplacement = true;
	InstancedPropertyUtils::FCPFUOWriter FCWriter(&source, copyParams);
	InstancedPropertyUtils::FCPFUOReader FCReader(FCWriter.SavedPropertyData, this);
}

void UAblAbilityTask::GetCompactData(FAblCompactTaskData& output)
{
	UClass* OurClass = GetClass();
	output.TaskClass = OurClass;
	for (const UAblAbilityTask* Dependency : m_Dependencies)
	{
		output.Dependencies.Add(TSoftObjectPtr<const UAblAbilityTask>(Dependency));
	}
	
	output.DataBlob.Empty();

	UEngine::FCopyPropertiesForUnrelatedObjectsParams copyParams;
	copyParams.bDoDelta = true;
	copyParams.bCopyDeprecatedProperties = true;
	copyParams.bSkipCompilerGeneratedDefaults = true;
	copyParams.bClearReferences = false;
	copyParams.bNotifyObjectReplacement = true;
	InstancedPropertyUtils::FCPFUOWriter FCWriter(this, copyParams);

	output.DataBlob.Append(FCWriter.SavedPropertyData);
}

void UAblAbilityTask::LoadCompactData(FAblCompactTaskData& input)
{
	if (!input.DataBlob.Num())
	{
		UE_LOG(LogAble, Warning, TEXT("Tried to load Task [%s] saved data - but it was empty!\n"), *GetTaskName().ToString());
		return;
	}
	
	InstancedPropertyUtils::FCPFUOReader FCReader(input.DataBlob, this);

	m_Dependencies.Empty();
	for (const TSoftObjectPtr<const UAblAbilityTask> Dependency : input.Dependencies)
	{
		if (const UAblAbilityTask* Task = Dependency.Get())
		{
			m_Dependencies.Add(Task);
		}
		else
		{
			UE_LOG(LogAble, Warning, TEXT("Could not get dependency task from soft path %s. Please re-add the dependency manually.\n"), *Dependency.ToString());
		}
	}
}

#endif

void UAblAbilityTask::GetActorsForTask(const TWeakObjectPtr<const UAblAbilityContext>& Context, TArray<TWeakObjectPtr<AActor>>& OutActorArray) const
{
	verify(m_TaskTargets.Num() != 0 && Context.IsValid());

	OutActorArray.Empty();
	for (TEnumAsByte<EAblAbilityTargetType> target : m_TaskTargets)
	{
		switch (target)
		{
			case EAblAbilityTargetType::ATT_Location:
			case EAblAbilityTargetType::ATT_Camera:
			case EAblAbilityTargetType::ATT_Self:
			{
				AActor* SelfActor = Context->GetSelfActor();

				if (IsTaskValidForActor(SelfActor))
				{
					OutActorArray.Add(SelfActor);			
				}
			}
			break;
			case EAblAbilityTargetType::ATT_Owner:
			{
				AActor* Owner = Context->GetOwner();
				if (IsTaskValidForActor(Owner))
				{
					OutActorArray.Add(Owner);
				}				
			}
			break;
			case EAblAbilityTargetType::ATT_Instigator:
			{
				AActor* Instigator = Context->GetInstigator();
				if (IsTaskValidForActor(Instigator))
				{
					OutActorArray.Add(Instigator);
				}
			}
			break;
			case EAblAbilityTargetType::ATT_TargetActor:
			{
				const TArray<TWeakObjectPtr<AActor>>& UnfilteredTargets = Context->GetTargetActorsWeakPtr();
				for (const TWeakObjectPtr<AActor>& Target : UnfilteredTargets)
				{
					if (IsTaskValidForActor(Target.Get()))
					{
						OutActorArray.Add(Target);
					}
				}
			}
			break;
			default:
			{
				checkNoEntry();
			}
			break;
		}
	}
}

AActor* UAblAbilityTask::GetSingleActorFromTargetType(const TWeakObjectPtr<const UAblAbilityContext>& Context, EAblAbilityTargetType TargetType) const
{
	check(Context.IsValid());

	switch (TargetType)
	{
		case EAblAbilityTargetType::ATT_Location:
		case EAblAbilityTargetType::ATT_Camera:
		case EAblAbilityTargetType::ATT_Self:
		{
			if (AActor* Actor = Context->GetSelfActor())
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		case EAblAbilityTargetType::ATT_Instigator:
		{
			if (AActor* Actor = Context->GetInstigator())
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		case EAblAbilityTargetType::ATT_Owner:
		{
			if (AActor* Actor = Context->GetOwner())
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		case EAblAbilityTargetType::ATT_TargetActor:
		{
			if (!Context->GetTargetActors().Num())
			{
				return nullptr;
			}

			if (AActor* Actor = Context->GetTargetActors()[0])
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		default:
		{
		}
		break;
	}

	return nullptr;
}

bool UAblAbilityTask::IsTaskValidForActor(const AActor* Actor) const
{
	if (!Actor || !Actor->GetWorld() || Actor->IsPendingKillPending() )
	{
		return false;
	}
	
	bool ActorLocallyControlled = false;
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		ActorLocallyControlled = Pawn->IsLocallyControlled();
	}

	ENetMode WorldNetMode = Actor->GetWorld()->GetNetMode();
	if (WorldNetMode == ENetMode::NM_Standalone)
	{
		// Standalone / Offline game, no need to worry about networking.
		return true;
	}
	else
	{
		switch (GetTaskRealm())
		{
			case EAblAbilityTaskRealm::ATR_Client:
			{
				// Client tasks are safe to run on any proxy/auth/etc.
				return WorldNetMode == NM_Client || ActorLocallyControlled || WorldNetMode == NM_ListenServer;
			}
			break;
			case EAblAbilityTaskRealm::ATR_Server:
			{
				return WorldNetMode != NM_Client;
			}
			break;
			case EAblAbilityTaskRealm::ATR_ClientAndServer:
			{
				return true;
			}
			break;
			default:
				break;
		}
	}

	return false;
}

#if !(UE_BUILD_SHIPPING)

void UAblAbilityTask::PrintVerbose(const TWeakObjectPtr<const UAblAbilityContext>& Context, const FString& Output) const
{
	static const UAbleSettings* Settings = nullptr;
	if (!Settings)
	{
		Settings = GetDefault<UAbleSettings>();
	}

	UWorld* World = nullptr;

#if WITH_EDITOR
	// The play world needs to handle these commands if it exists
	if (GIsEditor && GEditor->PlayWorld && !GIsPlayInEditorWorld)
	{
		World = GEditor->PlayWorld;
	}
#endif

	ULocalPlayer* Player = GEngine->GetDebugLocalPlayer();
	if (Player)
	{
		UWorld* PlayerWorld = Player->GetWorld();
		if (!World)
		{
			World = PlayerWorld;
		}
	}

#if WITH_EDITOR
	if (!World && GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
#endif

	if (World)
    {
        AActor* Owner = Context->GetOwner();
        FString AbilityDisplayName = Context->GetAbility() != nullptr ? Context->GetAbility()->GetDisplayName() : TEXT("NULL ABILITY");
        
        FString OwnerWorldName = Owner != nullptr ? FAbleLogHelper::GetWorldName(Context->GetOwner()->GetWorld()) : TEXT("NULL OWNER");
        UKismetSystemLibrary::PrintString(World, 
            FString::Format(TEXT("(World {0}) {1} - {2}"), { OwnerWorldName, AbilityDisplayName, Output }),
            Settings ? Settings->GetEchoVerboseToScreen() : false, 
            Settings ? Settings->GetLogVerbose() : true, 
            FLinearColor(0.0, 0.66, 1.0), 
            Settings ? Settings->GetVerboseScreenLifetime() : 2.0f);
	}
}

#if WITH_EDITOR

EDataValidationResult UAblAbilityTask::IsTaskDataValid(const UAblAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) 
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}

#endif

#endif

#undef LOCTEXT_NAMESPACE