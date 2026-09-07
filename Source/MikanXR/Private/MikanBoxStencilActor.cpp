#include "MikanBoxStencilActor.h"
#include "MikanCameraActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "MikanMath.h"
#include "MikanSceneActor.h"
#include "MikanStencilTypes.h"

// -- UMikanBoxStencilData -----
void UMikanBoxStencilData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanStencilData::Initialize(InValuesObject);

	// Swap Z and Y axes between Mikan and Unreal
	const auto* BoxStencilValues = InValuesObject.getTypedPointer<MikanBoxStencilComponentValues>();
	BoxXSize = BoxStencilValues->box_x_size;
	BoxYSize = BoxStencilValues->box_z_size;
	BoxZSize = BoxStencilValues->box_y_size;
}

bool UMikanBoxStencilData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	// Swap Z and Y axes between Mikan and Unreal
	if (FieldName == "box_x_size")
	{
		BoxXSize = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "box_z_size")
	{
		BoxYSize = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "box_y_size")
	{
		BoxZSize = FieldValue.getFloatValue();
		return true;
	}
	else
	{
		return UMikanStencilData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanBoxStencilData::Describe(TArray<FString>& OutLines) const
{
	UMikanStencilData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("box_x_size"), BoxXSize);
	Mikan::AppendDescribeLine(OutLines, TEXT("box_y_size"), BoxYSize);
	Mikan::AppendDescribeLine(OutLines, TEXT("box_z_size"), BoxZSize);
}

// -- AMikanBoxStencilActor -----
AMikanBoxStencilActor::AMikanBoxStencilActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Extents= FVector::Zero();
}

void AMikanBoxStencilActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Center = GetActorLocation();
	FRotator Rot = GetActorRotation();
	DrawDebugBox(GetWorld(), Center, Extents, Rot.Quaternion(), FColor::Yellow);
}

void AMikanBoxStencilActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	if (auto* BoxShapeData = Cast<UMikanBoxStencilData>(Data))
	{
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

		Extents = FVector(
			BoxShapeData->GetBoxXSize() * 0.5f * MetersToUU,
			BoxShapeData->GetBoxYSize() * 0.5f * MetersToUU,
			BoxShapeData->GetBoxZSize() * 0.5f * MetersToUU);
	}
}