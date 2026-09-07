#include "MikanBoxShapeActor.h"
#include "MikanSceneActor.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "DrawDebugHelpers.h"

// -- UMikanBoxShapeData -----
void UMikanBoxShapeData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanShapeData::Initialize(InValuesObject);

	// Flip Z and Y axies when converting from Mikan to Unreal
	const auto* BoxShapeValues = InValuesObject.getTypedPointer<MikanBoxShapeComponentValues>();
	BoxXSize = BoxShapeValues->box_x_size;
	BoxYSize = BoxShapeValues->box_z_size;
	BoxZSize = BoxShapeValues->box_y_size;
}

bool UMikanBoxShapeData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	// Flip Z and Y axies when converting from Mikan to Unreal
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
		return UMikanShapeData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanBoxShapeData::Describe(TArray<FString>& OutLines) const
{
	UMikanShapeData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("box_x_size"), BoxXSize);
	Mikan::AppendDescribeLine(OutLines, TEXT("box_y_size"), BoxYSize);
	Mikan::AppendDescribeLine(OutLines, TEXT("box_z_size"), BoxZSize);
}

// -- AMikanBoxShapeActor -----
AMikanBoxShapeActor::AMikanBoxShapeActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Extents= FVector::Zero();
}

void AMikanBoxShapeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawDebugBox(GetWorld(), GetActorLocation(), Extents, GetActorRotation().Quaternion(), FColor::Cyan);
}

void AMikanBoxShapeActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	if (auto* BoxShapeData = Cast<UMikanBoxShapeData>(Data)) 
	{ 
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

		Extents = FVector(
			BoxShapeData->GetBoxXSize() * 0.5f * MetersToUU,
			BoxShapeData->GetBoxYSize() * 0.5f * MetersToUU,
			BoxShapeData->GetBoxZSize() * 0.5f * MetersToUU);
	}
}
