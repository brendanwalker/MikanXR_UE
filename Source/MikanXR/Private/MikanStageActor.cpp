#include "MikanStageActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "MikanAPI.h"
#include "MikanMath.h"
#include "MikanStageTypes.h"
#include "MikanClient.h"

// -- UMikanStageData -----
void UMikanStageData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanTransformData::Initialize(InValuesObject);

	const auto* StageValues = InValuesObject.getTypedPointer<MikanStageComponentValues>();
	TrackingVolumeId = StageValues->tracking_volume_id;
	StageBoundsMinCM = FMikanMath::MikanVector3fToFVector(StageValues->stage_bounds_min);
	StageBoundsMaxCM = FMikanMath::MikanVector3fToFVector(StageValues->stage_bounds_max);
}

bool UMikanStageData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "tracking_volume_id")
	{
		TrackingVolumeId = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "stage_bounds_min")
	{
		StageBoundsMinCM = FMikanMath::MikanVector3fToFVector(FieldValue.getVector3fValue());
		return true;
	}
	else if (FieldName == "stage_bounds_max")
	{
		StageBoundsMaxCM = FMikanMath::MikanVector3fToFVector(FieldValue.getVector3fValue());
		return true;
	}
	else
	{
		return UMikanTransformData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanStageData::Describe(TArray<FString>& OutLines) const
{
	UMikanTransformData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("tracking_volume_id"), TrackingVolumeId);
	Mikan::AppendDescribeLine(OutLines, TEXT("stage_bounds_min"), StageBoundsMinCM);
	Mikan::AppendDescribeLine(OutLines, TEXT("stage_bounds_max"), StageBoundsMaxCM);
}

AMikanStageActor::AMikanStageActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void AMikanStageActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		if (!MikanClient->OnScriptMessage.IsAlreadyBound(this, &AMikanStageActor::OnMikanScriptMessage))
		{
			MikanClient->OnScriptMessage.AddDynamic(this, &AMikanStageActor::OnMikanScriptMessage);
		}
	}
}

void AMikanStageActor::Destroyed()
{
	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		MikanClient->OnScriptMessage.RemoveDynamic(this, &AMikanStageActor::OnMikanScriptMessage);
	}

	Super::Destroyed();
}

void AMikanStageActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (StageBoundsMax != StageBoundsMin)
	{
		const FVector Center = (StageBoundsMin + StageBoundsMax) * 0.5f;
		const FVector Extent = (StageBoundsMax - StageBoundsMin) * 0.5f;
		DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, FColor::Yellow, false, -1.f, 0, 2.f);
	}
}

void AMikanStageActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	OnMikanStageBoundsChanged();
}

void AMikanStageActor::OnComponentDataChanged(const FString& FieldName)
{
	if (FieldName == "stage_bounds_min" || FieldName == "stage_bounds_max")
	{
		OnMikanStageBoundsChanged();
	}
	else
	{
		Super::OnComponentDataChanged(FieldName);
	}
}

void AMikanStageActor::OnMikanTransformDataChanged()
{
	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient && MikanClient->GetSavedStageTransform(GetTransformId()) != nullptr)
	{
		// A user-placed transform is saved — do not let the Mikan API override it.
		return;
	}

	Super::OnMikanTransformDataChanged();
}

void AMikanStageActor::OnMikanStageBoundsChanged()
{
	auto* StageData= Cast<UMikanStageData>(GetTransformData());

	if (StageData)
	{
		const float CMToUU = FMikanMath::kCentimetersToMeters * GetWorld()->GetWorldSettings()->WorldToMeters;
		StageBoundsMin = StageData->GetStageBoundsMinMM() * CMToUU;
		StageBoundsMax = StageData->GetStageBoundsMaxMM() * CMToUU;
	}
}

#if WITH_EDITOR
void AMikanStageActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AMikanStageActor::PostEditMove(bool bFinished)
{
	// Do NOT call Super — base class snaps back to Mikan transform, which we don't want here.
	if (bFinished)
	{
		AMikanClient* MikanClient = GetOwnerMikanClient();
		if (MikanClient && GetTransformData())
		{
			MikanClient->SetSavedStageTransform(GetTransformId(), GetRootComponent()->GetRelativeTransform());
		}
	}
}
#endif

// Script Message Events
void AMikanStageActor::HandleScriptMessage(const FString& Message)
{
	OnMikanScriptMessage(Message);
}