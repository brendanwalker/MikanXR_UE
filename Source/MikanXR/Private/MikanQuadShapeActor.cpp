#include "MikanQuadShapeActor.h"
#include "MikanSceneActor.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "DrawDebugHelpers.h"

// -- UMikanQuadShapeData -----
void UMikanQuadShapeData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanShapeData::Initialize(InValuesObject);

	const auto* QuadShapeValues = InValuesObject.getTypedPointer<MikanQuadShapeComponentValues>();
	QuadWidth = QuadShapeValues->quad_width;
	QuadHeight = QuadShapeValues->quad_height;
	bIsDoubleSided = QuadShapeValues->is_double_sided;
}

bool UMikanQuadShapeData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "quad_width")
	{
		QuadWidth = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "quad_height")
	{
		QuadHeight = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "is_double_sided")
	{
		bIsDoubleSided = FieldValue.getBoolValue();
		return true;
	}
	else
	{
		return UMikanShapeData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanQuadShapeData::Describe(TArray<FString>& OutLines) const
{
	UMikanShapeData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("quad_width"), QuadWidth);
	Mikan::AppendDescribeLine(OutLines, TEXT("quad_height"), QuadHeight);
	Mikan::AppendDescribeLine(OutLines, TEXT("is_double_sided"), bIsDoubleSided);
}

// -- AMikanQuadShapeActor -----
AMikanQuadShapeActor::AMikanQuadShapeActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	QuadSize= FVector2D::Zero();
}

void AMikanQuadShapeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Draw wireframe quad debug visualization
	const FVector Center = GetActorLocation();
	const FRotator Rot = GetActorRotation();
	const FVector Right = Rot.RotateVector(FVector(QuadSize.X, 0.f, 0.f));
	const FVector Up = Rot.RotateVector(FVector(0.f, 0.f, QuadSize.Y));

	const FVector TL = Center - Right + Up;
	const FVector TR = Center + Right + Up;
	const FVector BL = Center - Right - Up;
	const FVector BR = Center + Right - Up;

	const FColor WireColor = FColor::Cyan;
	DrawDebugLine(GetWorld(), TL, TR, WireColor);
	DrawDebugLine(GetWorld(), TR, BR, WireColor);
	DrawDebugLine(GetWorld(), BR, BL, WireColor);
	DrawDebugLine(GetWorld(), BL, TL, WireColor);
}

void AMikanQuadShapeActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	if (auto* QuadShapeData = Cast<UMikanQuadShapeData>(Data))
	{
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

		QuadSize = FVector2D(
			QuadShapeData->GetQuadWidth() * 0.5f * MetersToUU,
			QuadShapeData->GetQuadHeight() * 0.5f * MetersToUU);
	}
}
