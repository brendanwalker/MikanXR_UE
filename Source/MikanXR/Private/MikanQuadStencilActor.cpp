#include "MikanQuadStencilActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "MikanMath.h"
#include "MikanSceneActor.h"
#include "MikanStencilTypes.h"
#include "DrawDebugHelpers.h"

// -- UMikanQuadStencilData -----
void UMikanQuadStencilData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanStencilData::Initialize(InValuesObject);

	const auto* QuadStencilValues = InValuesObject.getTypedPointer<MikanQuadStencilComponentValues>();
	QuadWidth = QuadStencilValues->quad_width;
	QuadHeight = QuadStencilValues->quad_height;
	bIsDoubleSided = QuadStencilValues->is_double_sided;
}

bool UMikanQuadStencilData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
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
		return UMikanStencilData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanQuadStencilData::Describe(TArray<FString>& OutLines) const
{
	UMikanStencilData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("quad_width"), QuadWidth);
	Mikan::AppendDescribeLine(OutLines, TEXT("quad_height"), QuadHeight);
	Mikan::AppendDescribeLine(OutLines, TEXT("is_double_sided"), bIsDoubleSided);
}

// -- AMikanQuadStencilActor -----
AMikanQuadStencilActor::AMikanQuadStencilActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	QuadSize= FVector2D::Zero();
}

void AMikanQuadStencilActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Center = GetActorLocation();
	FRotator Rot = GetActorRotation();
	FVector Extent = FVector(QuadSize.X, QuadSize.Y, 1.0f);

	DrawDebugBox(GetWorld(), Center, Extent, Rot.Quaternion(), FColor::Yellow);
}

void AMikanQuadStencilActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	if (auto* QuadShapeData = Cast<UMikanQuadStencilData>(Data))
	{
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

		QuadSize = FVector2D(
			QuadShapeData->GetQuadWidth() * 0.5f * MetersToUU,
			QuadShapeData->GetQuadHeight() * 0.5f * MetersToUU);
	}
}