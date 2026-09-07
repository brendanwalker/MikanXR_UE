#include "MikanShapeActor.h"
#include "MikanShapeTypes.h"

// -- UMikanShapeData -----
void UMikanShapeData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanTransformData::Initialize(InValuesObject);

	const auto* ShapeValues = InValuesObject.getTypedPointer<MikanShapeComponentValues>();
	Mikan::ToUnrealString(ShapeValues->shape_graph_path, ShapeGraphPath);
}

bool UMikanShapeData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "shape_graph_path")
	{
		Mikan::ToUnrealString(FieldValue, ShapeGraphPath);
		return true;
	}
	else
	{
		return UMikanTransformData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanShapeData::Describe(TArray<FString>& OutLines) const
{
	UMikanTransformData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("shape_graph_path"), ShapeGraphPath);
}

// -- AMikanShapeActor -----
AMikanShapeActor::AMikanShapeActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}