#include "MikanStencilActor.h"
#include "MikanStencilTypes.h"
#
// -- UMikanStencilData -----
void UMikanStencilData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanTransformData::Initialize(InValuesObject);

	const auto* StencilValues = InValuesObject.getTypedPointer<MikanStencilComponentValues>();
	bIsDisabled = StencilValues->is_disabled;
	CullMode = static_cast<int32>(StencilValues->cull_mode);
}

bool UMikanStencilData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "is_disabled")
	{
		bIsDisabled = FieldValue.getBoolValue();
		return true;
	}
	else if (FieldName == "cull_mode")
	{
		CullMode = FieldValue.getIntValue();
		return true;
	}
	else
	{
		return UMikanTransformData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanStencilData::Describe(TArray<FString>& OutLines) const
{
	UMikanTransformData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("is_disabled"), bIsDisabled);
	Mikan::AppendDescribeLine(OutLines, TEXT("cull_mode"), CullMode);
}
